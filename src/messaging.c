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

#include <libotr/privkey.h>

#define OTRNG_MESSAGING_PRIVATE
#define OTRNG_PERSISTENCE_PRIVATE

#include "messaging.h"

#include "persistence.h"

API otrng_user_state_s *
otrng_user_state_new(const otrng_client_callbacks_s *cb) {
  otrng_user_state_s *user_state = malloc(sizeof(otrng_user_state_s));
  if (!user_state) {
    return NULL;
  }

  user_state->states = NULL;
  user_state->clients = NULL;
  user_state->callbacks = cb;
  user_state->user_state_v3 = otrl_userstate_create();

  return user_state;
}

tstatic void free_client_state(void *data) { otrng_client_state_free(data); }

tstatic void free_client(void *data) { otrng_client_free(data); }

API void otrng_user_state_free(otrng_user_state_s *user_state) {
  if (!user_state) {
    return;
  }

  otrng_list_free(user_state->states, free_client_state);
  user_state->states = NULL;

  otrng_list_free(user_state->clients, free_client);
  user_state->clients = NULL;

  otrl_userstate_free(user_state->user_state_v3);

  free(user_state);
}

tstatic int find_state_by_client_id(const void *current, const void *wanted) {
  const otrng_client_state_s *client_state = current;
  return client_state->client_id == wanted;
}

tstatic otrng_client_state_s *get_client_state(otrng_user_state_s *user_state,
                                               const void *client_id) {
  list_element_s *el =
      otrng_list_get(client_id, user_state->states, find_state_by_client_id);
  if (el) {
    return el->data;
  }

  otrng_client_state_s *client_state = otrng_client_state_new(client_id);
  if (!client_state) {
    return NULL;
  }

  // TODO: @architecture why is this necessary?
  client_state->callbacks = user_state->callbacks;
  client_state->user_state = user_state->user_state_v3;

  user_state->states = otrng_list_add(client_state, user_state->states);

  return client_state;
}

tstatic int find_client_by_client_id(const void *current, const void *wanted) {
  const otrng_client_s *client = current;
  return client && client->state && client->state->client_id == wanted;
}

tstatic otrng_messaging_client_s *
otrng_messaging_client_new(otrng_user_state_s *user_state, void *client_id) {
  if (!client_id) {
    return NULL;
  }

  list_element_s *e =
      otrng_list_get(client_id, user_state->clients, find_client_by_client_id);

  if (e) {
    return e->data;
  }
  otrng_client_state_s *client_state = get_client_state(user_state, client_id);
  if (!client_state) {
    return NULL;
  }

  otrng_client_s *client = otrng_client_new(client_state);
  if (!client) {
    return NULL;
  }

  user_state->clients = otrng_list_add(client, user_state->clients);

  return client;
}

otrng_messaging_client_s *
otrng_messaging_client_get(otrng_user_state_s *user_state,

                           void *client_id) {
  list_element_s *el =
      otrng_list_get(client_id, user_state->clients, find_client_by_client_id);
  if (el) {
    return el->data;
  }
  return otrng_messaging_client_new(user_state, client_id);
}

API otrng_result otrng_user_state_private_key_v3_generate_FILEp(
    otrng_user_state_s *user_state, void *client_id, FILE *privf) {
  return otrng_client_state_private_key_v3_write_FILEp(
      get_client_state(user_state, client_id), privf);
}

API otrng_result otrng_user_state_private_key_v3_read_FILEp(
    otrng_user_state_s *user_state, FILE *keys) {
  gcry_error_t res = otrl_privkey_read_FILEp(user_state->user_state_v3, keys);
  if (res) {
    return OTRNG_ERROR;
  }
  return OTRNG_SUCCESS;
}

tstatic otrng_result otrng_user_state_add_private_key_v4(
    otrng_user_state_s *user_state, const void *clientop,
    const uint8_t sym[ED448_PRIVATE_BYTES]) {
  return otrng_client_state_add_private_key_v4(
      get_client_state(user_state, clientop), sym);
}

API otrng_result otrng_user_state_generate_private_key(
    otrng_user_state_s *user_state, void *client_id) {
  uint8_t sym[ED448_PRIVATE_BYTES];
  gcry_randomize(sym, ED448_PRIVATE_BYTES, GCRY_VERY_STRONG_RANDOM);
  return otrng_user_state_add_private_key_v4(user_state, client_id, sym);
}

API otrng_result otrng_user_state_generate_client_profile(
    otrng_user_state_s *user_state, void *client_id) {
  otrng_client_state_s *client_state = get_client_state(user_state, client_id);
  if (!client_state) {
    return OTRNG_ERROR;
  }

  client_profile_s *profile =
      otrng_client_state_build_default_client_profile(client_state);
  if (!profile) {
    return OTRNG_ERROR;
  }

  otrng_result err =
      otrng_client_state_add_client_profile(client_state, profile);
  otrng_client_profile_free(profile);

  return err;
}

