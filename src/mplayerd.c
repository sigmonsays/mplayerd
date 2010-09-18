/*
 * mplayerd
 *
 * Website: http://mplayerd.sourceforge.net/
 * Secondary Website: http://signuts.net/projects/id/59
 *
 * http://www.signuts.net/
 * exonic@signuts.net
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
#include "mplayer_command.h"
#include "client.h"
#include "client_request.h"
#include "parse_config.h"
#include "debug.h"
#include "xmemory.h"
#include "instance.h"

int debug = 0;
char *logfile;
int logging = 0, logging_fd;
int doFork = 1;
client_list clients[MAX_CLIENTS];

mpd_config *config = NULL;

char config_file[1024];

pthread_t main_pthread;
pthread_t mplayer_message_pthread;

/* pointer to current instance */
int instance_count = 0;


struct ll_list *mp_instances = NULL;

pthread_mutex_t instances_mutex = PTHREAD_MUTEX_INITIALIZER;

extern cmd_list command_list[];
int listen_sock;

int main(int argc, char *argv[]) {
	int new_client, i, clinum;
	struct sockaddr_in cl_addr;
	int cl_addrlen;
	int pid;
	char *p;

	init_clients();
	cl_addrlen = sizeof(cl_addr);

#ifdef DEBUG_MEMORY
	ll_mem_init();
#endif

	main_pthread = pthread_self();

	get_config_file_name(argc, argv, config_file);

	/* parse config file -- command line options override config directives */
	if ((config = parse_config(config_file)) == NULL) {
		printf("Couldn't parse config file '%s'! Exiting...\n", config_file);
		exit(1);
	}

	if (parse_argv(argc, argv) == -1) exit(1);

	if (config->default_home[0] == 0) {
		p = getcwd(NULL, 0);
		strcpy(config->default_home, p);
		xfree(p);
	}

	if (strcmp(config->default_home, "$HOME") == 0) {
		strcpy(config->default_home, getenv("HOME"));
	}

	if (logging) {
		logging_fd = open(logfile, O_WRONLY|O_APPEND|O_CREAT, S_IRWXU);
		if (logging_fd == -1) {
			printf("WARNING: Logging disabled!\n");
			logging = 0;
		}
	}

	if ( (listen_sock = server_setup()) == -1) exit(1);

	mp_instances = ll_new();
	if (!mp_instances) {
		printf("mplayer instance couldn't be init'd... failed! Exiting..\n");
		exit(1);
	}

	new_instance();

	if (doFork) {   
		printf("forking into background...\n");

		pid = fork();
		if (pid == -1) {
			perror("fork");
			exit(1);
		}

		if (pid) {
			exit(0);
		}
	}

	signal(SIGINT, (void *) mplayerd_quit);
	signal(SIGTERM, (void *) mplayerd_quit);
	signal(SIGPIPE, SIG_IGN);


	if (pthread_create(&mplayer_message_pthread, NULL, (void *) mplayer_message_thread, NULL)) return -1;

	debug_msg(0, "mplayerd %s started on port %d with pid %d\n", MPLAYERD_VERSION, config->port, getpid());
	debug_msg(1, "main thread id %ld\n", main_pthread);


	for( ;; ) {

		new_client = accept(listen_sock, (struct sockaddr *)&cl_addr, &cl_addrlen);

		debug_msg(0, "New connection from %s\n", (char *)inet_ntoa(cl_addr.sin_addr));

		if (new_client > 0) {

			debug_msg(0, "%s : connected.\n", (char *)inet_ntoa(cl_addr.sin_addr));

			/* make sure they're allowed to connect */
			if (check_ip((char *)inet_ntoa(cl_addr.sin_addr), config->allow_ips) == 0) {
				shutdown(new_client, 2);
				close(new_client);
				debug_msg(0, "%s : Access denied.\n", (char *)inet_ntoa(cl_addr.sin_addr));

			} else {

				clinum = -1;
				for(i=0; i<MAX_CLIENTS; i++) {
					if (clients[i].socket == -1) {
						clinum = i;
						break;
					}
				}

				if (clinum == -1) {
					debug_msg(0, "%s : No more connections allowed. disconnected.\n", (char *)inet_ntoa(cl_addr.sin_addr));

					send(new_client, "Sorry. No more connections allowed!\n", 37, MSG_NOSIGNAL);
					shutdown(new_client, 2);
					close(new_client);

				} else {

					clients[i].id = clinum;
					clients[i].socket = new_client;
					clients[i].ip = xstrdup((char *)inet_ntoa(cl_addr.sin_addr));
					strcpy(clients[i].cwd, config->default_home);

					if (pthread_create(&clients[i].pthread, NULL, (void *)client_thread, (void *) &clients[i])) {
						debug_msg(0, "%s : pthread_created(...) failed!\n", clients[i].ip);
						shutdown(new_client, 2);
						close(new_client);
					} else {
						debug_msg(0, "%s : Assigned to slot %d.\n", clients[i].ip, i);
					}
				}

			}


		}

	}

	/* never reached */
	exit(0);
}

void client_thread(client_list *client) {
	int r;

	DBG("client_thread(): client %d, thread id %ld\n", client->id, pthread_self());

	if (client_send(client->id, TELNET_WILL_ECHO) == -1) goto out;
	if (client_send(client->id, TELNET_WILL_SUPRESS_GO_AHEAD) == -1) goto out;
	if (client_send(client->id, client->cwd) == -1) goto out;
	if (client_send(client->id, " >") == -1) goto out;
	if (client_send(client->id, OUTPUT_DONE) == -1) goto out;

	for( ;; ) {
		r = client_request(client->id);

		if (r == -1) {
			debug_msg(0, "%s (%d) : disconnected.\n", client->ip, client->id);
			break;
		}

		if (r == -2) {
			debug_msg(0, "%s (%d) : client says shutdown!\n", client->ip, client->id);
			mplayerd_quit();
		}
	}


out:
	kill_client(client->id);
	pthread_exit(NULL);
}


