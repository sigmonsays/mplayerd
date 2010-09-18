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
#include "mplayerd.h"
#include "ll.h"
#include "client.h"
#include "fileops.h"
#include "parse_config.h"
#include "client_request.h"

#include "tab_comp.h"
#include "client_commands.h"
#include "xmemory.h"
#include "mplayer_command.h"
#include "mplayer_slave.h"
#include "instance.h"

extern int debug;
extern client_list clients[MAX_CLIENTS];
extern mpd_config *config;
extern pthread_mutex_t instances_mutex;

extern struct ll_list *mp_instances;
extern int instance_count;

/*
	-1 on error	
	0 on success
*/
int client_request(int idx) {
	int process_cmd = 0, i, j, j2, l, r, rc = 0;
	u_char buffer[MAX_COMMAND_LENGTH];
	char tmp[2];
	int matches_found = 0, c = 0, telnet_command = 0;
	char *p2;
	cmd_list *cmd;

#ifdef TAB_COMPLETION
	void *tab_func = NULL;
#endif

	if (clients[idx].socket < 0) return -1;

	r = recv(clients[idx].socket, buffer, 4096, 0);

	if (r <= 0) return -1;

	buffer[r] = '\0';

	l = strlen(clients[idx].buf);

	if (clients[idx].mode == CL_HISTORY_MODE) {
		rc = client_history_search(idx, buffer, r);
		return rc;
	}

	if ((l + r) < MAX_COMMAND_LENGTH) {
		for(i=0; i<r; i++) {

			if (buffer[i] == 255 && r >= 2 && buffer[i + 1] >= 240) telnet_command = 1;

			if (telnet_command == 0) {

				if (buffer[i] == '\r') {
					process_cmd = 1;
#ifdef COMMAND_HISTRY
					clients[idx].history.position = clients[idx].history.count;
#endif /* COMMAND_HISTORY */

					clients[idx].cmd = -1;
					break;

				} else if (buffer[i] == ' ') { // space
					strcat(clients[idx].buf, " ");
					if (client_send(idx, " ") == -1) goto out;

					/* if no command set, and there is a space we'll attempt to set it for tab completion later */
					if (clients[idx].cmd == -1) {
						p2 = index(clients[idx].buf, ' ');
						if (p2) {
							c = p2 - (&clients[idx].buf[0]);
							clients[idx].cmd = get_command_id(clients[idx].buf, c);
							DBG("clients[idx].cmd = %d\n", clients[idx].cmd);
						}
					}


				} else if (buffer[i] == 3) { // control C 

					clients[idx].buf[0] = '\0';
					clients[idx].cmd = -1;
					l = 0;
					if (client_send(idx, "\r\n") == -1) goto out;
					if (client_send(idx, clients[idx].cwd) == -1) goto out;
					if (client_send(idx, " > ") == -1) goto out;

#ifdef COMMAND_HISTORY
					clients[idx].history.position = clients[idx].history.count;
#endif /* COMMAND_HISTORY */

				} else if (buffer[i] == 127) { // backspace
					if (l > 0) {
						clients[idx].buf[--l] = '\0';
						if (client_send(idx, "\b \b") == -1) goto out;
						clients[idx].cmd = -1;
					}

#ifdef COMMAND_HISTORY
				/* this is command history code which is crap too */
				} else if (buffer[i] == 27) {	// special keys

					if (r > 2) {


						if (buffer[i + 1] == 91 && buffer[i + 2] == 65) { /* UP ARROW */

							if (clients[idx].history.position > 0 && clients[idx].history.count) {

								clients[idx].history.position--;

								j2 = strlen(clients[idx].buf);
								for(j=0; j<j2; j++) {
									if (client_send(idx, "\b \b") == -1) goto out;
								}


								if (client_send(idx, clients[idx].history.commands[clients[idx].history.position].command) == -1) goto out;

								strcpy(clients[idx].buf, clients[idx].history.commands[clients[idx].history.position].command);


							}

						} else if (buffer[i + 1] == 91 && buffer[i + 2] == 66) { /* DOWN ARROW */

							if (clients[idx].history.position < (clients[idx].history.count - 1)) {

								clients[idx].history.position++;

								j2 = strlen(clients[idx].buf);
								for(j=0; j<j2; j++) {
									if (client_send(idx, "\b \b") == -1) goto out;
								}

								if (clients[idx].history.position + 1 == clients[idx].history.count) {
									strcpy(clients[idx].buf, "");
								} else {
									if (client_send(idx, clients[idx].history.commands[clients[idx].history.position].command) == -1) goto out;
									strcpy(clients[idx].buf, clients[idx].history.commands[clients[idx].history.position].command);
								}

							}

						}

						i += 3;
						continue;
					}
#endif /* COMMAND_HISTORY */


				} else if (buffer[i] == 18) { // CTRL + R
					if (client_send(idx, "\r\n: ") == -1) goto out;
					clients[idx].hbuf[0] = '\0';
					clients[idx].mode = CL_HISTORY_MODE;

				} else if (buffer[i] == '?') {
					p2 = strchr(clients[idx].buf, ' ');

					if (p2 == NULL) { /* no command specified, print out all help */

						if (client_send(idx, "\r\n") == -1) goto out;
						for(c=0; (cmd = (cmd_list *) &command_list[c])->cmd; c++) {
							if (strncmp(clients[idx].buf, cmd->cmd, l) == 0) {
								if (client_send(idx, cmd->cmd) == -1) goto out;
								if (client_send(idx, "\t\t\t") == -1) goto out;
								if (client_send(idx, cmd->description) == -1) goto out;
								if (client_send(idx, "\r\n") == -1) goto out;
							}
						}
						if (client_send(idx, clients[idx].cwd) == -1) goto out;
						if (client_send(idx, " > ") == -1) goto out;
						if (client_send(idx, clients[idx].buf) == -1) goto out;

					} else { /* print out help on the command in the buffer */
						c = 0;
						j = 0;
						j2 = -1;
						while ((  cmd = (cmd_list *) &command_list[c++]  )) {
							if (cmd->cmd == NULL) break;
							if (strncmp(clients[idx].buf, cmd->cmd, p2 - clients[idx].buf) == 0) {
								if (j2 == -1) j2 = c - 1;
								if (j++ > 1) break;
							}
						}

						if (j == 0) { 
							if (client_send(idx, "\r\nUnknown command\r\n") == -1) goto out;

						} else if (j == 1) { /* one match found, print help for it */
							cmd = (cmd_list *) &command_list[j2];

							if (client_send(idx, "\r\n") == -1) goto out;
							if (client_send(idx, cmd->cmd) == -1) goto out;
							if (client_send(idx, "\t\t") == -1) goto out;
							if (client_send(idx, cmd->description) == -1) goto out;
							if (client_send(idx, "\r\n") == -1) goto out;

						} else if ( j > 1) {
							if (client_send(idx, "\r\nmplayerd: Ambiguous command\r\n") == -1) goto out;
						}

						if (client_send(idx, clients[idx].cwd) == -1) goto out;
						if (client_send(idx, " > ") == -1) goto out;
						if (client_send(idx, clients[idx].buf) == -1) goto out;

					}

#ifdef TAB_COMPLETION
				} else if (buffer[i] == '\t') {
					if (l == 0) clients[idx].cmd = -1;

					if (clients[idx].cmd == -1) {
						p2 = index(clients[idx].buf, ' ');
						if (p2) {
							c = p2 - (&clients[idx].buf[0]);
							clients[idx].cmd = get_command_id(clients[idx].buf, c);
						}
					}

					j = clients[idx].cmd;
					if (j > -1) {
						tab_func = (void *) command_list[j].tab_func;
					} else {
						tab_func = tab_complete_commands;
					}

					DBG("Tab completion function (function %p)\n", tab_func);
					if (tab_func && tab_complete(idx, tab_func) == -1) goto out;

#endif /* TAB_COMPLETION */


				} else { /* not a special key, append */
					sprintf(tmp, "%c", buffer[i]);
					strcat(clients[idx].buf, tmp);
					if (client_send(idx, tmp) == -1) goto out;
				}

			} /* end telnet_command == 0 */

		} /* end for loop */


	} else { /* we don't have room */

		debug_msg(0, "WARNING: client %d (%s) reached recv buffer limit of %d\n", idx, clients[idx].ip, MAX_COMMAND_LENGTH);

		if (client_send(idx, "\r\nERROR: recv buffer limit reached\r\n") == -1) goto out;
		if (client_send(idx, clients[idx].cwd) == -1) goto out;
		if (client_send(idx, " >") == -1) goto out;
		if (client_send(idx, OUTPUT_DONE) == -1) goto out;
		clients[idx].buf[0] = '\0';
	}

	if (process_cmd == 1) {
		matches_found = 0;
		i = c = 0;
		p2 = NULL;

		l = strlen(clients[idx].buf);

		if ((p2 = index(clients[idx].buf, ' '))) {
			i = (int) (p2 - clients[idx].buf);
		} else {
			i = l;
		}
		p2 = NULL;
			
		if (client_send(idx, "\r\n") == -1) goto out;

		c = 0;
		while ((cmd = (cmd_list *) &command_list[c++]) && l > 0) {
			if (cmd->cmd == NULL) break;
			if (strncmp(cmd->cmd, clients[idx].buf, i) == 0) {
				matches_found++;
				if (matches_found > 1) break;
				p2 = cmd->cmd;
			}
		}

		if (matches_found == 1) {
			if (i == l) {
				strcat(clients[idx].buf, p2 + i);
			} else {
				buffer[0] = '\0';
				strncpy(buffer, clients[idx].buf, i);
				buffer[i] = '\0';
				strcat(buffer, p2 + i);
				strcat(buffer, clients[idx].buf + i);

				strcpy(clients[idx].buf, buffer);
			}

#ifdef COMMAND_HISTORY
			add_history(&clients[idx].history, clients[idx].buf);
#endif /* COMMAND_HISTORY */

			rc = mp_process_command(idx, clients[idx].buf);

			clients[idx].buf[0] = '\0';

		} else if (matches_found > 1) {
			if (client_send(idx, "mplayerd: Ambiguous command\r\n") == -1) goto out;
			clients[idx].buf[0] = '\0';

		} else {
			if (i > 0) {
				if (clients[idx].buf[0] != '\n') {
					if (client_send(idx, "mplayerd: Unknown command: ") == -1) goto out;
					p2 = (char *) xmalloc(i + 1);
					strncpy(p2, clients[idx].buf, i);
					*(p2 + i) = '\0';
					if (client_send(idx, p2) == -1) {
						xfree(p2);
						goto out;
					}
					xfree(p2);
					if (client_send(idx, "\r\n") == -1) goto out;
					clients[idx].buf[0] = '\0';
				}
			}
		}

		if (client_send(idx, clients[idx].cwd) == -1) goto out;
		if (client_send(idx, " >") == -1) goto out;
		if (client_send(idx, OUTPUT_DONE) == -1) goto out;

		clients[idx].cmd = -1;
	}

	return rc;
out:
	return -1;
}