API otrng_result otrng_user_state_generate_prekey_profile(
    otrng_user_state_s *user_state, void *client_id) {
  otrng_client_state_s *client_state = get_client_state(user_state, client_id);
  if (!client_state) {
    return OTRNG_ERROR;
  }

  otrng_prekey_profile_s *profile =
      otrng_client_state_build_default_prekey_profile(client_state);
  if (!profile) {
    return OTRNG_ERROR;
  }

  otrng_result err =
      otrng_client_state_add_prekey_profile(client_state, profile);
  otrng_prekey_profile_free(profile);

  return err;
}

API otrng_result otrng_user_state_generate_shared_prekey(
    otrng_user_state_s *user_state, void *client_id) {
  uint8_t sym[ED448_PRIVATE_BYTES];
  gcry_randomize(sym, ED448_PRIVATE_BYTES, GCRY_VERY_STRONG_RANDOM);

  otrng_client_state_s *client_state = get_client_state(user_state, client_id);
  if (!client_state) {
    return OTRNG_ERROR;
  }

  return otrng_client_state_add_shared_prekey_v4(client_state, sym);
}

API otrng_keypair_s *
otrng_user_state_get_private_key_v4(otrng_user_state_s *user_state,
                                    const void *client_id) {
  return otrng_client_state_get_keypair_v4(
      get_client_state(user_state, client_id));
}

tstatic void add_private_key_v4_to_FILEp(list_element_s *node, void *context) {
  FILE *privf = context;
  otrng_client_state_s *client_state = node->data;
  // TODO: check the return value
  if (!otrng_client_state_private_key_v4_write_FILEp(client_state, privf)) {
    return;
  }
}

API otrng_result otrng_user_state_private_key_v4_write_FILEp(
    const otrng_user_state_s *user_state, FILE *privf) {
  if (!privf) {
    return OTRNG_ERROR;
  }

  otrng_list_foreach(user_state->states, add_private_key_v4_to_FILEp, privf);

  return OTRNG_SUCCESS;
}

tstatic void add_shared_prekey_to_FILEp(list_element_s *node, void *context) {
  FILE *privf = context;
  otrng_client_state_s *client_state = node->data;
  // TODO: check the return value
  if (!otrng_client_state_shared_prekey_write_FILEp(client_state, privf)) {
    return;
  }
}

API otrng_result otrng_user_state_shared_prekey_write_FILEp(
    const otrng_user_state_s *user_state, FILE *shared_prekey_f) {
  if (!shared_prekey_f) {
    return OTRNG_ERROR;
  }

  otrng_list_foreach(user_state->states, add_shared_prekey_to_FILEp,
                     shared_prekey_f);

  return OTRNG_SUCCESS;
}

tstatic void add_client_profile_to_FILEp(list_element_s *node, void *context) {
  FILE *privf = context;
  otrng_client_state_s *client_state = node->data;
  otrng_client_state_client_profile_write_FILEp(client_state, privf);
}

API otrng_result otrng_user_state_client_profile_write_FILEp(
    const otrng_user_state_s *user_state, FILE *privf) {
  if (!privf) {
    return OTRNG_ERROR;
  }

  otrng_list_foreach(user_state->states, add_client_profile_to_FILEp, privf);
  return OTRNG_SUCCESS;
}

tstatic void add_prekey_profile_to_FILEp(list_element_s *node, void *context) {
  FILE *privf = context;
  otrng_client_state_s *client_state = node->data;
  otrng_client_state_prekey_profile_write_FILEp(client_state, privf);
}

API otrng_result otrng_user_state_prekey_profile_write_FILEp(
    const otrng_user_state_s *user_state, FILE *privf) {
  if (!privf) {
    return OTRNG_ERROR;
  }

  otrng_list_foreach(user_state->states, add_prekey_profile_to_FILEp, privf);
  return OTRNG_SUCCESS;
}

tstatic void add_prekey_messages_to_FILEp(list_element_s *node, void *context) {
  FILE *privf = context;
  otrng_client_state_s *client_state = node->data;
  if (!otrng_client_state_prekeys_write_FILEp(client_state, privf)) {
    return;
  }
}

API otrng_result otrng_user_state_prekey_messages_write_FILEp(
    const otrng_user_state_s *user_state, FILE *privf) {
  if (!privf) {
    return OTRNG_ERROR;
  }

  otrng_list_foreach(user_state->states, add_prekey_messages_to_FILEp, privf);
  return OTRNG_SUCCESS;
}

