#ifndef HAVE_INSTANCE_H
#define HAVE_INSTANCE_H

#include <pthread.h>
#include "ll.h"

struct mp_instance_t {
	pthread_mutex_t lock;
	int id;
	struct mp *mpx;
	char *args;
};


/* prototypes */
int new_instance();
int free_instance_members(struct mp_instance_t *tmp_inst);
int free_instance(int id);
int free_instances();
struct mp_instance_t *get_instance(int id);
int debug_print_instances();

#endif
