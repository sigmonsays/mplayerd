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

#ifndef HAVE_X_MEMORY_H
#define HAVE_X_MEMORY_H

#include <stdlib.h>
#include <string.h>
#include "ll.h"
#include "config.h"

#ifdef DEBUG_MEMORY

typedef struct {
	void *pointer;
	size_t size;
} alloc_t ;

#endif

void *xmalloc(size_t size);
void xfree(void *p);
char *xstrdup(char *p);

struct ll_list *ll_mem_new();
int ll_mem_init();
int ll_mem_add(void *pointer, size_t size);
int ll_mem_free();
int ll_mem_delete(struct ll *entry);
void ll_mem_print_statistics();
void *ll_mem_find(void *entry);
void safe_memory_print(char *pointer, int size);


#endif


