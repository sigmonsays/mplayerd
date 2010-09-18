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

#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H

#include "mplayerd-version.h"

/* tab completion , note this is still buggy.. */
#define TAB_COMPLETION


/* set free'd pointers to NULL */
/* #define NULL_FREE_POINTER */

/* instance support */
#define INSTANCE_SUPPORT

/* command history */
#define COMMAND_HISTORY


/* count mallocs & help debug mem */
/* #define DEBUG_MEMORY */

#define HISTORY_SIZE 5

#define OUTPUT_DONE "\xA0"

#define MAX_CLIENTS 8

#define MAX_COMMAND_LENGTH 1024

#define BIND_ADDR "127.0.0.1"
#define DEFAULT_PORT 7400

#define DEFAULT_CONFIG_FILE "/etc/mplayerd.conf"

#endif