/* scan linked list and sets the portion that is common in all strings
 * into the buffer pointer at common_part
 *
 *	returns length of common_part, otherwise 0
*/
int get_unique(struct ll *list, int offset, char *common_part) {
	int x = 0, cig = 0, l;
	struct ll *head;
	char let = 0;

	head = list;
	for(;;) {
		list = head;
		l = strlen(list->data);

		let = *(list->data + x);

		while (list) {
			if ( *(list->data + x) != let ) {
				cig = 1;
				break;
			}
			list = list->next;
		}
		if (cig == 1) break;
		x++;
	}
	list = head;
	l = x - offset;

	strncpy(common_part, list->data + offset, l);
	common_part[l] = 0;
	return l;
}

int mp_process_command(int idx, char *cmdline) {
	int cmdline_len, num_args;
	char *cmd_str, *p;
	char *cmd_args[4] = { NULL, NULL, NULL, NULL };
	struct ll_list *ll_list;
	int l, r, i, rc = 0, j;
	int inst = 0;
	int cmd = -1;

	char buffer[4096];
	char *tab_buf;

	int arg_offset = 0;

	char new_path[PATH_MAX], tmp_path[PATH_MAX];
	cmd_list *cmdl;
	char tmp[16];

	struct mp_instance_t *mp_inst, *tmp_inst;
	pthread_mutex_t *lock;
	struct mp *mpx;

	cmdline_len = strlen(cmdline);

	num_args = 4;
	parse_arguments(cmdline, cmd_args, &num_args);
	cmd_str = cmd_args[0];

	debug_msg(2, "Process command buffer = '%s', %d arguments\n", cmdline, num_args);

#ifdef INSTANCE_SUPPORT
	if (num_args > 1) {
		inst = strtol(cmd_args[1], NULL, 0);
		if (inst) arg_offset = 1;
	}

	if (inst == 0) inst = 1;

	clients[idx].instance = inst;

	// pthread_mutex_lock(&instances_mutex);
	mp_inst = get_instance(inst);
	// pthread_mutex_unlock(&instances_mutex);

	if (!mp_inst) {
		if (client_sendf(idx, "invalid instance %d\r\n", inst) == -1) goto out;
		return 0;
	}

#else /* INSTANCE_SUPPORT */
	inst = 1;
	mp_inst = get_instance(inst);

#endif /* INSTANCE_SUPPORT */

	lock = &mp_inst->lock;
	mpx = mp_inst->mpx;

	if (clients[idx].cmd == -1) {
		clients[idx].cmd = get_command_id(cmd_str, -1);
	}
	cmd = clients[idx].cmd;

	DBG("command %d (%s)\n", cmd, ((cmd > -1) ? command_list[cmd].cmd : NULL) );

	if (cmd == CMD_STATUS) {

#ifdef INSTANCE_SUPPORT
		if (client_sendf(idx, "instance: %d\r\n", inst) == -1) goto out;
#endif /* INSTANCE_SUPPORT */

		// pthread_mutex_lock(lock);

		if (mpx->filename[0] == '\0') {
			if (client_send(idx, "file: none\r\n") == -1) {
				// pthread_mutex_unlock(lock);
				goto out;
			}
		} else {
			if (client_sendf(idx, "file: %s\r\n", mpx->filename) == -1) {
				// pthread_mutex_unlock(lock);
				goto out;
			}
		}
		if (client_sendf(idx, "status: %s\r\n", mplayer_status_str(mpx)) == -1) {
			// pthread_mutex_unlock(lock);
			goto out;
		}

		if (mpx->status == MP_PLAYING || mpx->status == MP_PAUSED) {
			if (client_sendf(idx, 
				"time: %.2f %.2f sec\r\n"
				"length: %d\r\n"
				"frame: %d\r\n",
				mpx->video_sec, mpx->audio_sec, mpx->length, mpx->current_frame) == -1) {

				// pthread_mutex_unlock(lock);
				goto out;
			}

		}
		// pthread_mutex_unlock(lock);

	} else if (cmd == CMD_QUIT) {
		goto out;

	} else if (cmd == CMD_SHUTDOWN) {
		if (num_args > 1 + arg_offset) {
			if (strcmp(cmd_args[1 + arg_offset], "now") == 0) {
				if (client_send(idx, "1 shutdown\r\nshutting down...\r\n") == -1) goto out;
				rc = -2;
			} else {
				if (client_send(idx, "0 shutdown\r\nuse 'shutdown now' to shutdown\r\n") == -1) goto out;
			}
		} else {
			if (client_send(idx, "0 shutdown\r\nuse 'shutdown now' to shutdown\r\n") == -1) goto out;
		}

	} else if (cmd == CMD_VERSION) {
		p = mplayerd_version();
		if (client_send(idx, p) == -1) {
			xfree(p);
			goto out;
		}
		xfree(p);
		if (client_send(idx, "\r\n") == -1) goto out;

	} else if (cmd == CMD_HELP) {
		i = 0;
		cmdl = (cmd_list *) &command_list[i];

		if (num_args > 1 + arg_offset) {
			while (1) {
				cmdl = (cmd_list *) &command_list[i++];
				if (cmdl->cmd == NULL) break;
				if (strcmp(cmd_args[1 + arg_offset], cmdl->cmd) == 0) {
					i = -1;
					break;
				}
			}

			j = 0;

			if (i == -1) { /* found command */
				if (client_sendf(idx, "Syntax: %s", cmdl->cmd) == -1) goto out;
				p = cmdl->args;
				for(i=0; *(p + i); i++) {

					if (*(p + i) == '*') {
						if (client_send(idx, " [") == -1) goto out;
						j = 1;
						continue;
					}

					if (*(p + i) == 's') {
						if (client_send(idx, " $string") == -1) goto out;

					} else if (*(p + i) == 'f') {
						if (client_send(idx, " $file") == -1) goto out;

					} else if (*(p + i) == 'd') {
						if (client_send(idx, " $directory") == -1) goto out;

					} else if (*(p + i) == 'i') {
						if (client_send(idx, " $integer") == -1) goto out;


					} else if (*(p + i) == 'n') {
						if (client_send(idx, " $instance") == -1) goto out;

					} else {
						if (client_send(idx, " $X") == -1) goto out;
					}

					if (j) {
						if (client_send(idx, " ]") == -1) goto out;
						j = 0;
					}

				}

				if (client_send(idx, "\r\nDescription: ") == -1) goto out;
				if (client_send(idx, cmdl->help) == -1) goto out;
				if (client_send(idx, "\r\n") == -1) goto out;
				
			} else {
				if (client_send(idx, "Unknown command\r\n") == -1) goto out;
			}

		} else {
			while (1) {
				cmdl = (cmd_list *) &command_list[i++];
				if (cmdl->cmd == NULL) break;

				if (client_send(idx, cmdl->cmd) == -1) goto out;
				if (client_send(idx, "\t\t\t") == -1) goto out;
				if (client_send(idx, cmdl->description) == -1) goto out;
				if (client_send(idx, "\r\n") == -1) goto out;
			}
		}
	

	} else if (cmd == CMD_VOLUME) {

		if (num_args > 1 + arg_offset) {
			pthread_mutex_lock(lock);
			r = mplayer_volume(mpx, strtol(cmd_args[1 + arg_offset], NULL, 0));
			// pthread_mutex_unlock(lock);

			if (r == -1) {
				if (client_send(idx, "0 volume error\r\n") == -1) goto out;
			} else {
				if (client_send(idx, "1 volume set\r\n") == -1) goto out;
			}
		} else {
			if (client_send(idx, "0 Invalid argument\r\n") == -1) goto out;
		}

	} else if (cmd == CMD_LITERAL) {

		if (num_args > 1 + arg_offset) {
			// pthread_mutex_lock(lock);
			r = mplayer_command(mp_inst->mpx, cmd_args[1 + arg_offset]);
			// pthread_mutex_unlock(lock);

			if (r == -1) {
				if (client_send(idx, "0 mplayer_command error\r\n") == -1) goto out;
			} else {

				if (client_sendf(idx, "%s\r\n1 mplayer_command ok\r\n", buffer) == -1) goto out;

			}

		} else {
			if (client_send(idx, "0 Invalid argument\r\n") == -1) goto out;
		}

	} else if (cmd == CMD_SEEK_PERCENT) {
		if (num_args > 1 + arg_offset) {

			// pthread_mutex_lock(lock);
			r = mplayer_seek_percent(mpx, cmd_args[1 + arg_offset]);
			// pthread_mutex_unlock(lock);

			if (r == -1) {
				if (client_send(idx, "0 No file loaded\r\n") == -1) goto out;
			} else {
				if (client_send(idx, "1 ok\r\n") == -1) goto out;
			}
		} else {
			if (client_send(idx, "0 Invalid argument\r\n") == -1) goto out;
		}

	} else if (cmd == CMD_SEEK_ABSOLUTE) {
		if (num_args > 1 + arg_offset) {

			// pthread_mutex_lock(lock);
			r = mplayer_seek_absolute(mpx, cmd_args[1 + arg_offset]);
			// pthread_mutex_unlock(lock);

			if (r == -1) {
				if (client_send(idx, "0 No file loaded\r\n") == -1) goto out;
			} else {
				if (client_send(idx, "1 ok\r\n") == -1) goto out;
			}
		} else {
			if (client_send(idx, "0 Invalid argument\r\n") == -1) goto out;
		}

	} else if (cmd == CMD_SEEK_RELATIVE) {
		if (num_args > 1 + arg_offset) {
			// pthread_mutex_lock(lock);
			r = mplayer_seek_relative(mpx, cmd_args[1 + arg_offset]);
			// pthread_mutex_unlock(lock);

			if (r == -1) {
				if (client_send(idx, "0 No file loaded\r\n") == -1) goto out;
			} else { 
				if (client_send(idx, "1 ok\r\n") == -1) goto out;
			}
		} else {
			if (client_send(idx, "0 Invalid argument\r\n") == -1) goto out;
		}

	} else if (cmd == CMD_OSD) {
		if (num_args > 1 + arg_offset) {
			// pthread_mutex_lock(lock);
			r = mplayer_osd(mpx, strtol(cmd_args[1 + arg_offset], NULL, 0));
			// pthread_mutex_unlock(lock);

			if (r == -1) {
				if (client_send(idx, "0 failed to set osd level\r\n") == -1) goto out;
			} else {
				if (client_send(idx, "1 ok\r\n") == -1) goto out;
			}
		} else {
			if (client_send(idx, "0 Invalid argument\r\n") == -1) goto out;
		}

	} else if (cmd == CMD_FULLSCREEN) {

		// pthread_mutex_lock(lock);
		r = mplayer_fullscreen(mpx);
		// pthread_mutex_unlock(lock);

		if (r == -1) {
			if (client_send(idx, "0 No file loaded\r\n") == -1) goto out;
		} else { 
			if (client_send(idx, "1 ok\r\n") == -1) goto out;
		}
			
	} else if (cmd == CMD_PAUSE) {

		// pthread_mutex_lock(lock);
		r = mplayer_pause(mpx);
		// pthread_mutex_unlock(lock);

		if (r == -1) {
			if (client_send(idx, "0 pause failed\r\n") == -1) goto out;
		} else {

			// pthread_mutex_lock(lock);
			if (mpx->status == MP_PLAYING) {

				if (client_send(idx, "1 Playing\r\n") == -1) {
					// pthread_mutex_unlock(lock);
					goto out;
				}

			} else if (mpx->status == MP_PAUSED) {
				if (client_send(idx, "1 Paused\r\n") == -1) {
					// pthread_mutex_unlock(lock);
					goto out;
				}

			} else {
				DBG("ERROR: Bad state in pause command (%d)\n", mpx->status);
			}
			// pthread_mutex_unlock(lock);
		}

	} else if (cmd == CMD_STOP) {
		// pthread_mutex_lock(lock);
		r = mplayer_quit(mpx);
		// pthread_mutex_unlock(lock);

		if (r == -1) {
			if (client_send(idx, "0 No file loaded\r\n") == -1) goto out;
		} else {
			if (client_send(idx, "1 ok\r\n") == -1) goto out;
		}

	} else if (cmd == CMD_LOAD) {

		if (num_args > 1 + arg_offset) {

			r = 1;
			if (strstr(cmd_args[1 + arg_offset], "://")) { /* lets say it's a URL */
				strcpy(new_path, cmd_args[1 + arg_offset]);

			} else {

				if (check_path(idx, cmd_args[1 + arg_offset], new_path) == -1) r = 0;


				/* try to act as qload command */
				if (!r) {
					r = strtol(cmd_args[1 + arg_offset], NULL, 0);
					DBG("Trying to act as qload, loading file #%d\n", r);
					sprintf(tmp_path, "%s/%s", config->root, clients[idx].cwd);
					p = load_dir_entry(tmp_path, r);
					if (p) {
						sprintf(new_path, "%s/%s/%s", config->root, clients[idx].cwd, p);
						xfree(p);
					} else {
						r = 0;
					}
				}
			}

			/* now that we got a file/url to load .. lets do it! */
			if (r) {

				// pthread_mutex_lock(lock);
				r = mplayer_load(mpx, new_path, inst);
				// pthread_mutex_unlock(lock);

				if (r == -1) {
					if (client_send(idx, "0 load error\r\n") == -1) goto out;
				} else {
					if (client_send(idx, "1 ok\r\n") == -1) goto out;
				}

			} else {

					if (client_send(idx, "0 File not found\r\n") == -1) goto out;
			}


		} else {
			if (client_send(idx, "0 Invalid argument\r\n") == -1) goto out;
		}

	} else if (cmd == CMD_PWD) {
		if (client_send(idx, clients[idx].cwd) == -1) goto out;
		if (client_send(idx, "\r\n") == -1) goto out;

	} else if (cmd == CMD_LS) {

		if (num_args == 1 + arg_offset) {

			sprintf(tmp_path, "%s/%s", config->root, clients[idx].cwd);
			strcpy(new_path, tmp_path);

		} else {

			if ( *cmd_args[1 + arg_offset] == '/') { //  absolute path

				sprintf(tmp_path, "%s/%s", config->root, cmd_args[1 + arg_offset]);

				l = strlen(config->root);
				if (realpath(tmp_path, new_path) && (strncmp(config->root, new_path, l) == 0)) 
					strcpy(tmp_path, new_path);
				else
					*tmp_path = 0;

			} else { // relative path

				l = strlen(cmd_args[1 + arg_offset]);

				tab_buf = (char *) xmalloc( l + 2 + strlen(clients[idx].cwd) );

				sprintf(tab_buf, "%s/%s", clients[idx].cwd, cmd_args[1 + arg_offset]);

				l = strlen(config->root);
				if (realpath(tab_buf, new_path) && (strncmp(config->root, new_path, l) == 0))
					strcpy(tmp_path, new_path);
				else
					*tmp_path = 0;

				xfree(tab_buf);
			}
		}

		ll_list = load_dir(tmp_path, 1024);
		if (ll_list) {
			i  = 1;
			for(ll_list->list = ll_list->head; ll_list->list; ll_list->list = ll_list->list->next) {
				struct stat fstat;

				sprintf(tmp_path, "%s/%s", new_path, ll_list->list->data);

				if (stat(tmp_path, &fstat) == -1) {
					DBG("stat failed for '%s'\n", tmp_path);
				}


				if (S_ISDIR(fstat.st_mode))
					strcpy(tmp, "/");
				else
					*tmp = 0;

				if (client_sendf(idx, "%d %s%s\r\n", i++, ll_list->list->data, tmp) == -1) {
					ll_free(ll_list);
					goto out;
				}
			}
			ll_free(ll_list);
		}



	} else if (cmd == CMD_CD) {

		if (num_args > 1 + arg_offset) {
			if ( *cmd_args[1 + arg_offset] == '/') {	// absolute path
				sprintf(tmp_path, "%s/%s", config->root, cmd_args[1 + arg_offset]);

			} else {			// relative
				sprintf(tmp_path, "%s/%s/%s", config->root, clients[idx].cwd, cmd_args[1 + arg_offset]);
			}

			if (realpath(tmp_path, new_path)) {

				if (isdir(new_path)) {

					l = strlen(config->root);
					if (strncmp(config->root, new_path, l) == 0) {
						if (strlen(new_path) == l) {
							strcpy(clients[idx].cwd, "/");
						} else {
							strcpy(clients[idx].cwd, new_path + l);
						}
					} else {
						if (client_send(idx, STR_NO_SUCH_FILE) == -1) goto out;
					}

				} else {
					if (client_send(idx, STR_NO_SUCH_FILE) == -1) goto out;
				}
			} else {
				if (client_send(idx, STR_NO_SUCH_FILE) == -1) goto out;
			}
		} else {
			strcpy(clients[idx].cwd, "/");
		}


	} else if (cmd == CMD_MUTE) {
		// pthread_mutex_lock(lock);
		r = mplayer_mute(mpx);
		// pthread_mutex_unlock(lock);

		if (r == -1) {
			if (client_send(idx, "0 mute failed\r\n") == -1) goto out;
		} else {
			if (client_send(idx, "1 mute ok\r\n") == -1) goto out;
		}

	} else if (cmd == CMD_WHO) {
		send_current_clients(idx);

	} else if (cmd == CMD_KILL) {
		if (num_args > 1 + arg_offset) {
			if (kill_client(strtol(cmd_args[1 + arg_offset], NULL, 0)) == -1) {
				if (client_send(idx, "0 kill failed\r\n") == -1) goto out;
			} else {
				if (client_send(idx, "1 session killed\r\n") == -1) goto out;
			}
		} else {
			if (client_send(idx, "0 Invalid argument\r\n") == -1) goto out;
		}

#ifdef COMMAND_HISTORY
	} else if (cmd == CMD_HISTORY) {

		for(i=0; i<HISTORY_SIZE; i++) {
			if (clients[idx].history.commands[i].command == NULL) break;
			sprintf(tmp, "%d ", i);
			if (client_send(idx, tmp) == -1) goto out;
			if (client_send(idx, clients[idx].history.commands[i].command) == -1) goto out;
			if (client_send(idx, "\r\n") == -1) goto out;
		}
#endif /* COMMAND_HISTORY */

	} else if (cmd == CMD_QLOAD) {

		if (num_args > 1 + arg_offset) {
			r = strtol(cmd_args[1 + arg_offset], NULL, 0);

			sprintf(tmp_path, "%s/%s", config->root, clients[idx].cwd);
			p = load_dir_entry(tmp_path, r);

			if (p) {
				sprintf(new_path, "%s/%s/%s", config->root, clients[idx].cwd, p);
				xfree(p);

				// pthread_mutex_lock(lock);
				r = mplayer_load(mpx, new_path, inst);
				// pthread_mutex_unlock(lock);

				if (r == -1) {
					if (client_send(idx, "0 load error\r\n") == -1) goto out;
				} else {
					if (client_send(idx, "1 ok\r\n") == -1) goto out;
				}
			} else {
				if (client_send(idx, "0 bad qload entry\r\n") == -1) goto out;
			}

		} else {
			if (client_send(idx, "0 Invalid argument\r\n") == -1) goto out;
		}

	} else if (cmd == CMD_CLEAR) {
		if (client_send(idx, "Not implemented\r\n") == -1) goto out;

	} else if (cmd == CMD_RESET) {

		// pthread_mutex_lock(lock);
		mplayer_unload(mpx);
		// pthread_mutex_unlock(lock);


#ifdef INSTANCE_SUPPORT
	} else if (cmd == CMD_INSTANCES) {

		i = 1;

		// pthread_mutex_lock(&instances_mutex);

		if (client_sendf(idx, "%d/%d instances\r\n", instance_count, config->max_instances) == -1) {
			// pthread_mutex_unlock(&instances_mutex);
			goto out;
		}

		for(
			mp_instances->list = mp_instances->head;
			mp_instances->list;
			mp_instances->list = mp_instances->list->next
		) {

			tmp_inst = (struct mp_instance_t *) mp_instances->list->data;

			// pthread_mutex_lock(&tmp_inst->lock);

			if (client_sendf(idx, "%d %s %s\r\n",
				tmp_inst->id,
				mplayer_status_str(tmp_inst->mpx),
				((tmp_inst->mpx->filename[0] == 0) ? "none" : tmp_inst->mpx->filename)) == -1) {

				// pthread_mutex_unlock(&tmp_inst->lock);
				// pthread_mutex_unlock(&instances_mutex);
				goto out;
			}

			// pthread_mutex_unlock(&tmp_inst->lock);

		}
		// pthread_mutex_unlock(&instances_mutex);

	} else if (cmd == CMD_NEW_INSTANCE) {

		if (instance_count < config->max_instances) {

			i = new_instance();
			if (i > 0) {
				if (client_sendf(idx, "1 new instance %d\r\n", i) == -1) goto out;
			} else {
				if (client_send(idx, "0 new instance failed\r\n") == -1) goto out;
			}

		} else {
			if (client_sendf(idx, "0 max instances limited (limit %d).\r\n", config->max_instances) == -1) goto out;
		}
	

	} else if (cmd == CMD_DEL_INSTANCE) {


		if (inst == 1) { /* can't delete the first instance */
			if (client_send(idx, "0 can't delete first instance\r\n") == -1) goto out;
		} else {

			// pthread_mutex_lock(&instances_mutex);
			tmp_inst = get_instance(inst);

			if (tmp_inst) {

				if (free_instance(inst) == 0) {
					if (client_sendf(idx, "1 deleted instance %d\r\n", inst) == -1) {
						// pthread_mutex_unlock(&instances_mutex);
						goto out;
					}
				} else {
					if (client_sendf(idx, "0 error deleting instance %d\r\n", inst) == -1) {
						// pthread_mutex_unlock(&instances_mutex);
						goto out;
					}
				}
				// pthread_mutex_unlock(&instances_mutex);

			} else {
				// pthread_mutex_unlock(&instances_mutex);
				if (client_sendf(idx, "0 invalid instance %d\r\n", inst) == -1) {
					goto out;
				}

			}
		}


#endif /* INSTANCE_SUPPORT */

	} else if (cmd == CMD_ARGUMENTS) {

		// pthread_mutex_lock(&instances_mutex);
	
		tmp_inst = get_instance(inst);

		if (tmp_inst) {
			if (num_args > 1 + arg_offset) {
				/* change arguments */
				if (tmp_inst->args) xfree(tmp_inst->args);
				tmp_inst->args = xstrdup(cmd_args[1 + arg_offset]);

				if (client_sendf(idx, "1 instance %d arguments set.\r\n", inst) == -1) {
					// pthread_mutex_unlock(&instances_mutex);
					goto out;
				}

				// pthread_mutex_unlock(&instances_mutex);
			} else {
				/* just print arguments */
				if (client_sendf(idx, "1 instance %d arguments \"%s\"\r\n", inst, (tmp_inst->args) ? tmp_inst->args : config->mplayer_flags) == -1) {
					// pthread_mutex_unlock(&instances_mutex);
					goto out;
				}
				// pthread_mutex_unlock(&instances_mutex);
			}

		} else {
			// pthread_mutex_unlock(&instances_mutex);
			if (client_sendf(idx, "0 invalid instance %d\r\n", inst) == -1) {
				goto out;
			}
		}

	} else if (cmd == CMD_DVD) {
		if (num_args > 1) {

				sprintf(tmp, "dvd://%ld", strtol(cmd_args[1], NULL, 0) );

				// pthread_mutex_lock(lock);
				r = mplayer_load(mpx, tmp, inst);
				// pthread_mutex_unlock(lock);

				if (r == -1) {
					if (client_send(idx, "0 load error\r\n") == -1) goto out;
				} else {
					if (client_send(idx, "1 ok\r\n") == -1) goto out;
				}

		} else {
				if (client_send(idx, "0 title required\r\n") == -1) goto out;
		}


	} else if (cmd == CMD_LENGTH) {
			// pthread_mutex_lock(lock);
			r = mplayer_command(mpx, "get_time_length");
			// pthread_mutex_unlock(lock);

			if (r == -1) {
				if (client_send(idx, "0 get_time_length failed\r\n") == -1) goto out;
			} else {
				if (client_sendf(idx, "1 ok\r\n", mpx->length) == -1) goto out;
			}
	
	} else if (cmd == CMD_ID3) {

		if (num_args > 1) {
			if (check_path(idx, cmd_args[arg_offset + 1], new_path) == -1) {
				if (client_send(idx, "0 id3 file not found\r\n") == -1) goto out;

			} else {
					
				r = cmd_do_id3(idx, new_path);
				if (r == -1) {
					if (client_send(idx, "0 id3 failed\r\n") == -1) goto out;
				}	
			}

		} else {

			if (client_send(idx, "0 id3 needs filename\r\n") == -1) goto out;

		}

	} else if (cmd == CMD_DIRDUMP) {

		if (num_args > 1) {

			if (check_path(idx, cmd_args[arg_offset + 1], new_path) == -1) {

				if (client_send(idx, "0 dirdump file not found\r\n") == -1) goto out;

			} else {
					
				r = cmd_do_dirdump(idx, new_path);
				if (r == -1) {
					if (client_send(idx, "0 dirdump failed\r\n") == -1) goto out;
				}	
			}

		} else {
				if (client_send(idx, "0 dirdump needs directory\r\n") == -1) goto out;
		}


	} else if (cmd == CMD_FILEDUMP) {

		if (num_args > 1) {

			if (check_path(idx, cmd_args[arg_offset + 1], new_path) == -1) {

				if (client_send(idx, "0 filedump file not found\r\n") == -1) goto out;

			} else {
					
				r = cmd_do_filedump(idx, new_path);
				if (r == -1) {
					if (client_send(idx, "0 filedump failed\r\n") == -1) goto out;
				}	
			}

		} else {
				if (client_send(idx, "0 filedump needs directory\r\n") == -1) goto out;
		}


	} else {
		printf("Unknown command '%s' (id %d)\n", clients[idx].buf, cmd);
	}


	for(i=0; i<num_args; i++) {
		xfree(cmd_args[i]);
	}
	return rc;

out:
	for(i=0; i<num_args; i++) {
		xfree(cmd_args[i]);
	}
	return -1;
}

