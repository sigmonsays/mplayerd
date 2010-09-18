/*
 * mplayerd
 *
 * Website: http://mplayerd.sourceforge.net/
 * Secondary Website: http://signuts.net/projects/id/59
 *
 * http://www.signuts.net/
 * mplayerd@signuts.net
 * 
 *
 * Copyright (C) 2003, Sig Lange <exonic@signuts.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 */

#ifndef HAVE_CLIENTS_H
#define HAVE_CLIENTS_H

#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <limits.h>

#include "ll.h"
#include "history.h"
#include "config.h"

#define CL_NORMAL_MODE 1
#define CL_HISTORY_MODE 2

typedef struct {
   int id, socket;
   char buf[MAX_COMMAND_LENGTH];
	char hbuf[MAX_COMMAND_LENGTH];

   char cwd[PATH_MAX];

	cmd_hist history;

	int mode;
	pthread_t pthread;
	char *ip;

	int instance;
	int cmd;

} client_list;

int client_send(int index, char *msg);
int client_request(int idx);

int mp_process_command(int idx, char *cmd_line);
int parse_arguments(char *cmd, char **args, int *max);

int client_history_search(int index, char *buf, int buflen);

int check_ip(char *ip, struct ll_list *list);

void init_clients();
int client_sendf(int idx, const char *fmt, ...);


#endif
