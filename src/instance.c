#include "instance.h"
#include "xmemory.h"
#include "mplayer_command.h"
#include "dbg.h"
#include "parse_config.h"

extern int instance_count;
extern struct ll_list *mp_instances;
extern pthread_mutex_t instances_mutex;
extern mpd_config *config;

/* 	creates new instance
	returns instance id, or -1 on error
*/
int new_instance() {
	int id;
	struct mp_instance_t *mp_inst;

	mp_inst = xmalloc(sizeof(struct mp_instance_t));
	if (!mp_inst) return -1;

	if ((mp_inst->mpx = mplayer_init(NULL)) == NULL) {
		xfree(mp_inst);
		return -1;
	}

	pthread_mutex_init(&mp_inst->lock, NULL);

	pthread_mutex_lock(&instances_mutex);

	instance_count++;

	/* find a unique instance id under the max instance count */
	id = 0;
	for(id=1; id < config->max_instances + 1; id++) {
		if (!get_instance(id)) break;
	}
	DBG("Creating new instance with id %d\n", id);

	mp_inst->id = id;
	mp_inst->args = NULL;
	ll_addv(mp_instances, mp_inst, sizeof(struct mp_instance_t));
	xfree(mp_inst);

	pthread_mutex_unlock(&instances_mutex);

	return id;
}

/*	frees an instance
	returns -1 on error, 0 on success
	This should be called with the instances_mutex already locked!
*/
int free_instance(int id) {
	int i = 1, rc = -1, found = 0;
	struct mp_instance_t *tmp_inst;
	struct ll *cur_pos;

	cur_pos = mp_instances->list;

	for(
		mp_instances->list = mp_instances->head;
		mp_instances->list;
		mp_instances->list = mp_instances->list->next
	) {

		tmp_inst = (struct mp_instance_t *) mp_instances->list->data;

		if (tmp_inst->id == id) {
			found = i;
			break;
		}
	}

	if (found) {
		DBG("free_instance(): id %d data %p, instance_count %d\n", id, tmp_inst, instance_count);
		free_instance_members(tmp_inst);

		ll_delete(mp_instances, mp_instances->list);

		instance_count--;
		rc = 0;
	}

	mp_instances->list = cur_pos;

	return rc;
}

/* free malloc'd members of instance
*/
int free_instance_members(struct mp_instance_t *tmp_inst) {
	xfree(tmp_inst->mpx); tmp_inst->mpx = NULL;
	if (tmp_inst->args) xfree(tmp_inst->args); tmp_inst->args = NULL;
	return 0;
}


/*	frees all instances, even the first one
	always returns 0
*/
int free_instances() {
	int i = 0, x = instance_count;

	DBG("free_instances()\n");

	pthread_mutex_lock(&instances_mutex);
	for(i=1; i<=x; i++) {
		free_instance(i);
	}
	instance_count = 0;

	xfree(mp_instances);
	pthread_mutex_unlock(&instances_mutex);

	return 0;
}

int debug_print_instances() {
	struct mp_instance_t *tmp_inst;
	struct ll *cur_pos;

	cur_pos = mp_instances->list;
	for(
		mp_instances->list = mp_instances->head;
		mp_instances->list;
		mp_instances->list = mp_instances->list->next
	) {

		tmp_inst = (struct mp_instance_t *) mp_instances->list->data;

			printf("id %d list %p data %p (prev %p, next %p)\n",
				tmp_inst->id,
				mp_instances->list,
				mp_instances->list->data,
				mp_instances->list->prev,
				mp_instances->list->next
			);
	}
	mp_instances->list = cur_pos;
	printf("\n");
	return 0;
}


/*	get pointer to instance by id
	returns pointer to instance id, otherwise NULL
*/
struct mp_instance_t *get_instance(int id) {
	struct ll *inst = NULL, *cur_position;
	struct mp_instance_t *tmp_inst;

	cur_position = mp_instances->list;

	for(
		mp_instances->list = mp_instances->head;
		mp_instances->list;
		mp_instances->list = mp_instances->list->next
	) {
		tmp_inst = (struct mp_instance_t *) mp_instances->list->data;
		if (tmp_inst->id == id) {
			inst = mp_instances->list;
			break;
		}
	}

	mp_instances->list = cur_position;

	return (struct mp_instance_t *) ((inst) ? inst->data : NULL);
}