/* parse arguments on a command line buffer (char *cmd)
 * sets how many arguments parsed in max (int *max).
 * parses no more than max (int *max) arguments as well
 */
int parse_arguments(char *cmd, char **args, int *max) {
	int i = 0;
	char *p, *q, *tmp;

	p = cmd;

	while (*p != '"' && *p != ' ' && *p != '\0') p++;
	if ((p - cmd) > 0) {
		tmp = (char *) xmalloc((p - cmd) + 1);
		strncpy(tmp, cmd, p - cmd);
		*(tmp + (p - cmd)) = '\0';
		args[i++] = tmp;
	}

	for( ; *p != '\0' ; p++) {

		if (*p == ' ') {
			while(*p == ' ' && *p != '\0') p++;

			q = p;
			while(*q != ' ' && *q != '"' && *q != '\0') q++;

			if ((q - p)  > 0) {
				tmp = (char *) xmalloc((q - p) + 1);
				strncpy(tmp, p, q - p);
				*(tmp + (q - p)) = '\0';

				args[i++] = tmp;
				if (i == *max) break;
			}
			while ((*p == ' ') && *p != '\0') p++;
		}

		if (*p == '"') {
			q = p++;

			while (*p != '"' && *p != '\0') p++;

			if ((p - q) > 0) {
				tmp = (char *) xmalloc((p - q));
				strncpy(tmp, q + 1, p - q - 1);
				*(tmp + (p - q - 1)) = '\0';

				args[i++] = tmp;
				if (i == *max) break;

			}
			while ((*p == ' ') && *p != '\0') p++;
		}

	}

	*max = i++;
	return 0;
}

