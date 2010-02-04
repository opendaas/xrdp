/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   xrdp: A Remote Desktop Protocol server.
   Copyright (C) Jay Sorg 2005-2009
*/

/**
 *
 * @file sessvc.c
 * @brief Session supervisor
 * @author Simone Fedele
 * 
 */

#if defined(HAVE_CONFIG_H)
#include "config_ac.h"
#endif
#include "file_loc.h"
#include "os_calls.h"
#include "thread_calls.h"
#include "arch.h"

static int g_term = 0;

/*****************************************************************************/
void DEFAULT_CC
term_signal_handler(int sig)
{
  g_writeln("xrdp-sessvc: term_signal_handler: got signal %d", sig);
  g_term = 1;
}

/*****************************************************************************/
void DEFAULT_CC
nil_signal_handler(int sig)
{
  g_writeln("xrdp-sessvc: nil_signal_handler: got signal %d", sig);
}

/******************************************************************************/
/* chansrv can exit at any time without cleaning up, its an xlib app */
int APP_CC
chansrv_cleanup(int pid)
{
  char text[256];

  g_snprintf(text, 255, "/var/spool/xrdp/xrdp_chansrv_%8.8x_main_term", pid);
  if (g_file_exist(text))
  {
    g_file_delete(text);
  }
  g_snprintf(text, 255, "/var/spool/xrdp/xrdp_chansrv_%8.8x_thread_done", pid);
  if (g_file_exist(text))
  {
    g_file_delete(text);
  }
  return 0;
}



/******************************************************************************/
int DEFAULT_CC
main(int argc, char** argv)
{
  int ret;
  int chansrv_pid;
  int wm_pid;
  int x_pid;
  int lerror;
  char exe_path[262];
  char *username;

  if (argc < 4)
  {
    g_writeln("xrdp-sessvc: exiting, not enough params");
    return 1;
  }
  g_signal_kill(term_signal_handler); /* SIGKILL */
  g_signal_terminate(term_signal_handler); /* SIGTERM */
  g_signal_user_interrupt(term_signal_handler); /* SIGINT */
  g_signal_pipe(nil_signal_handler); /* SIGPIPE */
  x_pid = g_atoi(argv[1]);
  wm_pid = g_atoi(argv[2]);
  username = argv[3];

  g_writeln("xrdp-sessvc: waiting for X (pid %d) and WM (pid %d)",
             x_pid, wm_pid);
  /* run xrdp-chansrv as a seperate process */
  chansrv_pid = g_fork();
  if (chansrv_pid == -1)
  {
    g_writeln("xrdp-sessvc: fork error");
    return 1;
  }
  else if (chansrv_pid == 0) /* child */
  {
    g_set_current_dir(XRDP_SBIN_PATH);
    g_snprintf(exe_path, 261, "%s/xrdp-chansrv", XRDP_SBIN_PATH);
    g_execlp3(exe_path, "xrdp-chansrv", username);
    /* should not get here */
    g_writeln("xrdp-sessvc: g_execvp failed");
    return 1;
  }
  lerror = 0;
  /* wait for window manager to get done */
  ret = g_waitpid(wm_pid);
  while ((ret == 0) && !g_term)
  {
    ret = g_waitpid(wm_pid);
  }
  if (ret < 0)
  {
    lerror = g_get_errno();
  }
  g_writeln("xrdp-sessvc: WM is dead (waitpid said %d, errno is %d) "
            "exiting...", ret, lerror);
  /* kill channel server */
  g_writeln("xrdp-sessvc: stopping channel server");
  g_sigterm(chansrv_pid);
  ret = g_waitpid(chansrv_pid);
  while ((ret == 0) && !g_term)
  {
    ret = g_waitpid(chansrv_pid);
  }
  chansrv_cleanup(chansrv_pid);
  /* kill X server */
  g_writeln("xrdp-sessvc: stopping X server");
  g_sigterm(x_pid);
  ret = g_waitpid(x_pid);
  while ((ret == 0) && !g_term)
  {
    ret = g_waitpid(x_pid);
  }
  g_writeln("xrdp-sessvc: clean exit");
  return 0;
}
