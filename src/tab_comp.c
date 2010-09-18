#include "tab_comp.h"
#include "xmemory.h"
#include "config.h"
#include "ll.h"
#include "client.h"
#include "client_request.h"
#include "parse_config.h"
#include "debug.h"
#include "mplayerd.h"
#include "fileops.h"
#include "instance.h"


extern const cmd_list command_list[];
extern client_list clients[MAX_CLIENTS];
extern mpd_config *config;

extern pthread_mutex_t instances_mutex;
extern struct ll_list *mp_instances;

/* init tab complete structure
	returns structure or NULL on error
*/
tab_comp_t *tab_comp_init(void) {
	tab_comp_t *tab_comp;

	tab_comp = xmalloc(sizeof(tab_comp_t));
	if (!tab_comp) return NULL;

	tab_comp->list = ll_new();
	tab_comp->matches = 0;
	tab_comp->append[0] = 0;
	return tab_comp;
}

/* free tab complete structure
	returns 0
*/
int tab_comp_free(tab_comp_t *tab_comp) {

	if (tab_comp->list) ll_free(tab_comp->list);

	xfree(tab_comp);
	return 0;
}


/* tab complete handle function
	returns 0 on success, -1 on error
*/
int tab_complete(int idx, tab_comp_t* (*func)(int)) {
	struct ll_list *list;
	tab_comp_t *tab_comp;
	int i;

	tab_comp = func(idx);

	if (!tab_comp) return 0;

	i = clients[idx].cmd - 1;

	if (tab_comp->append) {
		strcat(clients[idx].buf, tab_comp->append);
	}

	DBG("tab_comp->matches = %d, ->append = '%s'\n", tab_comp->matches, tab_comp->append);
	if (tab_comp->matches == 1) {

		if (client_send(idx, tab_comp->append) == -1) goto out;

	} else if (tab_comp->matches > 0) {

		list = tab_comp->list;
		if (list) {
			if (client_send(idx, "\r\n") == -1) goto out;

			for(list->list = list->head; list->list; list->list = list->list->next) {
				if (client_send(idx, list->list->data) == -1) goto out;
			}
		}

		if (client_send(idx, "\r\n") == -1) goto out;
		if (client_send(idx, clients[idx].cwd) == -1) goto out;
		if (client_send(idx, " > ") == -1) goto out;
		if (client_send(idx, clients[idx].buf) == -1) goto out;

	}


	tab_comp_free(tab_comp);
	return 0;

out:
	tab_comp_free(tab_comp);
	return -1;
}


/* tab complete clients
*/
tab_comp_t *tab_complete_clients(int idx) {
	int i;
	int matches_found = 0;
	tab_comp_t *tab_comp = NULL;
	char buf[16];

	DBG("Tab complete clients\n");

	tab_comp = tab_comp_init();
	if (!tab_comp) return NULL;

	for(i=0; i<MAX_CLIENTS; i++) {
		if (clients[i].socket > 0) {
			matches_found++;
			sprintf(buf, "%d\r\n", clients[i].id);
			ll_add(tab_comp->list, buf);
		}

	}
	tab_comp->matches = matches_found;
	return tab_comp;
}


/* tab complete instances
*/
tab_comp_t *tab_complete_instances(int idx) {
	int matches_found = 0;
	tab_comp_t *tab_comp = NULL;
	struct ll *cur_pos;
	char buf[32];
	struct mp_instance_t *tmp_inst;

	DBG("Tab complete instances\n");

	tab_comp = tab_comp_init();
	if (!tab_comp) return NULL;

	pthread_mutex_lock(&instances_mutex);
	cur_pos = mp_instances->list;
	for(	mp_instances->list = mp_instances->head;
		mp_instances->list;
		mp_instances->list = mp_instances->list->next
	) {
		tmp_inst = (struct mp_instance_t *) mp_instances->list->data;
		sprintf(buf, "%d ", tmp_inst->id);
		ll_add(tab_comp->list, buf);
		matches_found++;
	}
	mp_instances->list = cur_pos;
	pthread_mutex_unlock(&instances_mutex);

	tab_comp->matches = matches_found;

	if (tab_comp->matches == 1) {
		sprintf(tab_comp->append, "%s", tab_comp->list->head->data);
	}

	return tab_comp;
}

