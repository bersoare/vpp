/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2021 Cisco Systems, Inc.
 */

#include <vnet/vnet.h>
#include <vnet/devices/devices.h>
#include <vlib/unix/unix.h>


static u32
next_thread_index (vnet_main_t *vnm, clib_thread_index_t thread_index)
{
  vnet_device_main_t *vdm = &vnet_device_main;
  if (vdm->first_worker_thread_index == 0)
    return 0;

  if (thread_index != 0 && (thread_index < vdm->first_worker_thread_index ||
			    thread_index > vdm->last_worker_thread_index))
    {
      thread_index = vdm->next_worker_thread_index++;
      if (vdm->next_worker_thread_index > vdm->last_worker_thread_index)
	vdm->next_worker_thread_index = vdm->first_worker_thread_index;
    }

  return thread_index;
}