/* gets a command id (index), position in the command_list
 * array
*/
int get_command_id(char *cmd, int len) {
	int i, x = -1;

	if (len == -1) len = strlen(cmd);
	if (len <= 0) return 0;

	DBG("get_command_id '%s' (%d)\n", cmd, len);

	for(i=0; command_list[i].cmd; i++) {
		if (strncmp(cmd, command_list[i].cmd, len) == 0) {
			x = i;
			break;
		}
	}

	return x;
}

int read_id3_tag(char *file, id3v1_tag *id3) {
	FILE *f;
	char	buf[32]; 

	DBG("read_id3_tag '%s' (%d)\n", file, SEEK_END);
	f = fopen(file, "r");
	if (!f) return -1;

	fseek(f, -128, SEEK_END);

	fread(buf, 3, 1, f);

	if (strcmp(buf, "TAG") != 0) return -1;

	fread(buf, 30, 1, f); // title
		strcpy(id3->title, buf);

	fread(buf, 30, 1, f); // artist
		strcpy(id3->artist, buf);

	fread(buf, 30, 1, f); // album
		strcpy(id3->album, buf);

	fclose(f);

	return 0;
}

int cmd_do_filedump(int idx, char *path) {
	int r = 0;

	DIR *d;
	struct dirent *dir_entry;
	struct stat st;
	char tmp_filename[PATH_MAX];

	d = opendir(path);
	if (!d) return -1;

	readdir(d);
	readdir(d);

	while ( (dir_entry = readdir(d)) ) {

		sprintf(tmp_filename, "%s/%s", path, dir_entry->d_name);
		if (stat(tmp_filename, &st) == -1) continue;
	
		if (S_ISREG(st.st_mode)) {
			if (client_sendf(idx, "%d %s/%s\r\n", st.st_mtime, path, dir_entry->d_name) == -1) goto filedump_out;
		}
	}

	closedir(d);
	return r;

filedump_out:
		closedir(d);
		return -1;
}

