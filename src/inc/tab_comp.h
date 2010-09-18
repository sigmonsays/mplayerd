#ifndef HAVE_TAB_COMP_H
#define HAVE_TAB_COMP_H

#include <stdlib.h>

typedef struct {
	struct ll_list *list;
	char append[4096];
	int matches;
} tab_comp_t;


/* prototypes */
tab_comp_t *tab_comp_init(void);
int tab_comp_free(tab_comp_t *tab_comp);
int tab_complete(int idx, tab_comp_t *(*func)(int));

tab_comp_t *tab_complete_commands(int idx);
tab_comp_t *tab_complete_dirs(int idx);

tab_comp_t *tab_complete_instances(int idx);

tab_comp_t *tab_complete_clients(int idx);


#endif
