#ifndef OTRNG_KEYS_H
#define OTRNG_KEYS_H

#include "ed448.h"
#include "shared.h"

#define ED448_PUBKEY_TYPE 0x0010
#define ED448_PUBKEY_BYTES 2 + ED448_POINT_BYTES
#define ED448_SHARED_PREKEY_TYPE 0x0011
#define ED448_SHARED_PREKEY_BYTES 2 + ED448_POINT_BYTES

typedef ec_point_t otrng_public_key_t;
typedef ec_scalar_t otrng_private_key_t;
typedef ec_point_t otrng_shared_prekey_pub_t;
typedef ec_scalar_t otrng_shared_prekey_priv_t;

typedef struct {
  /* the private key is this symmetric key, and not the scalar serialized */
  uint8_t sym[ED448_PRIVATE_BYTES];

  otrng_public_key_t pub;
  otrng_private_key_t priv;
} otrng_keypair_t;

// TODO: implement correctly when the spec comes
typedef struct {
  /* the private key is this symmetric key, and not the scalar serialized */
  uint8_t sym[ED448_PRIVATE_BYTES];

  otrng_shared_prekey_pub_t pub;
  otrng_shared_prekey_priv_t priv;
} otrng_shared_prekey_pair_t;

INTERNAL otrng_keypair_t *otrng_keypair_new(void);

INTERNAL void otrng_keypair_generate(otrng_keypair_t *keypair,
                                     const uint8_t sym[ED448_PRIVATE_BYTES]);

INTERNAL void otrng_keypair_free(otrng_keypair_t *keypair);

INTERNAL otrng_err_t otrng_symmetric_key_serialize(
    char **buffer, size_t *buffer_size, uint8_t sym[ED448_PRIVATE_BYTES]);

INTERNAL otrng_shared_prekey_pair_t *otrng_shared_prekey_pair_new(void);

INTERNAL void
otrng_shared_prekey_pair_generate(otrng_shared_prekey_pair_t *prekey_pair,
                                  const uint8_t sym[ED448_PRIVATE_BYTES]);

INTERNAL void
otrng_shared_prekey_pair_free(otrng_shared_prekey_pair_t *prekey_pair);

#ifdef OTRNG_KEYS_PRIVATE

tstatic void keypair_destroy(otrng_keypair_t *keypair);

tstatic void
shared_prekey_pair_destroy(otrng_shared_prekey_pair_t *prekey_pair);

#endif

#endif
