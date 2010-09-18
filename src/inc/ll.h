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

#ifndef HAVE_LL_H
#define HAVE_LL_H

#include <stdio.h>
#include <string.h>

struct ll_list {
        struct ll *head, *tail;
        struct ll *list;
};

struct ll {
        char *data;  
        struct ll *next, *prev;
};

/* prototypes */
struct ll_list *ll_new();
int ll_add(struct ll_list *list, char *data);
int ll_add_sort(struct ll_list *m_list, char *data);
int ll_addv(struct ll_list *m_list, void *data, size_t size);
int ll_free(struct ll_list *m_list);
int ll_print(struct ll_list *list);
int ll_print_debug(struct ll_list *list);
int ll_print_reverse(struct ll_list *m_list);

void *ll_find(struct ll_list *list, void *entry, size_t size);
int ll_delete(struct ll_list *list, struct ll *entry);


#endif
