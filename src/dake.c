#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "constants.h"
#include "dake.h"
#include "data_message.h"
#include "deserialize.h"
#include "error.h"
#include "random.h"
#include "serialize.h"
#include "str.h"
#include "user_profile.h"

dake_identity_message_t *
dake_identity_message_new(const user_profile_t *profile) {
  if (profile == NULL)
    return NULL;

  dake_identity_message_t *identity_message =
      malloc(sizeof(dake_identity_message_t));
  if (!identity_message) {
    return NULL;
  }

  identity_message->sender_instance_tag = 0;
  identity_message->receiver_instance_tag = 0;
  identity_message->profile->versions = NULL;
  memset(identity_message->Y, 0, ED448_POINT_BYTES);
  identity_message->B = NULL;
  user_profile_copy(identity_message->profile, profile);

  return identity_message;
}

void dake_identity_message_destroy(dake_identity_message_t *identity_message) {
  user_profile_destroy(identity_message->profile);
  ec_point_destroy(identity_message->Y);
  dh_mpi_release(identity_message->B);
  identity_message->B = NULL;
}

void dake_identity_message_free(dake_identity_message_t *identity_message) {
  if (!identity_message)
    return;

  dake_identity_message_destroy(identity_message);
  free(identity_message);
}

otr4_err_t dake_identity_message_asprintf(
    uint8_t **dst, size_t *nbytes,
    const dake_identity_message_t *identity_message) {
  size_t profile_len = 0;
  uint8_t *profile = NULL;
  if (user_profile_asprintf(&profile, &profile_len,
                            identity_message->profile)) {
    return OTR4_ERROR;
  }

  size_t s = PRE_KEY_MIN_BYTES + profile_len;
  uint8_t *buff = malloc(s);
  if (!buff) {
    free(profile);
    return OTR4_ERROR;
  }

  uint8_t *cursor = buff;
  cursor += serialize_uint16(cursor, OTR_VERSION);
  cursor += serialize_uint8(cursor, OTR_IDENTITY_MSG_TYPE);
  cursor += serialize_uint32(cursor, identity_message->sender_instance_tag);
  cursor += serialize_uint32(cursor, identity_message->receiver_instance_tag);
  cursor += serialize_bytes_array(cursor, profile, profile_len);
  cursor += serialize_ec_point(cursor, identity_message->Y);

  size_t len = 0;
  otr4_err_t err = serialize_dh_public_key(cursor, &len, identity_message->B);
  if (err) {
    free(profile);
    free(buff);
    return OTR4_ERROR;
  }
  cursor += len;

  if (dst)
    *dst = buff;

  if (nbytes)
    *nbytes = cursor - buff;

  free(profile);
  return OTR4_SUCCESS;
}

