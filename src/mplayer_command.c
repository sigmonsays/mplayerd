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
#include "parse_config.h"
#include "xmemory.h"
#include "debug.h"
#include "mplayer_command.h"
#include "mplayer_slave.h"
#include "instance.h"

#include <pthread.h>

extern int logging;
extern mpd_config *config;
extern pthread_mutex_t instances_mutex;
extern struct ll_list *mp_instances;

char *mplayer_status_str(struct mp *mpx) {
	switch (mpx->status) {
		case MP_CLOSING:
		case MP_STOPPED:
			return "stopped";
			break;

		case MP_PAUSED:
			return "paused";
			break;

		case MP_PLAYING:
			return "playing";
			break;


		default:
			return "error";
			break;
	}
}

struct mp *mplayer_init(struct mp *mpx) {
	if (!mpx) mpx = (void*) xmalloc(sizeof(struct mp));

	mpx->filename[0] = '\0';
	mpx->readfd = 0;
	mpx->writefd = 0;

	mpx->status = MP_STOPPED;

	mpx->audio_sec = 0;
	mpx->video_sec = 0;

	mpx->current_frame = 0;

	mpx->length = -1;

	return mpx;
}

void mplayer_message_thread() {
	struct timeval timeout;

	struct mp_instance_t *mp_inst;
	struct mp *mpx;

	pthread_mutex_t *lock;

	debug_msg(2, "mplayer_message_thread id %ld pid %d\n", pthread_self(), getpid());

	for( ;; ) {

		pthread_mutex_lock(&instances_mutex);

		for(
			mp_instances->list = mp_instances->head;
			mp_instances->list;
			mp_instances->list = mp_instances->list->next
		) {

			mp_inst = (struct mp_instance_t *) mp_instances->list->data;

			mpx = mp_inst->mpx;
			lock = &mp_inst->lock;

			pthread_mutex_lock(lock);

			if (mpx->status == MP_PLAYING) {
				if (mplayer_message(mp_inst) == -1) {
					mplayer_unload(mpx);
				}
			}

			timeout.tv_sec = 0;
			timeout.tv_usec = 500;
			select(0, NULL, NULL, NULL, &timeout);

			pthread_mutex_unlock(lock);

		}
		pthread_mutex_unlock(&instances_mutex);
	}

	pthread_exit(NULL);
}

void mplayer_load_thread(struct mp_load_thread_tt *mp_load_thread_args) {
	char *file, *bname, *p;
	struct mp *mpx;
	int r, pid, l, i;
	int fdin[2], fdout[2];
	char *mplayer_args[32], *args_p;

#ifdef INSTANCE_SUPPORT
	int instance;
#endif

	struct mp_instance_t *mp_inst;
	pthread_mutex_t *lock;

	mpx = mp_load_thread_args->mpx;
	file = mp_load_thread_args->file;

	char *toker;

#ifdef INSTANCE_SUPPORT
	instance = mp_load_thread_args->instance;
	pthread_mutex_lock(&instances_mutex);
	mp_inst = get_instance(instance);
	pthread_mutex_unlock(&instances_mutex);
#else
	pthread_mutex_lock(&instances_mutex);
	mp_inst = get_instance(1);
	pthread_mutex_unlock(&instances_mutex);

#endif /* INSTANCE_SUPPORT */

	if (!mp_inst) pthread_exit(NULL);

	lock = &mp_inst->lock;

	debug_msg(2, "mplayer_load_thread id %ld pid %d\n", pthread_self(), getpid());

	pthread_mutex_lock(lock);


	bname = basename(file);

	if (pipe(fdin) != 0) goto out;
	if (pipe(fdout) != 0) goto out;

	l = strlen(bname);
	strncpy(mpx->filename, bname, l);
	mpx->filename[l] = '\0';

	mpx->length = -1;

	mpx->status = MP_PLAYING;
	mpx->readfd = fdout[0];
	mpx->writefd = fdin[1];

	pthread_mutex_unlock(lock);

	pid = fork();

	if (pid == -1) goto out;

	if (pid == 0) { /* chid process */

		close(fdin[1]);
		dup2(fdin[0], 0);

		close(fdout[0]);
		dup2(fdout[1], 1);

		// we don't want stderr printed in the daemon console
		close(2);

		if (mp_inst->args) {
			args_p = mp_inst->args;
		} else {
			args_p = config->mplayer_flags;
		}

		mplayer_args[0] = "mplayer";
		mplayer_args[1] = "-slave";
		mplayer_args[2] = "-vo";
		mplayer_args[3] = config->mplayer_vo;
		p = strtok_r(args_p, " ", &toker);
		i = 4;
		if (p) {
			do {
				mplayer_args[i] = xstrdup(p);
				if (i++ > 30) break;
			} while ((p = strtok_r(NULL, " ", &toker)));
		}
		mplayer_args[i++] = xstrdup(file);
		mplayer_args[i] = NULL;

		r = execvp(config->mplayer_path, mplayer_args);
	}

	if (pid) {      /* parent */

		wait(NULL);
		close(fdin[0]);
		close(fdout[1]);

		pthread_mutex_lock(lock);	
		mpx->status = MP_PLAYING;
		pthread_mutex_unlock(lock);	

		debug_msg(2, "pthread: mplayer_load_thread() exiting...\n");
		xfree(mp_load_thread_args->file);
		xfree(mp_load_thread_args);
		pthread_exit(NULL);
	}

out:
	pthread_exit(NULL);
}


