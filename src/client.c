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
#include "client.h"
#include "xmemory.h"
#include "mplayerd.h"

extern client_list clients[MAX_CLIENTS];

/*
	send current logins to client index
	returns -1 on error, 0 on success
*/
int send_current_clients(int index) {
	int i,t = 0;
	socklen_t addr_len, l;
	char buf[32];

	l = addr_len = sizeof(struct sockaddr_in);

	if (index < 0) return -1;

	if (client_send(index, "ID\t\tIP\t\t\tCWD\r\n") == -1) goto out;

	for(i=0; i<MAX_CLIENTS; i++) {
		addr_len = l;

		if (clients[i].socket > 0) {
			t++;

			sprintf(buf, "%d\t\t", i);
			if (client_send(index, buf) == -1) goto out;

			if (clients[i].ip) {
				if (client_send(index, clients[i].ip) == -1) goto out;
			} else {
				if (client_send(index, "0.0.0.0") == -1) goto out;
			}
			if (client_send(index, "\t\t") == -1) goto out;
			if (client_send(index, clients[i].cwd) == -1) goto out;

			if (client_send(index, "\r\n") == -1) goto out;
		}
	}

	sprintf(buf, "%d/%d clients connected\r\n", t, MAX_CLIENTS);
	if (client_send(index, buf) == -1) goto out;
	return 0;
out:
	return -1;
}



/*
	kill client
	returns -1 on error, 0 on success
*/
int kill_client(int index) {
	int i;

	if (index < 0) return -1;
	if (index > MAX_CLIENTS) return -1;

	DBG("kill_client(%d)\n", index);

	if (clients[index].socket > 0) {
		shutdown(clients[index].socket, 2);
		close(clients[index].socket);
	}
	clients[index].socket = -1;
	clients[index].buf[0] = '\0';
	clients[index].cwd[0] = '\0';

	for(i=0; i<HISTORY_SIZE; i++) {
		clients[index].history.commands[i].command[0] = 0;
	}

	clients[index].history.count = 0;
	clients[index].history.position = 0;

	xfree(clients[index].ip);
	clients[index].ip = NULL;

	clients[index].mode = CL_NORMAL_MODE;
	clients[index].hbuf[0] = '\0';
	clients[index].instance = 1;
	clients[index].cmd = -1;

	pthread_cancel(clients[index].pthread);

	pthread_join(clients[index].pthread, NULL);

	return 0;
}


/* send data on client index
	0 on success
	-1 on error
*/
int client_send(int index, char *msg) {
	int r;
	if (index < 0) return -1;
	if (index > MAX_CLIENTS) return -1;

	if (!msg) {
		DBG("client_send(..) called with NULL message.\n");
		return -1;
	}

	if (clients[index].socket <= 0) return -1;

	// DBG("client_send: client %d, '%s' strlen %d\n", index, msg, strlen(msg));

	r = send(clients[index].socket, msg, strlen(msg), 0);
	return (r == -1) ? -1 : 0;
}

/* deals with history search
	0 on success
	-1 on error
*/
int client_history_search(int index, char *buf, int buflen) {
	int i, l, t;
	char tmp[2];

	for(i=0; i<buflen; i++) {

		if (buf[i] >= '0' && buf[i] <= '9') {
			l = strlen(clients[index].hbuf);

			if (l < HISTORY_SIZE) {
				sprintf(tmp, "%c", buf[i]);
				strcat(clients[index].hbuf, tmp);
				if (client_send(index, tmp) == -1) goto out;
			}
		}

		if (buf[i] == '\r') {
			if (client_send(index, "\r") == -1) goto out;
			if (client_send(index, clients[index].cwd) == -1) goto out;
			if (client_send(index, " > ") == -1) goto out;
			clients[index].mode = CL_NORMAL_MODE;

			t = (int ) strtol(clients[index].hbuf, NULL, 0);

			if (t > HISTORY_SIZE) break;

			if (clients[index].history.commands[t].command != NULL) {
				if (client_send(index, clients[index].history.commands[t].command) == -1) goto out;
				strcat(clients[index].buf, clients[index].history.commands[t].command);
			}
			break;
		}
	}
	return 0;
out:
	return -1;
}

void init_clients() {
        int i,r;
        for(r=0; r<MAX_CLIENTS; r++) {
                clients[r].socket = -1;

                clients[r].buf[0] = '\0';

                clients[r].cwd[0] = '\0';
        
                clients[r].history.count = 0;
                clients[r].history.position = 0;
                for(i=0; i<HISTORY_SIZE; i++) {
                        clients[r].history.commands[i].command[0] = 0;
                        clients[r].history.commands[i].length = 0;
                }
                clients[r].mode = CL_NORMAL_MODE; 
                clients[r].hbuf[0] = '\0';
		clients[r].instance = 1;
		clients[r].cmd = -1;
        }
}


/*
        checks ip to see if it's in list
        returns 1 if it is, 0 if not, -1 on error
        check_ip(char *ip, struct ll_list *list)
*/
int check_ip(char *ip, struct ll_list *list) {
        int l;

        if (list == NULL) return -1;
        if (list->head == NULL) return -1;

        if (strlen(ip) == 0) return -1;

        for(list->list = list->head; list->list; list->list = list->list->next) {
                l = strlen(list->list->data);
                if (strncmp(list->list->data, ip, l) == 0) return 1;
        }
        return 0;
}

/* send data to client using printf style arguments
	returns 0 on success, -1 on error
*/
int client_sendf(int idx, const char *fmt, ...) {
	/* Guess we need no more than 100 bytes. */
	int n, size = 100, buf_len, r;
	char *p;
	va_list ap;
	if ((p = xmalloc (size)) == NULL) return -1;

	while (1) {
		va_start(ap, fmt);
		n = vsnprintf (p, size, fmt, ap);
		va_end(ap);

		if (n > -1 && n < size) break;

		if (n > -1) {
			size =  n + 1;
		} else {
			size *= 2;
		}

		if ((p = realloc (p, size)) == NULL) return -1;
	}

	buf_len = strlen(p);

	r = client_send(idx, p);

	xfree(p);
	return r;
}