otr4_err_t dake_identity_message_deserialize(dake_identity_message_t *dst,
                                             const uint8_t *src,
                                             size_t src_len) {
  const uint8_t *cursor = src;
  int64_t len = src_len;
  size_t read = 0;

  uint16_t protocol_version = 0;
  if (deserialize_uint16(&protocol_version, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (protocol_version != OTR_VERSION) {
    return OTR4_ERROR;
  }

  uint8_t message_type = 0;
  if (deserialize_uint8(&message_type, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (message_type != OTR_IDENTITY_MSG_TYPE) {
    return OTR4_ERROR;
  }

  if (deserialize_uint32(&dst->sender_instance_tag, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (deserialize_uint32(&dst->receiver_instance_tag, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (user_profile_deserialize(dst->profile, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (deserialize_ec_point(dst->Y, cursor)) {
    return OTR4_ERROR;
  }

  cursor += ED448_POINT_BYTES;
  len -= ED448_POINT_BYTES;

  otr_mpi_t b_mpi; // no need to free, because nothing is copied now
  if (otr_mpi_deserialize_no_copy(b_mpi, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  return dh_mpi_deserialize(&dst->B, b_mpi->data, b_mpi->len, &read);
}

void dake_auth_r_destroy(dake_auth_r_t *auth_r) {
  dh_mpi_release(auth_r->A);
  auth_r->A = NULL;
  ec_point_destroy(auth_r->X);
  user_profile_destroy(auth_r->profile);
  snizkpk_proof_destroy(auth_r->sigma);
}

otr4_err_t dake_auth_r_asprintf(uint8_t **dst, size_t *nbytes,
                                const dake_auth_r_t *auth_r) {
  size_t our_profile_len = 0;
  uint8_t *our_profile = NULL;

  if (user_profile_asprintf(&our_profile, &our_profile_len, auth_r->profile)) {
    return OTR4_ERROR;
  }

  size_t s = AUTH_R_MIN_BYTES + our_profile_len;

  uint8_t *buff = malloc(s);
  if (!buff) {
    free(our_profile);
    return OTR4_ERROR;
  }

  uint8_t *cursor = buff;
  cursor += serialize_uint16(cursor, OTR_VERSION);
  cursor += serialize_uint8(cursor, OTR_AUTH_R_MSG_TYPE);
  cursor += serialize_uint32(cursor, auth_r->sender_instance_tag);
  cursor += serialize_uint32(cursor, auth_r->receiver_instance_tag);
  cursor += serialize_bytes_array(cursor, our_profile, our_profile_len);
  cursor += serialize_ec_point(cursor, auth_r->X);

  size_t len = 0;

  otr4_err_t err = serialize_dh_public_key(cursor, &len, auth_r->A);
  if (err) {
    free(our_profile);
    free(buff);
    return OTR4_ERROR;
  }

  cursor += len;
  cursor += serialize_snizkpk_proof(cursor, auth_r->sigma);

  if (dst)
    *dst = buff;

  if (nbytes)
    *nbytes = cursor - buff;

  free(our_profile);
  return OTR4_SUCCESS;
}

otr4_err_t dake_auth_r_deserialize(dake_auth_r_t *dst, const uint8_t *buffer,
                                   size_t buflen) {
  const uint8_t *cursor = buffer;
  int64_t len = buflen;
  size_t read = 0;

  uint16_t protocol_version = 0;
  if (deserialize_uint16(&protocol_version, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (protocol_version != OTR_VERSION) {
    return OTR4_ERROR;
  }

  uint8_t message_type = 0;
  if (deserialize_uint8(&message_type, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (message_type != OTR_AUTH_R_MSG_TYPE) {
    return OTR4_ERROR;
  }

  if (deserialize_uint32(&dst->sender_instance_tag, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (deserialize_uint32(&dst->receiver_instance_tag, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (user_profile_deserialize(dst->profile, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (deserialize_ec_point(dst->X, cursor)) {
    return OTR4_ERROR;
  }

  cursor += ED448_POINT_BYTES;
  len -= ED448_POINT_BYTES;

  otr_mpi_t tmp_mpi; // no need to free, because nothing is copied now
  if (otr_mpi_deserialize_no_copy(tmp_mpi, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (dh_mpi_deserialize(&dst->A, tmp_mpi->data, tmp_mpi->len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  return deserialize_snizkpk_proof(dst->sigma, cursor, len, &read);
}

void dake_auth_i_destroy(dake_auth_i_t *auth_i) {
  snizkpk_proof_destroy(auth_i->sigma);
}

otr4_err_t dake_auth_i_asprintf(uint8_t **dst, size_t *nbytes,
                                const dake_auth_i_t *auth_i) {
  size_t s = DAKE_HEADER_BYTES + SNIZKPK_BYTES;
  *dst = malloc(s);

  if (!*dst) {
    return OTR4_ERROR;
  }

  if (nbytes) {
    *nbytes = s;
  }

  uint8_t *cursor = *dst;
  cursor += serialize_uint16(cursor, OTR_VERSION);
  cursor += serialize_uint8(cursor, OTR_AUTH_I_MSG_TYPE);
  cursor += serialize_uint32(cursor, auth_i->sender_instance_tag);
  cursor += serialize_uint32(cursor, auth_i->receiver_instance_tag);
  cursor += serialize_snizkpk_proof(cursor, auth_i->sigma);

  return OTR4_SUCCESS;
}

otr4_err_t dake_auth_i_deserialize(dake_auth_i_t *dst, const uint8_t *buffer,
                                   size_t buflen) {
  const uint8_t *cursor = buffer;
  int64_t len = buflen;
  size_t read = 0;

  uint16_t protocol_version = 0;
  if (deserialize_uint16(&protocol_version, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (protocol_version != OTR_VERSION) {
    return OTR4_ERROR;
  }

  uint8_t message_type = 0;
  if (deserialize_uint8(&message_type, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (message_type != OTR_AUTH_I_MSG_TYPE) {
    return OTR4_ERROR;
  }

  if (deserialize_uint32(&dst->sender_instance_tag, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (deserialize_uint32(&dst->receiver_instance_tag, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  return deserialize_snizkpk_proof(dst->sigma, cursor, len, &read);
}

dake_prekey_message_t *dake_prekey_message_new(const user_profile_t *profile) {
  if (profile == NULL)
    return NULL;

  dake_prekey_message_t *prekey_message = malloc(sizeof(dake_prekey_message_t));
  if (!prekey_message) {
    return NULL;
  }

  prekey_message->sender_instance_tag = 0;
  prekey_message->receiver_instance_tag = 0;
  prekey_message->profile->versions = NULL;
  memset(prekey_message->Y, 0, ED448_POINT_BYTES);
  prekey_message->B = NULL;
  user_profile_copy(prekey_message->profile, profile);

  return prekey_message;
}

void dake_prekey_message_destroy(dake_prekey_message_t *prekey_message) {
  user_profile_destroy(prekey_message->profile);
  ec_point_destroy(prekey_message->Y);
  dh_mpi_release(prekey_message->B);
  prekey_message->B = NULL;
}

void dake_prekey_message_free(dake_prekey_message_t *prekey_message) {
  if (!prekey_message)
    return;

  dake_prekey_message_destroy(prekey_message);
  free(prekey_message);
}

otr4_err_t
dake_prekey_message_asprintf(uint8_t **dst, size_t *nbytes,
                             const dake_prekey_message_t *prekey_message) {
  size_t profile_len = 0;
  uint8_t *profile = NULL;
  if (user_profile_asprintf(&profile, &profile_len, prekey_message->profile)) {
    return OTR4_ERROR;
  }

  size_t s = PRE_KEY_MIN_BYTES + profile_len;
  uint8_t *buff = malloc(s);
  if (!buff) {
    free(profile);
    return OTR4_ERROR;
  }

  uint8_t *cursor = buff;
  cursor += serialize_uint16(cursor, OTR_VERSION);
  cursor += serialize_uint8(cursor, OTR_PRE_KEY_MSG_TYPE);
  cursor += serialize_uint32(cursor, prekey_message->sender_instance_tag);
  cursor += serialize_uint32(cursor, prekey_message->receiver_instance_tag);
  cursor += serialize_bytes_array(cursor, profile, profile_len);
  cursor += serialize_ec_point(cursor, prekey_message->Y);

  size_t len = 0;
  otr4_err_t err = serialize_dh_public_key(cursor, &len, prekey_message->B);
  if (err) {
    free(profile);
    free(buff);
    return OTR4_ERROR;
  }
  cursor += len;

  if (dst)
    *dst = buff;

  if (nbytes)
    *nbytes = cursor - buff;

  free(profile);
  return OTR4_SUCCESS;
}

otr4_err_t dake_prekey_message_deserialize(dake_prekey_message_t *dst,
                                           const uint8_t *src, size_t src_len) {
  const uint8_t *cursor = src;
  int64_t len = src_len;
  size_t read = 0;

  uint16_t protocol_version = 0;
  if (deserialize_uint16(&protocol_version, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (protocol_version != OTR_VERSION) {
    return OTR4_ERROR;
  }

  uint8_t message_type = 0;
  if (deserialize_uint8(&message_type, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (message_type != OTR_PRE_KEY_MSG_TYPE) {
    return OTR4_ERROR;
  }

  if (deserialize_uint32(&dst->sender_instance_tag, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (deserialize_uint32(&dst->receiver_instance_tag, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (user_profile_deserialize(dst->profile, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  if (deserialize_ec_point(dst->Y, cursor)) {
    return OTR4_ERROR;
  }

  cursor += ED448_POINT_BYTES;
  len -= ED448_POINT_BYTES;

  otr_mpi_t b_mpi; // no need to free, because nothing is copied now
  if (otr_mpi_deserialize_no_copy(b_mpi, cursor, len, &read)) {
    return OTR4_ERROR;
  }

  cursor += read;
  len -= read;

  return dh_mpi_deserialize(&dst->B, b_mpi->data, b_mpi->len, &read);
}

void dake_non_interactive_auth_message_destroy(
    dake_non_interactive_auth_message_t *non_interactive_auth) {
  dh_mpi_release(non_interactive_auth->A);
  non_interactive_auth->A = NULL;
  ec_point_destroy(non_interactive_auth->X);
  user_profile_destroy(non_interactive_auth->profile);
  snizkpk_proof_destroy(non_interactive_auth->sigma);
  non_interactive_auth->enc_msg = NULL;
  non_interactive_auth->enc_msg_len = 0;
  sodium_memzero(non_interactive_auth->nonce, DATA_MSG_NONCE_BYTES);
  sodium_memzero(non_interactive_auth->auth_mac, HASH_BYTES);
}

otr4_err_t dake_non_interactive_auth_message_asprintf(
    uint8_t **dst, size_t *nbytes, uint8_t *ser_enc_msg,
    const dake_non_interactive_auth_message_t *non_interactive_auth) {
  size_t data_msg_len = 0;

  if (non_interactive_auth->enc_msg)
    data_msg_len = non_interactive_auth->enc_msg_len + MAC_KEY_BYTES;

  size_t our_profile_len = 0;
  uint8_t *our_profile = NULL;

  if (user_profile_asprintf(&our_profile, &our_profile_len,
                            non_interactive_auth->profile))
    return OTR4_ERROR;

  size_t s = NON_INT_AUTH_BYTES + our_profile_len + data_msg_len;

  uint8_t *buff = malloc(s);
  if (!buff) {
    free(our_profile);
    return OTR4_ERROR;
  }

  uint8_t *cursor = buff;
  cursor += serialize_uint16(cursor, OTR_VERSION);
  cursor += serialize_uint8(cursor, OTR_NON_INT_AUTH_MSG_TYPE);
  cursor += serialize_uint32(cursor, non_interactive_auth->sender_instance_tag);
  cursor +=
      serialize_uint32(cursor, non_interactive_auth->receiver_instance_tag);
  cursor += serialize_bytes_array(cursor, our_profile, our_profile_len);
  cursor += serialize_ec_point(cursor, non_interactive_auth->X);

  size_t len = 0;

  otr4_err_t err =
      serialize_dh_public_key(cursor, &len, non_interactive_auth->A);
  if (err) {
    free(our_profile);
    free(buff);
    return OTR4_ERROR;
  }

  cursor += len;
  cursor += serialize_snizkpk_proof(cursor, non_interactive_auth->sigma);

  size_t bodylen = 0;

  if (ser_enc_msg) {
    cursor += serialize_bytes_array(cursor, ser_enc_msg,
                                    non_interactive_auth->enc_msg_len + 4 +
                                        DATA_MSG_NONCE_BYTES);
    free(non_interactive_auth->enc_msg); // nullify
    cursor += bodylen;
  }

  cursor += serialize_bytes_array(cursor, non_interactive_auth->auth_mac,
                                  sizeof(non_interactive_auth->auth_mac));

  if (dst)
    *dst = buff;

  if (nbytes)
    *nbytes = cursor - buff;

  free(our_profile);

  return OTR4_SUCCESS;
}

otr4_err_t dake_non_interactive_auth_message_deserialize(
    dake_non_interactive_auth_message_t *dst, const uint8_t *buffer,
    size_t buflen) {
  const uint8_t *cursor = buffer;
  int64_t len = buflen;
  size_t read = 0;

  uint16_t protocol_version = 0;
  if (deserialize_uint16(&protocol_version, cursor, len, &read))
    return OTR4_ERROR;

  cursor += read;
  len -= read;

  if (protocol_version != OTR_VERSION)
    return OTR4_ERROR;

  uint8_t message_type = 0;
  if (deserialize_uint8(&message_type, cursor, len, &read))
    return OTR4_ERROR;

  cursor += read;
  len -= read;

  if (message_type != OTR_NON_INT_AUTH_MSG_TYPE)
    return OTR4_ERROR;

  if (deserialize_uint32(&dst->sender_instance_tag, cursor, len, &read))
    return OTR4_ERROR;

  cursor += read;
  len -= read;

  if (deserialize_uint32(&dst->receiver_instance_tag, cursor, len, &read))
    return OTR4_ERROR;

  cursor += read;
  len -= read;

  if (user_profile_deserialize(dst->profile, cursor, len, &read))
    return OTR4_ERROR;

  cursor += read;
  len -= read;

  if (deserialize_ec_point(dst->X, cursor))
    return OTR4_ERROR;

  cursor += ED448_POINT_BYTES;
  len -= ED448_POINT_BYTES;

  otr_mpi_t tmp_mpi; // no need to free, because nothing is copied now
  if (otr_mpi_deserialize_no_copy(tmp_mpi, cursor, len, &read))
    return OTR4_ERROR;

  cursor += read;
  len -= read;

  if (dh_mpi_deserialize(&dst->A, tmp_mpi->data, tmp_mpi->len, &read))
    return OTR4_ERROR;

  cursor += read;
  len -= read;

  if (deserialize_snizkpk_proof(dst->sigma, cursor, len, &read))
    return OTR4_ERROR;

  cursor += read;
  len -= read;

  // TODO: this has var length
  // if (data_message_deserialize(dst->enc_msg, cursor, len, &read))
  //  return OTR4_ERROR;

  // cursor += read;
  // len -= read;

  return deserialize_bytes_array(dst->auth_mac, HASH_BYTES, cursor, len);
}

bool not_expired(time_t expires) {
  if (difftime(expires, time(NULL)) > 0) {
    return true;
  }

  return false;
}

static bool no_rollback_detected(const char *versions) {
  while (*versions) {
    if (*versions != '3' && *versions != '4')
      return false;

    versions++;
  }
  return true;
}

// is it always their?
bool valid_received_values(const ec_point_t their_ecdh, const dh_mpi_t their_dh,
                           const user_profile_t *profile) {
  bool valid = true;

  /* Verify that the point their_ecdh received is on curve 448. */
  if (ec_point_valid(their_ecdh) == OTR4_ERROR)
    return false;

  /* Verify that the DH public key their_dh is from the correct group. */
  valid &= dh_mpi_valid(their_dh);

  /* Verify their profile is valid (and not expired). */
  valid &= user_profile_verify_signature(profile);
  valid &= not_expired(profile->expires);
  valid &= no_rollback_detected(profile->versions);

  return valid;
}