/* read from pipe and get/set 
	returns -1 on error, 0 on success
*/
int mplayer_message(struct mp_instance_t *mp_inst) {
	int r;
	char buf[1024];
	char *p;
	struct mp *mpx;
	pthread_mutex_t *lock;

	char *toker;

	mpx = mp_inst->mpx;

	lock = &mp_inst->lock;

	if (mpx->readfd == 0) {
		return -1;
	}

	r = read(mpx->readfd, buf, 4096);
	if (r <= 0) {
		return -1;
	}

	buf[r] = '\0';

	p = strtok_r(buf, "\r\n", &toker);
	if (p) {
		while (p) {

			// DBG("mplayer says '%s'\n", p);
			if (0) {

/*
			} else if (strcmp(p, "================= PAUSED =================") == 0) {
				mpx->status = MP_PAUSED;

			} else if (strcmp(p, "  =====  PAUSE  =====") == 0) {
				mpx->status = MP_PAUSED;
*/

			} else if (strncmp(p, "Exiting...", 10) == 0) {
				return -1;

			} else if (strncmp(p, "ANS_LENGTH=", 11) == 0) {
				mpx->length = strtol(p + 11, NULL, 0);
				DBG("song/movie length %d\n", mpx->length);

			} else if ( (strlen(p) > 2) && (strncmp(p, "A:", 2) == 0) ) {
				get_mplayer_av_info(p, mpx);

			} else {
				printf("mplayer %d `%s'\n", mp_inst->id, p);
			}

			p = strtok_r(NULL, "\r\n", &toker);
		}
	}

	return 0;
}

void get_mplayer_av_info(char *buf, struct mp *mpx) {
	int l;
	float audio_sec, video_sec, av_diff, ct;


	char time1_str[32];

	int step = 0;
	int current_frame;

	/* try various methods for different versions of mplayer, perhaps this should
	 * be config'ed in mplayerd.conf?
	*/
	while (1) {
		
		mpx->audio_sec = 0;
		mpx->video_sec = 0;
		mpx->current_frame = 0;
		
		/* first we'll assume audio,
		 * this block is going to need serious updating over time
		 * if mplayer devs decide to change it's A: output
		 * */
		if (step == 0) {

			l = sscanf(buf, "A: %f (%[0-9.:]) ", &audio_sec, time1_str);

			if (l != 2) {
				step++;
				continue;
			}

			mpx->audio_sec = audio_sec;
			break;

		} else if (step == 1) { /* perhaps a video file */

			l = sscanf(buf, "A: %f V: %f A-V: %f ct: %f %i",
				&audio_sec, &video_sec, &av_diff, &ct, &current_frame);

			if (l != 5) {
				step++;
				continue;
			}

			mpx->audio_sec = audio_sec;
			mpx->video_sec = video_sec;
			mpx->current_frame = current_frame;
			break;
		
		} else {
			printf("get_mplayer_av_info: I don't know how to handle '%s'\n", buf);
			break;
		}
	}
}

/*
 * converts a time in hh:mm:ss / mm:ss / ss to seconds.
 * seconds can be a float value
 * returns -1 on error
 */ 
int time_to_seconds(char *buf) {
	int i = 0,l;
	int hh, mm;
	float ss;

	int seconds = -1;

	int colons = 0;

	do {
		if (buf[i] == ':') colons++;
	} while (buf[++i]);

	if (colons == 2) { /* it's hh:mm:ss */
		l = sscanf(buf, "%d:%d:%f", &hh, &mm, &ss);
		if (l != 3) return -1;
		
		seconds = (hh * 60) + (mm * 60) + (int) ss;

	} else if (colons == 1) { /* it's mm:ss */
		l = sscanf(buf, "%d:%f", &mm, &ss);
		if (l != 2) return -1;

		seconds = (mm * 60) + (int) ss;

	} else { /* it's only seconds */
		seconds = strtol(buf, NULL, 0);
	}

	return seconds;
}
