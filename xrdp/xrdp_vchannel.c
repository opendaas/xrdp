/**
 * Copyright (C) 2013 Ulteo SAS
 * http://www.ulteo.com
 * Author David LECHEVALIER <david@ulteo.com> 2013
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/
#include "defines.h"
#include "xrdp_vchannel.h"



/*****************************************************************************/
/* returns error */
vchannel* APP_CC
xrdp_vchannel_create()
{
  vchannel* v = g_malloc(sizeof(vchannel), true);
  void* func;


  // loading lib vchannel
  v->handle = g_load_library(CHANSRV_LIBRARY);
  if (v->handle == 0)
  {
    g_printf("Failed to load library: %s\n", g_get_dlerror());
    goto failed;
  }
  func = g_get_proc_address(v->handle, "chansrv_init");
  if (func == 0)
  {
    func = g_get_proc_address(v->handle, "_chansrv_init");
  }
  if (func == 0)
  {
    g_printf("Failed to load function xrdp_module_init: %s\n", g_get_dlerror());
    goto failed;
  }

  v->init = (bool(*)(vchannel*))func;
  func = g_get_proc_address(v->handle, "chansrv_end");
  if (func == 0)
  {
    func = g_get_proc_address(v->handle, "_chansrv_end");
  }
  if (func == 0)
  {
    g_printf("Failed to load function xrdp_module_exit %s\n", g_get_dlerror());
    goto failed;
  }

  v->exit = (bool(*)(vchannel*))func;
  if ((v->init != 0) && (v->exit != 0))
  {

    if (v->init(v))
    {
      g_writeln("loaded modual '%s' ok", CHANSRV_LIBRARY);
    }
  }

  return v;

failed:
  g_free(v);
  return NULL;
}


/*****************************************************************************/
/* returns error */
bool APP_CC
xrdp_vchannel_setup(vchannel* vc)
{
  int index;
  int chan_id;
  int chan_flags;
  int size;
  char chan_name[256];

  index = 0;
  while (libxrdp_query_channel(vc->session, index++, chan_name, &chan_flags) == 0)
  {
    chan_id = libxrdp_get_channel_id(vc->session, chan_name);
    vc->add_channel(vc, chan_name, chan_id, chan_flags);
  }

  return true;
}


/*****************************************************************************/
/* returns error */
int APP_CC
xrdp_vchannel_send_data(vchannel* vc, int chan_id, char* data, int size)
{
  int chan_flags;
  int total_size;
  int sent;
  int rv;

  if (data == 0)
  {
    g_printf ("xrdp[xrdp_vchannel_send_data]: no data to send");
    return 1;
  }
  rv = 0;
  sent = 0;
  total_size = size;
  while (sent < total_size)
  {
    size = MIN(1600, total_size - sent);
    chan_flags = 0;
    if (sent == 0)
    {
      chan_flags |= 1; /* first */
    }
    if (size + sent == total_size)
    {
      chan_flags |= 2; /* last */
    }

    rv = libxrdp_send_to_channel(vc->session,chan_id, data + sent, size, total_size, chan_flags);
    if (rv != 0)
    {
      break;
    }

    sent += size;
  }
  return rv;
}


/*****************************************************************************/
/* returns error
   data coming from client that need to go to channel handler */
int APP_CC
xrdp_vchannel_process_channel_data(vchannel* vc, tbus param1, tbus param2, tbus param3, tbus param4)
{
  int rv;
  int length;
  int total_length;
  int flags;
  int id;
  unsigned char* data;
  rv = 0;
  if ((vc != 0))
  {
    if (data != 0)
    {
      id = LOWORD(param1);
      flags = HIWORD(param1);
      length = param2;
      data = (char*)param3;
      total_length = param4;
      if (total_length < length)
      {
        g_writeln("warning in xrdp_mm_process_channel_data total_len < length");
        total_length = length;
      }
      vc->send_data(vc, data, id, flags, length, total_length);
    }
  }
  return rv;
}
