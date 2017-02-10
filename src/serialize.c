#include <string.h>

#include "serialize.h"

static int
serialize_int(uint8_t *target, const uint64_t data, const int offset) {
  int i;
  int shift = offset;

  for(i = 0; i < offset; i++) {
    shift--;
    target[i] = (data >> shift*8) & 0xFF;
  }

  return offset;
}

int
serialize_uint64(uint8_t *dst, const uint64_t data) {
  return serialize_int(dst, data, sizeof(uint64_t));
}

int
serialize_uint32(uint8_t *dst, const uint32_t data) {
  return serialize_int(dst, data, sizeof(uint32_t));
}

int
serialize_uint16(uint8_t *dst, const uint16_t data) {
  return serialize_int(dst, data, sizeof(uint16_t));
}

int
serialize_uint8(uint8_t *dst, const uint8_t data) {
  return serialize_int(dst, data, sizeof(uint8_t));
}

int
serialize_bytes_array(uint8_t *target, const uint8_t *data, int len) {
  //this is just a memcpy thar returns the ammount copied for convenience
  memcpy(target, data, len);
  return len;
}

int
serialize_mpi(uint8_t *dst, const otr_mpi_t mpi) {
  uint8_t *cursor = dst;

  cursor += serialize_uint32(cursor, mpi->len);
  cursor += serialize_bytes_array(cursor, mpi->data, mpi->len);

  return cursor - dst;
}

int
serialize_ec_public_key(uint8_t *dst, const ec_public_key_t pub) {
  if (!ec_public_key_serialize(dst, sizeof(ec_public_key_t), pub)) {
    return 0;
  }

  return sizeof(ec_public_key_t);
}

int
serialize_ec_point(uint8_t *dst, const ec_point_t point) {
  ec_point_serialize(dst, 56, point);
  return 56;
}


int
serialize_dh_public_key(uint8_t *dst, const dh_public_key_t pub) {
  //From gcrypt MPI
  uint8_t buf[DH3072_MOD_LEN_BYTES] = {0};
  memset(buf, 0, DH3072_MOD_LEN_BYTES);
  size_t written = dh_mpi_serialize(buf, DH3072_MOD_LEN_BYTES, pub);

  //To OTR MPI
  //TODO: Maybe gcrypt MPI already has some API for this.
  //gcry_mpi_print with a different format, maybe?
  otr_mpi_t mpi;
  otr_mpi_set(mpi, buf, written);
  return serialize_mpi(dst, mpi);
}

int
serialize_cs_public_key(uint8_t *dst, const cs_public_key_t *pub) {
  uint8_t *cursor = dst;
  cursor += serialize_uint16(cursor, CRAMER_SHOUP_PUBKEY_TYPE);
  cursor += serialize_ec_point(cursor, pub->c);
  cursor += serialize_ec_point(cursor, pub->d);
  cursor += serialize_ec_point(cursor, pub->h);

  return cursor - dst;
}
