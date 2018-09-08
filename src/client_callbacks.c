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

#define OTRNG_CLIENT_CALLBACKS_PRIVATE
#include "client_callbacks.h"

INTERNAL void
otrng_client_callbacks_create_privkey_v4(const otrng_client_callbacks_s *cb,
                                         const void *client_opdata) {
  if (!cb) {
    return;
  }

  // This callback is required and is expected to segfault if not provided.
  cb->create_privkey_v4(client_opdata);
}

INTERNAL void
otrng_client_callbacks_create_privkey_v3(const otrng_client_callbacks_s *cb,
                                         const void *client_opdata) {
  if (!cb) {
    return;
  }

  // This callback is required and is expected to segfault if not provided.
  cb->create_privkey_v3(client_opdata);
}

INTERNAL void
otrng_client_callbacks_create_client_profile(const otrng_client_callbacks_s *cb,
                                             struct otrng_client_state_s *state,
                                             const void *client_opdata) {
  if (!cb) {
    return;
  }

  // This callback is required and is expected to segfault if not provided.
  cb->create_client_profile(state, client_opdata);
}

INTERNAL void
otrng_client_callbacks_create_prekey_profile(const otrng_client_callbacks_s *cb,
                                             struct otrng_client_state_s *state,
                                             const void *client_opdata) {
  if (!cb) {
    return;
  }

  cb->create_prekey_profile(state, client_opdata);
}

INTERNAL void
otrng_client_callbacks_create_shared_prekey(const otrng_client_callbacks_s *cb,
                                            struct otrng_client_state_s *state,
                                            const void *client_opdata) {
  if (!cb) {
    return;
  }

  // TODO: @client The callback may not need to be invoked at all (if the mode
  // does not support non-interactive DAKE, for example).

  // This callback is required and is expected to segfault if not provided.
  cb->create_shared_prekey(state, client_opdata);
}

INTERNAL void
otrng_client_callbacks_create_instag(const otrng_client_callbacks_s *cb,
                                     const void *client_opdata) {
  if (!cb || !cb->create_instag) {
    return;
  }

  cb->create_instag(client_opdata);
}

INTERNAL void
otrng_client_callbacks_gone_secure(const otrng_client_callbacks_s *cb,
                                   const otrng_client_conversation_s *conv) {
  if (!cb || !cb->gone_secure) {
    return;
  }

  cb->gone_secure(conv);
}

INTERNAL void
otrng_client_callbacks_gone_insecure(const otrng_client_callbacks_s *cb,
                                     const otrng_client_conversation_s *conv) {
  if (!cb || !cb->gone_insecure) {
    return;
  }

  cb->gone_insecure(conv);
}

INTERNAL void otrng_client_callbacks_fingerprint_seen(
    const otrng_client_callbacks_s *cb, const otrng_fingerprint_p fp,
    const otrng_client_conversation_s *conv) {
  if (!cb || !cb->fingerprint_seen) {
    return;
  }

  cb->fingerprint_seen(fp, conv);
}

INTERNAL void otrng_client_callbacks_fingerprint_seen_v3(
    const otrng_client_callbacks_s *cb, const otrng_fingerprint_v3_p fp,
    const otrng_client_conversation_s *conv) {
  if (!cb || !cb->fingerprint_seen_v3) {
    return;
  }

  cb->fingerprint_seen_v3(fp, conv);
}

INTERNAL void otrng_client_callbacks_smp_ask_for_secret(
    const otrng_client_callbacks_s *cb,
    const otrng_client_conversation_s *conv) {
  if (!cb || !cb->smp_ask_for_secret) {
    return;
  }

  cb->smp_ask_for_secret(conv);
}

INTERNAL void otrng_client_callbacks_smp_ask_for_answer(
    const otrng_client_callbacks_s *cb, const char *question,
    const otrng_client_conversation_s *conv) {
  if (!cb || !cb->smp_ask_for_answer) {
    return;
  }

  // TODO: The question should be a string
  cb->smp_ask_for_answer((const uint8_t *)question, strlen(question + 1), conv);
}

INTERNAL void otrng_client_callbacks_smp_update(
    const otrng_client_callbacks_s *cb, const otrng_smp_event_t event,
    const uint8_t progress_percent, const otrng_client_conversation_s *conv) {
  if (!cb || !cb->smp_update) {
    return;
  }

  cb->smp_update(event, progress_percent, conv);
}


#ifdef DEBUG_API

#include "debug.h"

API void otrng_client_callbacks_debug_print(FILE *f, int indent,
                                      const otrng_client_callbacks_s *c) {
  otrng_print_indent(f, indent);
  fprintf(f, "client_callbacks(");
  otrng_debug_print_pointer(f, c);
  fprintf(f, ") {\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "get_account_and_protocol = ");
  otrng_debug_print_pointer(f, c->get_account_and_protocol);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "create_instag = ");
  otrng_debug_print_pointer(f, c->create_instag);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "create_privkey_v3 = ");
  otrng_debug_print_pointer(f, c->create_privkey_v3);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "create_privkey_v4 = ");
  otrng_debug_print_pointer(f, c->create_privkey_v4);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "create_client_profile = ");
  otrng_debug_print_pointer(f, c->create_client_profile);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "create_prekey_profile = ");
  otrng_debug_print_pointer(f, c->create_prekey_profile);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "create_shared_prekey = ");
  otrng_debug_print_pointer(f, c->create_shared_prekey);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "gone_secure = ");
  otrng_debug_print_pointer(f, c->gone_secure);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "gone_insecure = ");
  otrng_debug_print_pointer(f, c->gone_insecure);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "fingerprint_seen = ");
  otrng_debug_print_pointer(f, c->fingerprint_seen);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "fingerprint_seen_v3 = ");
  otrng_debug_print_pointer(f, c->fingerprint_seen_v3);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "smp_ask_for_secret = ");
  otrng_debug_print_pointer(f, c->smp_ask_for_secret);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "smp_ask_for_answer = ");
  otrng_debug_print_pointer(f, c->smp_ask_for_answer);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "smp_update = ");
  otrng_debug_print_pointer(f, c->smp_update);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "received_extra_symm_key = ");
  otrng_debug_print_pointer(f, c->received_extra_symm_key);
  fprintf(f, "\n");

  otrng_print_indent(f, indent + 2);
  fprintf(f, "get_shared_session_state = ");
  otrng_debug_print_pointer(f, c->get_shared_session_state);
  fprintf(f, "\n");

  otrng_print_indent(f, indent);
  fprintf(f, "} // client_callbacks\n");
}

#endif /* DEBUG_API */
