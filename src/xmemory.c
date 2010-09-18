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

#include "config.h"
#include "xmemory.h"
#include "dbg.h"
#include "ll.h"
#include <pthread.h>

#ifdef DEBUG_MEMORY
	pthread_mutex_t ll_mem_mutex = PTHREAD_MUTEX_INITIALIZER;
	struct ll_list *alloc_list = NULL;
	unsigned long malloc_counter = 0;
#endif

void *xmalloc(size_t size) {
	void *r;
	r = malloc(size);
	if (r == NULL) return NULL;


#ifdef DEBUG_MEMORY
	ll_mem_add(r, size);
	malloc_counter++;
	DBG("memory: malloc'd %p size %d count %ld\n", r, size, malloc_counter);
#endif
	return r;
}


void xfree(void *p) {
#ifdef DEBUG_MEMORY
	void *tmp;
	tmp = ll_mem_find(p);
	if (tmp) {
		ll_mem_delete(tmp);
	} else {
		DBG("memory: warning: %p non-malloc'ed pointer free'd\n", p);
	}
#endif
	free(p);

#ifdef DEBUG_MEMORY
	malloc_counter--;
	DBG("memory: free'd %p count %ld\n", p, malloc_counter);
#endif

#ifdef NULL_FREE_POINTER
	p = NULL;
#endif
	
}


char *xstrdup(char *p) {
	size_t l;
	char *r;

	l = strlen(p);
	r = xmalloc(l + 1);
	strcpy(r, p);
	return r;
}

struct ll_list *ll_mem_new() {
	struct ll_list *list;
	list = (struct ll_list *) malloc( sizeof(struct ll_list) );
	if (list == NULL) return NULL;

	list->head = NULL;
	list->list = NULL;
	list->tail = NULL;

	return (struct ll_list *) list;
}


/* helper functions to track memory leakages */
#ifdef DEBUG_MEMORY

int ll_mem_init() {
	alloc_list = ll_mem_new();
	return 0;
}

int ll_mem_add(void *pointer, size_t size) {
	struct ll *new, *list;
	alloc_t *alloc;

	if ((alloc = ll_mem_find(pointer))) {
		printf("memory: warning: %p previously malloc'd size %d\n", pointer, alloc->size);
	}

	new = (struct ll *) malloc( sizeof(struct ll) );
	if (new == NULL) return 1;

	new->prev = NULL;

	alloc = malloc(sizeof(alloc_t));
	alloc->pointer = pointer;
	alloc->size = size;
	new->data = (void *) alloc;

	list = alloc_list->tail;

	if (alloc_list->head == NULL) {
		alloc_list->head = new;
	} else {
		new->prev = list;
	}

	if (list == NULL) {
		alloc_list->list = new;
	} else {
		list->next = new;
		list = list->next;
		list->next = NULL;
	}

	alloc_list->tail = new;
	new->next = NULL;
	return 0;
}

int ll_mem_free() {
	struct ll *ll_tmp, *list;

	pthread_mutex_lock(&ll_mem_mutex);

	list = alloc_list->head;
	free(alloc_list);
	while (list) {
		ll_tmp = list;
		list = list->next;
		free(ll_tmp->data);
		free(ll_tmp);
	}
	alloc_list = NULL;

	pthread_mutex_unlock(&ll_mem_mutex);
	return 0;
}

int ll_mem_delete(struct ll *entry) {
	struct ll *prev;

	pthread_mutex_lock(&ll_mem_mutex);

	prev = entry->prev;

	if (prev) {
		prev->next = entry->next;
	} else {
		alloc_list->head = entry->next;
	}

	if (entry->next && prev) {
		entry->next->prev = entry->prev;
	}

	if (!entry->next && !prev) {
		alloc_list->list = NULL;
		alloc_list->head = NULL;
		alloc_list->tail = NULL;
	}

	if (!entry->next && prev) {
		/* at the end of the list.. update tail */
		alloc_list->tail = prev;
	}

	free(entry->data);
	free(entry);
	pthread_mutex_unlock(&ll_mem_mutex);
	return 0;
}

void ll_mem_print_statistics() {
	int i = 0;
	struct ll *list;
	alloc_t *alloc;

   printf("\n\n------ DEBUG MEMORY ------\n");
   printf("thread %d malloc_counter: %ld\n", (int) pthread_self(), malloc_counter);

	list = alloc_list->head;

   printf("\n\n------ LEFT OVERS ------\n");
	while (list) {
		alloc = (alloc_t *) list->data;
		printf("mem_print: %d pointer: %p size: %d\n", i++, alloc->pointer, alloc->size);
//		safe_memory_print(alloc->pointer, alloc->size);
		list = list->next;
	}
}

void safe_memory_print(char *pointer, int size) {
	int i;
	u_char c;
	for(i=0; i<size; i++) {
		c = *(pointer + i);
		printf("%p + %d: %d ", pointer, i, c);
//		printf(" %c", c);
		printf("\n");
	}
}

void *ll_mem_find(void *entry) {
	alloc_t *alloc;

	pthread_mutex_lock(&ll_mem_mutex);

	for(alloc_list->list = alloc_list->head; alloc_list->list; alloc_list->list = alloc_list->list->next) {
		alloc = (alloc_t *) alloc_list->list->data;

		if (alloc->pointer == entry) {
			pthread_mutex_unlock(&ll_mem_mutex);
			return alloc_list->list;
		}
	}
	pthread_mutex_unlock(&ll_mem_mutex);

	return NULL;
}

#endif
