/*
 *  This file is part of the Off-the-Record Next Generation Messaging
 *  library (libotr-ng).
 *
 *  Copyright (C) 2016-2018, the libotr-ng contributors.
 *
 *  This library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../client.h"
#include "../str.h"

#include "../debug.h"

#define ALICE_IDENTITY "alice@otr.example"
#define BOB_IDENTITY "bob@otr.example"
#define CHARLIE_IDENTITY "charlie@otr.example"
#define PHI "alice@otr.jabber.net"
#define FORCE_CREATE_CONVO true

#define WITH_FIXTURE(_p, _c, _t, _f)                                           \
  do {                                                                         \
    g_test_add((_p), _t, NULL, _f##_setup, (_c), _f##_teardown);               \
  } while (0)

// TODO: for structs like scalars and points, use: goldilocks_memeq
// for the rest use our own implementation of mem_cmp
#define otrng_assert_cmpmem(s1, s2, len)                                       \
  do {                                                                         \
    const uint8_t *_s1_evaled = (uint8_t *)(s1);                               \
    const uint8_t *_s2_evaled = (uint8_t *)(s2);                               \
    const size_t _len_evaled = (len);                                          \
    char *__s1 = _otrng_memdump(_s1_evaled, _len_evaled);                      \
    char *__s2 = _otrng_memdump(_s2_evaled, _len_evaled);                      \
    char *__msg = g_strdup_printf(                                             \
        "assertion failed: (%s)\n\n%s (%p): %s\n\n%s (%p): %s\n",              \
        #s1 " ==  " #s2, #s1, _s1_evaled, __s1, #s2, _s2_evaled, __s2);        \
    if (memcmp(_s1_evaled, _s2_evaled, _len_evaled) == 0)                      \
      ;                                                                        \
    else                                                                       \
      g_assertion_message(G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, __msg); \
    free(__s1);                                                                \
    free(__s2);                                                                \
    g_free(__msg);                                                             \
  } while (0)

#define otrng_assert(expr)                                                     \
  do {                                                                         \
    if                                                                         \
      G_LIKELY(expr);                                                          \
    else                                                                       \
      g_assertion_message_expr(G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC,    \
                               #expr);                                         \
  } while (0)

#define otrng_assert_mpi_eq(m1, m2)                                            \
  do {                                                                         \
    const otrng_mpi_t _m1 = {(m1)[0]};                                         \
    const otrng_mpi_t _m2 = {(m2)[0]};                                         \
    g_assert_cmpuint(_m1->len, ==, _m2->len);                                  \
    otrng_assert_cmpmem(_m1->data, _m2->data, _m1->len);                       \
  } while (0)

#define otrng_assert_user_profile_eq(p1, p2)                                   \
  do {                                                                         \
    const user_profile_t *_p1 = (p1);                                          \
    const user_profile_t *_p2 = (p2);                                          \
    otrng_assert_point_equals(_p1->pub_key, _p2->pub_key);                     \
    otrng_assert_cmpmem(_p1->versions, _p2->versions,                          \
                        strlen(_p1->versions) + 1);                            \
    g_assert_cmpuint(_p1->expires, ==, _p2->expires);                          \
    otrng_assert_cmpmem(_p1->signature, _p2->signature,                        \
                        ED448_SIGNATURE_BYTES);                                \
    otrng_assert_mpi_eq(_p1->transitional_signature,                           \
                        _p2->transitional_signature);                          \
  } while (0)

#define otrng_assert_not_zero(s, len)                                          \
  do {                                                                         \
    const uint8_t *_s = (uint8_t *)(s);                                        \
    const size_t _len = (len);                                                 \
    char *__s = _otrng_memdump(_s, _len);                                      \
    char zero_value[_len];                                                     \
    memset(zero_value, 0, sizeof zero_value);                                  \
    char *__msg = g_strdup_printf("assertion failed: (%s)\nRESULT (%p): %s\n", \
                                  #s " is zero", _s, __s);                     \
    if (goldilocks_memeq(_s, zero_value, _len) == 0)                           \
      ;                                                                        \
    else                                                                       \
      g_assertion_message(G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, __msg); \
    free(__s);                                                                 \
    g_free(__msg);                                                             \
  } while (0)

#define otrng_assert_zero(s, len)                                              \
  do {                                                                         \
    const uint8_t *_s = (uint8_t *)(s);                                        \
    const size_t _len = (len);                                                 \
    char zero_value[_len];                                                     \
    memset(zero_value, 0, sizeof zero_value);                                  \
    otrng_assert_cmpmem(zero_value, _s, _len);                                 \
  } while (0)

#define otrng_assert_ec_public_key_eq(pk1, pk2)                                \
  do {                                                                         \
    otrng_assert_cmpmem((pk1), (pk2), sizeof(ec_public_key_t));                \
  } while (0)

#define otrng_assert_dh_public_key_eq(pk1, pk2)                                \
  do {                                                                         \
    g_assert_cmpint(dh_mpi_cmp((pk1), (pk2)), ==, 0);                          \
  } while (0)

#define otrng_assert_root_key_eq(rk1, rk2)                                     \
  do {                                                                         \
    otrng_assert_cmpmem((rk1), (rk2), sizeof(root_key_t));                     \
  } while (0)

#define otrng_assert_chain_key_eq(ck1, ck2)                                    \
  do {                                                                         \
    otrng_assert_cmpmem((ck1), (ck2), sizeof(chain_key_t));                    \
  } while (0)

// TODO: here actually use the designated initializers as I was planning to
// do for the future nonetheless
#define fn_apply(fn, ...)                                                      \
  {                                                                            \
    void *stopper = (int[]){0};                                                \
    void **list = (void *[]){__VA_ARGS__, stopper};                            \
    for (int i = 0; list[i] != stopper; i++)                                   \
      fn(list[i]);                                                             \
  }

#define otrng_free_all(...) fn_apply(otrng_free, __VA_ARGS__);

#define otrng_response_free_all(...) fn_apply(otrng_response_free, __VA_ARGS__);

#define otrng_userstate_free_all(...)                                          \
  fn_apply(otrl_userstate_free, __VA_ARGS__);

#define otrng_client_state_free_all(...)                                       \
  fn_apply(otrng_client_state_free, __VA_ARGS__);

#define otrng_client_free_all(...) fn_apply(otrng_client_free, __VA_ARGS__);

static inline void otrng_assert_point_equals(const ec_point_t expected,
                                             const ec_point_t actual) {
  g_assert_cmpint(otrng_ec_point_eq(expected, actual), !=, otrng_false);
}
