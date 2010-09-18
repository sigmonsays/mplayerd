#include "ll.h"
#include "xmemory.h"

struct ll_list *ll_new() {
	struct ll_list *list;

	list = (struct ll_list *) xmalloc( sizeof(struct ll_list) );
	if (list == NULL) return NULL;

	list->head = NULL;
	list->list = NULL;
	list->tail = NULL;

	return (struct ll_list *) list;
}

/* add new entry to linked list 
	0 on success
	non-zero errors:
	1 xmalloc failed
*/
int ll_addv(struct ll_list *m_list, void *data, size_t size) {
	struct ll *new, *list;

	new = (struct ll *) xmalloc( sizeof(struct ll) );
	if (new == NULL) return 1;

	new->prev = NULL;

	new->data = xmalloc(size);
	memcpy(new->data, data, size);

	list = m_list->tail;

	if (m_list->head == NULL) {
		m_list->head = new;
	} else {
		new->prev = list;
	}

	if (list == NULL) {
		m_list->list = new;
	} else {
		list->next = new;
		list = list->next;
		list->next = NULL;
	}

	m_list->tail = new;
	new->next = NULL;
	return 0;
}

/* add new entry to linked list 
	0 on success
	non-zero errors:
	1 xmalloc failed
*/
int ll_add(struct ll_list *m_list, char *data) {
	struct ll *new, *list;

	new = (struct ll *) xmalloc( sizeof(struct ll) );
	if (new == NULL) return 1;

	new->prev = NULL;

	new->data = (char *) xstrdup(data);

	list = m_list->tail;

	if (m_list->head == NULL)  {
		m_list->head = new;
	} else {
		new->prev = list;
	}

	if (list == NULL) {
		m_list->list = new;
	} else {
		list->next = new;
		list = list->next;
		list->next = NULL;
	}

	m_list->tail = new;
	new->next = NULL;
	return 0;
}

/* add entry to linked list sorting ascend
	0 on success
	non-zero errors:
	1 xmalloc failed
*/
int ll_add_sort(struct ll_list *m_list, char *data) {
	struct ll *list;
	struct ll *ll_next = NULL, *ll_prev = NULL;
	struct ll *new;

	// loop through list finding where to add entry
	list = m_list->head;
	ll_prev = NULL;

	while (list) {
		if (strcmp(data, list->data) < 0) {
			ll_next = list;
			break;
		}

		ll_prev = list;
		list = list->next;
	}

	new = (struct ll *) xmalloc( sizeof(struct ll) );
	if (new == NULL) return 1;

	new->prev = NULL;
	new->data = (char *) xstrdup(data);
	if (m_list->head == NULL) m_list->head = new;
	if (ll_prev == NULL && ll_next == NULL) { // only node in the list
		new->next = NULL;

	} else if (ll_prev == NULL && ll_next != NULL) { // put node before other node
		new->prev = list;
		m_list->head = new;
		new->next = ll_next;

	} else if (ll_prev != NULL && ll_next == NULL) { // put node at end of list
		new->prev = list;
		ll_prev->next = new;
		new->next = NULL;
		m_list->tail = new;

	} else {				 // put node in between nodes
		new->prev = list;
		ll_prev->next = new;
		new->next = ll_next;
	}
	return 0;
}

/* free list */
int ll_free(struct ll_list *m_list) {
	struct ll *ll_tmp, *list;
	list = m_list->head;
	xfree(m_list);
	while (list) {
		ll_tmp = list;
		list = list->next;

		xfree(ll_tmp->data);
		xfree(ll_tmp);
	}
	m_list = NULL;
	return 0;
}

int ll_print(struct ll_list *m_list) {
	int i = 0;
	struct ll *list;

	list = m_list->head;

	while (list) {
		printf("%d %s\n", i++, (char *) list->data);
		list = list->next;
	}
	return 0;
}

int ll_print_debug(struct ll_list *m_list) {
	int i = 0;
	struct ll *list;

	list = m_list->head;

	while (list) {
		printf("%d ''%s'' (prev %p, next %p)\n", i++, (char *) list->data, list->prev, list->next);
		list = list->next;
	}
	return 0;
}

int ll_print_reverse(struct ll_list *m_list) {
	int i = 0;
	struct ll *list;

	list = m_list->tail;

	while (list) {
		printf("%d %s\n", i++, (char *) list->data);
		list = list->prev;
	}
	return 0;
}


/* find an entry in the list
	returns an entry pointing or NULL if not found

	NOTE: 	list->list is also set to the current entry if found,
		otherwise it points to the end of the list
*/
void *ll_find(struct ll_list *list, void *entry, size_t size) {

	for(list->list = list->head; list->list; list->list = list->list->next) {
		if (memcmp(list->list->data, entry, size) == 0) return list->list;
	}
	return NULL;
}


/* delete an entry out of the list
	returns -1 on error, 0 on success
*/

int ll_delete(struct ll_list *list, struct ll *entry) {
	struct ll *prev;
	prev = entry->prev;

	xfree(entry->data);

	if (prev) {
		prev->next = entry->next;
	} else {
		list->head = entry->next;
	}

	if (entry->next && prev) {
		entry->next->prev = entry->prev;
	}

	if (!entry->next && !prev) {
		list->list = NULL;
		list->head = NULL;
		list->tail = NULL;
	}

	if (!entry->next && prev) {
		/* at the end of the list.. update tail */
		list->tail = prev;
	}

	xfree(entry);

	return 0;

}