int cmd_do_dirdump(int idx, char *path) {
	int r = 0;

	DIR *d;
	struct dirent *dir_entry;
	struct stat st;
	char tmp_filename[PATH_MAX];

	d = opendir(path);
	if (!d) return -1;

	readdir(d);
	readdir(d);

	while ( (dir_entry = readdir(d)) ) {

		sprintf(tmp_filename, "%s/%s", path, dir_entry->d_name);
		if (stat(tmp_filename, &st) == -1) continue;
	
		if (S_ISDIR(st.st_mode)) {

			if (client_sendf(idx, "%d %s/%s\r\n", st.st_mtime, path, dir_entry->d_name) == -1) goto dirdump_out;

			cmd_do_dirdump(idx, tmp_filename);	
		}
	}

	closedir(d);
	return r;

dirdump_out:
		closedir(d);
		return -1;
}


int cmd_do_id3(int idx, char *file) {
	int r = 0;

	id3v1_tag id3_tag;


	r = read_id3_tag(file, &id3_tag);

	if (client_sendf(idx, "artist: %s\r\n", id3_tag.artist) == -1) return -1;
	if (client_sendf(idx, "album: %s\r\n", id3_tag.album) == -1) return -1;
	if (client_sendf(idx, "title: %s\r\n", id3_tag.title) == -1) return -1;

	return r;

}


/* checks a fiel path to see if it's first inside the root, a valid path
 * and stores the new valid path (if any) in new_path
 * returns -1 on error, 0 on success
*/
int check_path(int idx, char *file, char *new_path) {
	char tmp_path[PATH_MAX];
	int r = 0;

	if (*file == '/') {
		sprintf(tmp_path, "%s/%s", config->root, file);
	} else {
		sprintf(tmp_path, "%s/%s/%s", config->root, clients[idx].cwd, file);
	}

	DBG("check_path: tmp_path '%s'\n", tmp_path);

	if (realpath(tmp_path, new_path)) {
		DBG("check_path: new_path '%s'\n", new_path);
		if (strncmp(config->root, new_path, strlen(config->root))) r = -1;
	
	} else {
		r = -1;
	}

	return r;
}