void mplayerd_quit() {
	int err;
	int i;

	debug_msg(0, "mplayerd shutting down...\n");
	DBG("mplayerd_quit, in thread %ld\n", pthread_self());

	err = pthread_cancel(mplayer_message_pthread);

	for(i=0; i<MAX_CLIENTS; i++) {
		if (clients[i].socket <= 0) continue;
		err = client_send(i, "\r\nserver shutdown\r\n" );
		DBG("closing clients[%d], result %d\n", i, err);
		kill_client(i);
	}


//	free_instances();

	if (config->allow_ips) ll_free(config->allow_ips);
	xfree(config);

#ifdef DEBUG_MEMORY
	ll_mem_print_statistics();
#endif
	err = pthread_cancel(main_pthread);

	pthread_join(mplayer_message_pthread, NULL);
	pthread_join(main_pthread, NULL);

	DBG("Done, exiting...\n");
	exit(0);
}

char *mplayerd_version() {      
        char *buf = (void*) xmalloc(32);
        sprintf(buf, "mplayerd %s", MPLAYERD_VERSION);
        return buf;                             
}                                        
                                        
void mplayerd_help() {
	char *p;
	p = mplayerd_version();

        printf( "%s \n"                 
                "daemon to control mplayer via TCP/IP\n\n"
                "-h --help             Print this help message\n"
                "-fg --foreground      Don't fork\n"
                "-p --port port        Specify port, default %d\n"
                "-l --logfile file     Log to file default off\n"
                "-v --version          display version\n"
		"-d --debug            debug, can specify multiple times\n"
		"-c --config           specify config file\n"
		"\n"
		"You can print the entire help for commands by using flags '-d -h'\n"
		, p, DEFAULT_PORT);

	if (debug) print_command_help();

	xfree(p);
}


int parse_argv(int argc, char **argv) {
	int i;
	char *p;

	for(i=0; i<argc; i++) {

		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			mplayerd_help();
			return -1;

		} else if (!strcmp(argv[i], "-fg") || !strcmp(argv[i], "--foreground")) {
			doFork = 0;

		} else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port")) {
			if (argv[i + 1]) {
				config->port = strtol(argv[i + 1], NULL, 0);
			} else {
				printf("port parameter required\n");
				return -1;
			}

		} else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--logfile")) {
			logging = 1;
			if (argv[i + 1]) {
				logfile = xstrdup(argv[i + 1]);
			} else {
				printf("logfile parameter required\n");
				return -1;
			}

		} else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
			p = mplayerd_version();
			printf("%s\n", p);
			xfree(p);
			return -1;

		} else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")) {
			debug++;

		} else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--config")) {
			if (argv[i + 1]) {
				strcpy(config_file, argv[i + 1]);
			} else {
				printf("config file parameter required\n");
				return -1;
			}
		}
	}
	return 0;
}

int server_setup() {
	int sock;
	struct sockaddr_in my_addr;
	int my_addrlen, yes = 1;


	memset(&my_addr, 0, sizeof(my_addr));

	my_addr.sin_family = AF_INET;
/*	my_addr.sin_addr.s_addr = INADDR_ANY; */
	my_addr.sin_addr.s_addr = inet_addr(BIND_ADDR);
	my_addr.sin_port = htons(config->port);

	my_addrlen = sizeof(my_addr);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return -1;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		perror ("setsockopt: SO_REUSEADDR: ");
		return -1;
	}


	if (bind(sock, (struct sockaddr*) &my_addr, my_addrlen) == -1) { /* Must have this on server */
		perror("bind");
		return -1;
	}

	if (listen(sock, 10) == -1) {
		perror("listen");
		return -1;
	}
	return sock;
}

int get_config_file_name(int argc, char **argv, char *file) {
	int i;  
	memset(file, 0, 1024);
	for(i=0; i<argc; i++) { 
		if (strncmp("-c", argv[i], 2) == 0 || strncmp("--config", argv[i], 8) == 0) {
			if (argv[i + 1]) strcpy(file, argv[i + 1]);
			return 0;
		}
	}
	strcpy(file, DEFAULT_CONFIG_FILE);
	return 0;
}

void print_command_help() {
	int i2 = 0, i = 0, j = 0;
	cmd_list *cmdl;
	char *p;

	printf("\n\nmplayerd commands\n"
	           "-----------------\n");

	while (1) {
		cmdl = (cmd_list *) &command_list[i2++];
		if (cmdl->cmd == NULL) break;

		printf("Syntax: %s", cmdl->cmd);

		p = cmdl->args;
		for(i=0; *(p + i); i++) {

			if (*(p + i) == '*') {
				printf(" [");
				j = 1;
				continue;
			}

			if (*(p + i) == 's') {
				printf(" $string");

			} else if (*(p + i) == 'f') {
				printf(" $file");

			} else if (*(p + i) == 'd') {
				printf(" $directory");

			} else if (*(p + i) == 'i') {
				printf(" $integer");

			} else if (*(p + i) == 'n') {
				printf(" $instance");

			} else {
				printf(" $X");
			}

			if (j) {
				printf(" ]");
				j = 0;
			}
		}
		printf("\nDescription: %s\n", cmdl->description);
		printf("%s\n\n", cmdl->help);
	}
}