/* tab complete commands
*/
tab_comp_t *tab_complete_commands(int idx) {
	int c = 0, matches_found = 0;
	cmd_list *cmd;
	int l;
	struct ll_list *list;
	tab_comp_t *tab_comp = NULL;
	char tmp[1024];

	tab_comp = tab_comp_init();
	if (!tab_comp) return NULL;

	list = ll_new();
	if (!list) return NULL;

	l = strlen(clients[idx].buf);

	DBG("Tab complete commands\n");
 
	while ((cmd = (cmd_list *) &command_list[c++]) && l > 0) {
		if (cmd->cmd == NULL) break;

		if (strncmp(cmd->cmd, clients[idx].buf, l) == 0) {   
                                                        
			matches_found++;
			ll_add_sort(list, cmd->cmd);

			sprintf(tmp, "%s\r\n", cmd->cmd);
			ll_add(tab_comp->list, tmp);
		}
	}

	if (matches_found == 1) { /* complete word */
  
		sprintf(tab_comp->append, "%s ", list->head->data + l);

		clients[idx].cmd = get_command_id( list->head->data, -1 );

	} else if (matches_found > 1) {

		get_unique(list->head, l, tab_comp->append);
	}

	ll_free(list);
	tab_comp->matches = matches_found;
	return tab_comp;
}


/* tab complete dirs
*/
tab_comp_t *tab_complete_dirs(int idx) {
	tab_comp_t *tab_comp = NULL;

	char new_path[MAXPATHLEN], tmp_path[MAXPATHLEN];

	char tmp[1024];
	int offset = 0;
	char *p, *p2;
	int rel = 0;
	struct ll_list *f_list = NULL, *list;
	int l, quote = 0;
	int matches_found = 0;

	DBG("Tab complete directories\n");

	p = new_path;
	l = strlen(clients[idx].buf);
	p2 = strrchr(clients[idx].buf, ' ');

	if (strlen(p2) == 0) {
		printf("WARNING: Directory tab completion with no directory\n");
		return NULL;
	}

	tab_comp = tab_comp_init();
	if (!tab_comp) return NULL;

	/* skip past spaces and quotes */
	while ( *(p2 + offset) 
		&&	(
			( *(p2 + offset) == '"' )
			|| (*(p2 + offset) == '\'') 
			|| (*(p2 + offset) == ' ')
		)
	) offset++;


	if (*(p2 + offset) == '/') {			// absolute path completion
		rel = 0;
		sprintf(new_path, "%s/%s", config->root, p2 + offset);

	} else {				// relative path completion
		rel = 1;
		if (*(p2 + 1) == '"' || *(p2 + 1) == '\'') {
			sprintf(new_path, "%s/%s/%s", config->root, clients[idx].cwd, p2 + 2);
			quote = 1;
		} else {
			sprintf(new_path, "%s/%s/%s", config->root, clients[idx].cwd, p2 + 1);
		}
	}

	p2 = strrchr(p, '/');
	l = strlen(p2) - 1;

	matches_found = 0;

	if (rel) {
		strncpy(tmp_path, p, (p2 - p));
		tmp_path[(p2 - p)] = '\0';

	} else {

		if (p2 - p > 0) {
			strncpy(tmp_path, p, (p2 - p));
			tmp_path[(p2 - p)] = '\0';

		} else {
			strcpy(tmp_path, "/");
		}
	}


	if (p2) {

		if (realpath(tmp_path, new_path)) {
			/* if they use .. to escape the root, i'll catch 'em here  -- and send them back to root */
			if (strncmp(config->root, new_path, strlen(config->root))) {
				strcpy(new_path, config->root);
			}
			f_list = load_dir(new_path, 20000);
		}

		DBG("tab_complete_dirs: Using directory '%s'\n", new_path);

		if (f_list) {
			list = ll_new();

			for( f_list->list = f_list->head; f_list->list; f_list->list = f_list->list->next) {

				if (strncmp(f_list->list->data, p2 + 1, l) == 0) {
					matches_found++;
					ll_add_sort(list, f_list->list->data);
				}
			}

			if (matches_found == 1) {

				strcat(tab_comp->append, list->head->data + l);

				sprintf(tmp, "%s/%s", tmp_path, list->head->data);

				if (realpath(tmp, tmp_path) && isdir(tmp_path)) {
					strcat(tab_comp->append, "/");
				}

				if (quote) {
					strcat(tab_comp->append, "\"");
				}

			} else if (matches_found > 1) {

				for( list->list = list->head; list->list; list->list = list->list->next ) {
					sprintf(tmp, "%s\r\n", list->list->data);
					ll_add(tab_comp->list, tmp);
				}

				get_unique(list->head, l,tab_comp->append);

			}
			ll_free(list);
			ll_free(f_list);
		}
	}
	tab_comp->matches = matches_found;
	return tab_comp;
}

