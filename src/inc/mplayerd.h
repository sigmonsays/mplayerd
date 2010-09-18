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

#ifndef HAVE_MPLAYERD_H
#define HAVE_MPLAYERD_H

#define __USE_GNU

#include <pthread.h>

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/types.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/param.h>
#include <signal.h>

#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "client.h"
#include "dbg.h"


#define TELNET_WILL_ECHO                "\xff\xfb\x01"
#define TELNET_WILL_SUPRESS_GO_AHEAD    "\xff\xfb\x03"

#define STR_NO_SUCH_FILE "No such file or directory\r\n"

#include "dbg.h"

/* structures, enums, etc */

enum mp_status {
	MP_STOPPED,
	MP_PAUSED,
	MP_PLAYING,
	MP_CLOSING
};


struct mp {
	enum mp_status status;
	char filename[4096];
	int length; /* seconds long */
	int readfd, writefd;

	float audio_sec, video_sec;
	int current_frame;

};


/* wrap-up for pthread_create */
struct mp_load_thread_tt {
	char *file;
	struct mp *mpx;

	int instance;
};



/* prototypes */

// mplayerd.c
char *mplayerd_version();
void mplayerd_help();
void print_command_help();

int parse_argv(int argc, char **argv);
int server_setup();
int get_config_file_name(int argc, char **argv, char *file);

void mplayerd_quit();
void mplayerd_quit_sigint();
void mplayerd_quit_sigterm();

/* thread functions */
void mplayer_load_thread(struct mp_load_thread_tt *mp_load_thread_args);
void mplayer_message_thread();
void client_thread(client_list *client);

// clients.c
int send_current_clients(int fd);
int kill_client(int id);

#endif