API otrng_result otrng_user_state_private_key_v4_read_FILEp(
    otrng_user_state_s *user_state, FILE *privf,
    const void *(*read_client_id_for_key)(FILE *filep)) {
  if (!privf) {
    return OTRNG_ERROR;
  }

  // Scan the whole file for a private key for this client
  while (!feof(privf)) {
    const void *client_id = read_client_id_for_key(privf);
    if (!client_id) {
      continue;
    }

    otrng_client_state_s *client_state =
        get_client_state(user_state, client_id);
    if (otrng_client_state_private_key_v4_read_FILEp(client_state, privf) !=
        OTRNG_SUCCESS) {
      return OTRNG_ERROR; /* We decide to abort, since this means the file is
                             malformed */
    }
  }

  return OTRNG_SUCCESS;
}

API otrng_result otrng_user_state_client_profile_read_FILEp(
    otrng_user_state_s *user_state, FILE *profile_filep,
    const void *(*read_client_id_for_key)(FILE *filep)) {
  if (!profile_filep) {
    return OTRNG_ERROR;
  }

  while (!feof(profile_filep)) {
    const void *client_id = read_client_id_for_key(profile_filep);
    if (!client_id) {
      continue;
    }

    otrng_client_state_s *client_state =
        get_client_state(user_state, client_id);
    if (otrng_client_state_client_profile_read_FILEp(
            client_state, profile_filep) != OTRNG_SUCCESS) {
      return OTRNG_ERROR; /* We decide to abort, since this means the file is
                             malformed */
    }
  }

  return OTRNG_SUCCESS;
}

API otrng_result otrng_user_state_shared_prekey_read_FILEp(
    otrng_user_state_s *user_state, FILE *shared_prekeyf,
    const void *(*read_client_id_for_key)(FILE *filep)) {
  if (!shared_prekeyf) {
    return OTRNG_ERROR;
  }

  // Scan the whole file for a private key for this client
  while (!feof(shared_prekeyf)) {
    const void *client_id = read_client_id_for_key(shared_prekeyf);
    if (!client_id) {
      continue;
    }

    otrng_client_state_s *client_state =
        get_client_state(user_state, client_id);
    if (otrng_client_state_shared_prekey_read_FILEp(
            client_state, shared_prekeyf) != OTRNG_SUCCESS) {
      return OTRNG_ERROR; /* We decide to abort, since this means the file is
                             malformed */
    }
  }

  return OTRNG_SUCCESS;
}

API otrng_result otrng_user_state_prekey_profile_read_FILEp(
    otrng_user_state_s *user_state, FILE *profile_filep,
    const void *(*read_client_id_for_key)(FILE *filep)) {
  if (!profile_filep) {
    return OTRNG_ERROR;
  }

  while (!feof(profile_filep)) {
    const void *client_id = read_client_id_for_key(profile_filep);
    if (!client_id) {
      continue;
    }
    otrng_client_state_s *client_state =
        get_client_state(user_state, client_id);
    if (otrng_client_state_prekey_profile_read_FILEp(
            client_state, profile_filep) != OTRNG_SUCCESS) {
      return OTRNG_ERROR; /* We decide to abort, since this means the file is
                             malformed */
    }
  }

  return OTRNG_SUCCESS;
}

API otrng_result otrng_user_state_prekeys_read_FILEp(
    otrng_user_state_s *user_state, FILE *prekey_filep,
    const void *(*read_client_id_for_prekey)(FILE *filep)) {
  if (!prekey_filep) {
    return OTRNG_ERROR;
  }

  while (!feof(prekey_filep)) {
    const void *client_id = read_client_id_for_prekey(prekey_filep);
    if (!client_id) {
      continue;
    }

    otrng_client_state_s *client_state =
        get_client_state(user_state, client_id);

    if (otrng_client_state_prekey_messages_read_FILEp(
            client_state, prekey_filep) != OTRNG_SUCCESS) {
      return OTRNG_ERROR; /* We decide to abort, since this means the file is
                             malformed */
    }
  }

  return OTRNG_SUCCESS;
}

API otrng_result otrng_user_state_add_instance_tag(
    otrng_user_state_s *user_state, void *client_id, unsigned int instag) {
  return otrng_client_state_add_instance_tag(
      get_client_state(user_state, client_id), instag);
}

// TODO: why we have this?
API unsigned int
otrng_user_state_get_instance_tag(otrng_user_state_s *user_state,
                                  void *client_id) {
  UNUSED_ARG(user_state);
  UNUSED_ARG(client_id);
  return 0;
}

API otrng_result otrng_user_state_instag_generate_generate_FILEp(
    otrng_user_state_s *user_state, void *client_id, FILE *instag) {
  return otrng_client_state_instance_tag_write_FILEp(
      get_client_state(user_state, client_id), instag);
}

