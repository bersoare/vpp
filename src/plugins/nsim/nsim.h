
/*
 * nsim.h - skeleton vpp engine plug-in header file
 *
 * Copyright (c) <current-year> <your-organization>
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
#ifndef __included_nsim_h__
#define __included_nsim_h__

#include <vnet/vnet.h>
#include <vnet/ip/ip.h>
#include <vnet/ethernet/ethernet.h>

#include <vppinfra/hash.h>
#include <vppinfra/error.h>

#define NSIM_MAX_TX_BURST 32	/**< max packets in a tx burst */

typedef struct
{
  f64 tx_time;
  u32 rx_sw_if_index;
  u32 tx_sw_if_index;
  u32 output_next_index;
  u32 buffer_index;
  u32 pad;			/* pad to 32-bytes */
} nsim_wheel_entry_t;

typedef struct
{
  u32 wheel_size;
  u32 cursize;
  u32 head;
  u32 tail;
  nsim_wheel_entry_t *entries;
    CLIB_CACHE_LINE_ALIGN_MARK (pad);
} nsim_wheel_t;

typedef struct nsim_node_ctx
{
  vnet_feature_config_main_t *fcm;
  f64 expires;
  u32 *drop;
  u32 *reord;
  u16 *reord_nexts;
  u32 *fwd;
  u16 *fwd_nexts;
  u8 *action;
  u32 n_buffered;
  u32 n_loss;
  u32 n_reordered;
} nsim_node_ctx_t;

#define foreach_nsm_action			\
  _(DROP, "Packet loss")			\
  _(REORDER, "Packet reorder")

enum nsm_action_bit
{
#define _(sym, str) NSIM_ACTION_##sym##_BIT,
  foreach_nsm_action
#undef _
};

typedef enum nsm_action
{
#define _(sym, str) NSIM_ACTION_##sym = 1 << NSIM_ACTION_##sym##_BIT,
  foreach_nsm_action
#undef _
} nsm_action_e;

typedef struct
{
  /* API message ID base */
  u16 msg_id_base;

  /* output feature arc index */
  u16 arc_index;

  /* Two interfaces, cross-connected with delay */
  u32 sw_if_index0, sw_if_index1;
  u32 output_next_index0, output_next_index1;

  /* N interfaces, using the output feature */
  u32 *output_next_index_by_sw_if_index;

  /* Random seed for loss-rate simulation */
  u32 seed;

  /* Per-thread scheduler wheels */
  nsim_wheel_t **wheel_by_thread;

  /* Config parameters */
  f64 delay;
  f64 bandwidth;
  f64 drop_fraction;
  f64 reorder_fraction;
  u32 packet_size;
  u32 wheel_slots_per_wrk;
  u32 poll_main_thread;

  u64 mmap_size;

  /* Wheels are configured */
  int is_configured;

  /* convenience */
  vlib_main_t *vlib_main;
  vnet_main_t *vnet_main;
} nsim_main_t;

extern nsim_main_t nsim_main;

extern vlib_node_registration_t nsim_node;
extern vlib_node_registration_t nsim_input_node;

#endif /* __included_nsim_h__ */

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
