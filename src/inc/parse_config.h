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

#ifndef HAVE_PARSE_CONFIG_H
#define HAVE_PARSE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "ll.h"

typedef struct {
	int port;
	char mplayer_path[PATH_MAX];
	char mplayer_vo[16];
	char mplayer_flags[256];

	struct ll_list *allow_ips;

	char default_home[256];

	char root[PATH_MAX];

	unsigned int max_instances;

} mpd_config;

mpd_config *parse_config(char *cfile);
struct ll_list *parse_ips(char *ips);

#endif