API otrng_result otrng_user_state_instance_tags_read_FILEp(
    otrng_user_state_s *user_state, FILE *instag) {
  // We use v3 user_state also for v4 instance tags, for now. */
  gcry_error_t res = otrl_instag_read_FILEp(user_state->user_state_v3, instag);
  if (res) {
    return OTRNG_ERROR;
  }
  return OTRNG_SUCCESS;
}

#ifdef DEBUG_API

#include "debug.h"

static const char **debug_print_ignores = NULL;
static size_t debug_print_ignores_len;
static size_t debug_print_ignores_cap;

API void otrng_add_debug_print_ignore(const char *ign) {
  if (debug_print_ignores == NULL) {
    debug_print_ignores = malloc(7 * sizeof(char *));
    debug_print_ignores_len = 0;
    debug_print_ignores_cap = 7;
  }

  if (debug_print_ignores_len + 1 >= debug_print_ignores_cap) {
    debug_print_ignores_cap += 13;
    debug_print_ignores =
        realloc(debug_print_ignores, debug_print_ignores_cap * sizeof(char *));
  }

  debug_print_ignores[debug_print_ignores_len] = ign;
  debug_print_ignores_len++;
}

API void otrng_clear_debug_print_ignores() { debug_print_ignores_len = 0; }

API otrng_bool otrng_debug_print_should_ignore(const char *ign) {
  int ix;
  for (ix = 0; ix < debug_print_ignores_len; ix++) {
    if (strcmp(ign, debug_print_ignores[ix]) == 0) {
      return otrng_true;
    }
  }
  return otrng_false;
}

static void (*client_id_debug_printer)(FILE *, const void *);

API void otrng_register_client_id_debug_printer(void (*printer)(FILE *,
                                                                const void *)) {
  client_id_debug_printer = printer;
}

API void otrng_client_id_debug_print(FILE *f, const void *client_id) {
  if (client_id_debug_printer) {
    client_id_debug_printer(f, client_id);
  } else {
    otrng_debug_print_pointer(f, client_id);
  }
}

API void otrng_user_state_debug_print(FILE *f, int indent,
                                      otrng_user_state_s *user_state) {
  int ix;
  list_element_s *curr;

  if (otrng_debug_print_should_ignore("user_state")) {
    return;
  }

  otrng_print_indent(f, indent);
  fprintf(f, "user_state(");
  otrng_debug_print_pointer(f, user_state);
  fprintf(f, ") {\n");

  if (otrng_debug_print_should_ignore("user_state->states")) {
    otrng_print_indent(f, indent + 2);
    fprintf(f, "states = IGNORED\n");
  } else {
    otrng_print_indent(f, indent + 2);
    fprintf(f, "states = {\n");
    ix = 0;
    curr = state->states;
    while (curr) {
      otrng_print_indent(f, indent + 4);
      fprintf(f, "[%d] = {\n", ix);
      otrng_client_state_debug_print(f, indent + 6, curr->data);
      otrng_print_indent(f, indent + 4);
      fprintf(f, "} // [%d]\n", ix);
      curr = curr->next;
      ix++;
    }
    otrng_print_indent(f, indent + 2);
    fprintf(f, "} // states\n");
  }

  if (otrng_debug_print_should_ignore("user_state->clients")) {
    otrng_print_indent(f, indent + 2);
    fprintf(f, "clients = IGNORED\n");
  } else {
    otrng_print_indent(f, indent + 2);
    fprintf(f, "clients = {\n");
    ix = 0;
    curr = state->clients;
    while (curr) {
      otrng_print_indent(f, indent + 4);
      fprintf(f, "[%d] = {\n", ix);
      otrng_client_debug_print(f, indent + 6, curr->data);
      otrng_print_indent(f, indent + 4);
      fprintf(f, "} // [%d]\n", ix);
      curr = curr->next;
      ix++;
    }

    otrng_print_indent(f, indent + 2);
    fprintf(f, "} // clients\n");
  }

  if (otrng_debug_print_should_ignore("user_state->callbacks")) {
    otrng_print_indent(f, indent + 2);
    fprintf(f, "callbacks = IGNORED\n");
  } else {
    otrng_print_indent(f, indent + 2);
    fprintf(f, "callbacks = {\n");
    otrng_client_callbacks_debug_print(f, indent + 4, user_state->callbacks);
    otrng_print_indent(f, indent + 2);
    fprintf(f, "} // callbacks\n");
  }

  if (otrng_debug_print_should_ignore("user_state->user_state_v3")) {
    otrng_print_indent(f, indent + 2);
    fprintf(f, "user_state_v3 = IGNORED\n");
  } else {
    otrng_print_indent(f, indent + 2);
    fprintf(f, "user_state_v3 = ");
    otrng_debug_print_pointer(f, user_state->user_state_v3);
    fprintf(f, "\n");
  }

  otrng_print_indent(f, indent);
  fprintf(f, "} // user_state\n");
}

#endif /* DEBUG_API */
