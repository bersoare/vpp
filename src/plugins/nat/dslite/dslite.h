/*
 * Copyright (c) 2017 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __included_dslite_h__
#define __included_dslite_h__

#include <vppinfra/dlist.h>
#include <vppinfra/bihash_8_8.h>
#include <vppinfra/bihash_16_8.h>
#include <vppinfra/bihash_24_8.h>

#include <nat/lib/lib.h>
#include <nat/lib/alloc.h>

typedef struct
{
  u16 identifier;
  u16 sequence;
} echo_header_t;

/* session key (4-tuple) */
typedef struct
{
  union
  {
    struct
    {
      ip4_address_t addr;
      u16 port;
      u16 protocol:3, fib_index:13;
    };
    u64 as_u64;
  };
} nat_session_key_t;

typedef struct
{
  union
  {
    struct
    {
      ip6_address_t softwire_id;
      ip4_address_t addr;
      u16 port;
      u8 proto;
      u8 pad;
    };
    u64 as_u64[3];
  };
} dslite_session_key_t;

typedef CLIB_PACKED (struct
{
  nat_session_key_t out2in;
  dslite_session_key_t in2out;
  u32 per_b4_index;
  u32 per_b4_list_head_index;
  f64 last_heard;
  u64 total_bytes;
  u32 total_pkts;
}) dslite_session_t;

typedef struct
{
  ip6_address_t addr;
  u32 sessions_per_b4_list_head_index;
  u32 nsessions;
} dslite_b4_t;

typedef struct
{
  /* Main lookup tables */
  clib_bihash_8_8_t out2in;
  clib_bihash_24_8_t in2out;

  /* Find a B4 */
  clib_bihash_16_8_t b4_hash;

  /* B4 pool */
  dslite_b4_t *b4s;

  /* Session pool */
  dslite_session_t *sessions;

  /* Pool of doubly-linked list elements */
  dlist_elt_t *list_pool;
} dslite_per_thread_data_t;

typedef struct
{
  ip6_address_t aftr_ip6_addr;
  ip4_address_t aftr_ip4_addr;
  ip6_address_t b4_ip6_addr;
  ip4_address_t b4_ip4_addr;
  dslite_per_thread_data_t *per_thread_data;
  u32 num_workers;
  u32 first_worker_index;
  u16 port_per_thread;

  /* nat address pool */
  nat_ip4_pool_t pool;

  /* counters/gauges */
  vlib_simple_counter_main_t total_b4s;
  vlib_simple_counter_main_t total_sessions;

  /* node index */
  u32 dslite_in2out_node_index;
  u32 dslite_in2out_slowpath_node_index;
  u32 dslite_out2in_node_index;

  /* If set then the DSLite component behaves as CPE/B4
   * otherwise it behaves as AFTR */
  u8 is_ce;

  u8 is_enabled;
  u16 msg_id_base;
} dslite_main_t;

typedef struct
{
  u32 next_index;
  u32 session_index;
} dslite_trace_t;

typedef struct
{
  u32 next_index;
} dslite_ce_trace_t;

#define foreach_dslite_error                    \
_(IN2OUT, "valid in2out DS-Lite packets")       \
_(OUT2IN, "valid out2in DS-Lite packets")       \
_(CE_ENCAP, "valid CE encap DS-Lite packets")   \
_(CE_DECAP, "valid CE decap DS-Lite packets")   \
_(NO_TRANSLATION, "no translation")             \
_(BAD_IP6_PROTOCOL, "bad ip6 protocol")         \
_(OUT_OF_PORTS, "out of ports")                 \
_(UNSUPPORTED_PROTOCOL, "unsupported protocol") \
_(BAD_ICMP_TYPE, "unsupported icmp type")       \
_(UNKNOWN, "unknown")

typedef enum
{
#define _(sym,str) DSLITE_ERROR_##sym,
  foreach_dslite_error
#undef _
    DSLITE_N_ERROR,
} dslite_error_t;

extern dslite_main_t dslite_main;
extern vlib_node_registration_t dslite_in2out_node;
extern vlib_node_registration_t dslite_in2out_slowpath_node;
extern vlib_node_registration_t dslite_out2in_node;
extern vlib_node_registration_t dslite_ce_encap_node;
extern vlib_node_registration_t dslite_ce_decap_node;

void dslite_set_ce (dslite_main_t * dm, u8 set);
int dslite_set_aftr_ip6_addr (dslite_main_t * dm, ip6_address_t * addr);
int dslite_set_b4_ip6_addr (dslite_main_t * dm, ip6_address_t * addr);
int dslite_set_aftr_ip4_addr (dslite_main_t * dm, ip4_address_t * addr);
int dslite_set_b4_ip4_addr (dslite_main_t * dm, ip4_address_t * addr);
int dslite_add_del_pool_addr (dslite_main_t * dm, ip4_address_t * addr,
			      u8 is_add);
u8 *format_dslite_trace (u8 * s, va_list * args);
u8 *format_dslite_ce_trace (u8 * s, va_list * args);

#endif /* __included_dslite_h__ */

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
