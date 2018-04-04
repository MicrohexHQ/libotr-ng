#ifndef OTRNG_DESERIALIZE_H
#define OTRNG_DESERIALIZE_H

#include "auth.h"
#include "ed448.h"
#include "error.h"
#include "shared.h"

INTERNAL otrng_err_t otrng_deserialize_uint64(uint64_t *n,
                                              const uint8_t *buffer,
                                              size_t buflen, size_t *nread);

INTERNAL otrng_err_t otrng_deserialize_uint32(uint32_t *n,
                                              const uint8_t *buffer,
                                              size_t buflen, size_t *nread);

INTERNAL otrng_err_t otrng_deserialize_uint16(uint16_t *n,
                                              const uint8_t *buffer,
                                              size_t buflen, size_t *nread);

INTERNAL otrng_err_t otrng_deserialize_uint8(uint8_t *n, const uint8_t *buffer,
                                             size_t buflen, size_t *nread);

INTERNAL otrng_err_t otrng_deserialize_data(uint8_t **dst,
                                            const uint8_t *buffer,
                                            size_t buflen, size_t *read);

INTERNAL otrng_err_t otrng_deserialize_bytes_array(uint8_t *dst, size_t dstlen,
                                                   const uint8_t *buffer,
                                                   size_t buflen);

/* otrng_err_t deserialize_mpi_data(uint8_t *dst, const uint8_t *buffer, */
/*                                  size_t buflen, size_t *read); */

INTERNAL otrng_err_t otrng_deserialize_ec_point(ec_point_t point,
                                                const uint8_t *serialized);

INTERNAL otrng_err_t otrng_deserialize_otrng_public_key(
    otrng_public_key_t pub, const uint8_t *serialized, size_t ser_len,
    size_t *read);

INTERNAL otrng_err_t otrng_deserialize_otrng_shared_prekey(
    otrng_shared_prekey_pub_t shared_prekey, const uint8_t *serialized,
    size_t ser_len, size_t *read);

INTERNAL otrng_err_t otrng_deserialize_ec_scalar(ec_scalar_t scalar,
                                                 const uint8_t *serialized,
                                                 size_t ser_len);

INTERNAL otrng_err_t otrng_deserialize_snizkpk_proof(snizkpk_proof_t *proof,
                                                     const uint8_t *serialized,
                                                     size_t ser_len,
                                                     size_t *read);

INTERNAL otrng_err_t otrng_symmetric_key_deserialize(otrng_keypair_t *pair,
                                                     const char *buff,
                                                     size_t len);

#ifdef OTRNG_DESERIALIZE_PRIVATE
#endif

#endif
