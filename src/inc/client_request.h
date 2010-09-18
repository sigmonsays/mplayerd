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

#ifndef HAVE_CLIENT_REQUEST_H
#define HAVE_CLIENT_REQUEST_H

#include "ll.h"
#include <unistd.h>
#include "debug.h"
#include "tab_comp.h"

typedef struct {
	char artist[32];
	char album[32];
	char title[32];

} id3v1_tag;

typedef struct {
	int id;
	char *cmd, *description, *args, *help;
	const void  *tab_func;
} cmd_list;

int read_id3_tag(char *file, id3v1_tag *id3);

int get_unique(struct ll *list, int offset, char *common_part);
int get_command_id(char *cmd, int len);
int cmd_do_id3(int idx, char *file);
int check_path(int idx, char *file, char *new_path);

int cmd_do_dirdump(int idx, char *path);
int cmd_do_filedump(int idx, char *path);

#endif
