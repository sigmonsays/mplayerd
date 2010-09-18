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
#include "mplayer_slave.h"
#include "instance.h"

extern int logging;
extern mpd_config *config;
extern pthread_mutex_t instances_mutex;
extern struct ll_list *mp_instances;


/* OSD level 0 through 2 */
int mplayer_osd(struct mp *mpx, int level) {
	char cmd[16];

	if (level > 2) return -1;

	if (mpx->status == MP_PAUSED) {
		if (mplayer_pause(mpx) == -1) return -1;
	}


	sprintf(cmd, "osd %d", level);
	return (mplayer_command(mpx, cmd));
}


/* send mplayer a command

	returns -1 on error, 0 on success

*/
int mplayer_command(struct mp *mpx, char *cmd) {
	int l, r;
	char c[MAX_COMMAND_LENGTH];

	if (mpx->readfd == 0 || mpx->writefd == 0) return -1;

	l = strlen(cmd);

	strcpy(c, cmd);
	strcat(c, "\n");

	r = write(mpx->writefd, c, l + 1);

	if (r == -1) {
		xfree(c);
		mplayer_unload(mpx);
		return -1;
	}

	if ((r - 1) != l) {
		debug_msg(2, "mplayer_command() : error writing to pipe\n");
	}

	debug_msg(2, "mplayer_command: cmd = '%s', r = %d, l = %d\n", cmd, r, l);

	return 0;
}


/*	seek <value> [type=<0/1/2>]
	Seek to some place in the movie.  Type 0 is a relative seek of +/- <value> seconds.  Type 1 seek to <value> % in the movie.  Type 2
	is a seek to an absolute position of <value> seconds. */
int mplayer_seek_percent(struct mp *mpx, char *p) {
	int l, r;
	char *cmd;

	l = strlen(p);

	cmd = (char *) xmalloc(l + 16);
	if (cmd == NULL) return -1;

	if (mpx->status == MP_PAUSED) {
		if (mplayer_pause(mpx) == -1) {
			xfree(cmd);
			return -1;
		}
	}


	sprintf(cmd, "seek %s 1", p);
	r = mplayer_command(mpx, cmd);
	xfree(cmd);
	return r;
}

int mplayer_seek_absolute(struct mp *mpx, char *s) {
	int l, r;
	char *cmd;

	l = strlen(s);

	cmd = (char *) xmalloc(l + 16);
	if (cmd == NULL) return -1;

	if (mpx->status == MP_PAUSED) {
		if (mplayer_pause(mpx) == -1) {
			xfree(cmd);
			return -1;
		}
	}

	sprintf(cmd, "seek %s 2", s);
	r = mplayer_command(mpx, cmd);
	xfree(cmd);
	return r;
}

int mplayer_seek_relative(struct mp *mpx, char *s) {
	int l, r;
	char *cmd;

	l = strlen(s);

	cmd = (char *) malloc(l + 16);
	if (cmd == NULL) return -1;

	if (mpx->status == MP_PAUSED) {
		if (mplayer_pause(mpx) == -1) {
			xfree(cmd);
			return -1;
		}
	}

	sprintf(cmd, "seek %s 0", s);
	r = mplayer_command(mpx, cmd);
	xfree(cmd);
	return r;
}


/* continue/pause playback
	alias to mplayer_pause
*/
int mplayer_play(struct mp *mpx) {
	return mplayer_pause(mpx);
}


/* continue/pause playback
	returns 0 on success, -1 on error
*/
int mplayer_pause(struct mp *mpx) {
	int r;

	if (mpx->status == MP_STOPPED) return -1;

	r = mplayer_command(mpx, "pause");

	if (mpx->status == MP_PLAYING) {
		mpx->status = MP_PAUSED;
	} else if (mpx->status == MP_PAUSED) {
		mpx->status = MP_PLAYING;
	} else {
		DBG("mplayer_pause expects status to be %d or %d, it's %d\n", MP_PLAYING, MP_PAUSED, mpx->status);
	}
	return r;
}


/* quit
	sends mplayer quit telling it to DIE!
	returns -1 on error, 0 on success
*/
int mplayer_quit(struct mp *mpx) {
	int r;

	if (mpx->status == MP_PAUSED) {
		DBG("mplayer is paused, unpausing to stop\n");
		if (mplayer_pause(mpx) == -1) return -1;
	}

	r = mplayer_command(mpx, "quit");
	mplayer_unload(mpx);
	return r;
}


/* load a file/url                      
        returns -1 on error, 0 on success
*/
int mplayer_load(struct mp *mpx, char *file, int instance) {
	pthread_t mplayer_load_pthread;
	struct mp_load_thread_tt *mp_load_thread_args;

	struct stat fstat;

	if (mpx->status != MP_STOPPED) return -1;

	if (file == NULL) return -1;
	if (file[0] == '\0')  return -1;

	/* we will skip the file check if there is a protocol specified */
	if (!strstr(file, "://")) {
		if (stat(file, &fstat) == -1) return -1;
		if (!S_ISREG(fstat.st_mode)) return -1;
	}

	if (! (mp_load_thread_args = malloc(sizeof(struct mp_load_thread_tt))) ) return -1;

	mpx->readfd = 0;
	mpx->writefd = 0;

	mp_load_thread_args->file = xstrdup(file);
	mp_load_thread_args->mpx = mpx;
	mp_load_thread_args->instance = instance;

	if (pthread_create(&mplayer_load_pthread, NULL, (void *) mplayer_load_thread, (void *) mp_load_thread_args)) return -1;

	return 0;
}

/* unload/unset variables and stuff
	convenience function
	returns 0 -- always
*/
int mplayer_unload(struct mp *mpx) {
	mpx->filename[0] = '\0';

	if (mpx->readfd) { 
		shutdown(mpx->readfd, SHUT_RDWR);
		close(mpx->readfd);
	}
	if (mpx->writefd) {
		shutdown(mpx->writefd,  SHUT_RDWR);
		close(mpx->writefd);
	}

	mpx->readfd = 0;
	mpx->writefd = 0;
	mpx->status = MP_STOPPED;

	return 0;
}


/* free mplayer data
	0 on success, -1 on error
*/
int mplayer_free(struct mp *mpx) {
	if (mpx == NULL) return -1;

	xfree(mpx);
	mpx = NULL;

	return 0;
}

/* toggle mute
	returns -1 on error, 0 on success
*/
int mplayer_mute(struct mp *mpx) {
	int r;
	if (mpx->status == MP_PAUSED) {
		if (mplayer_pause(mpx) == -1) return -1;
	}

	r = mplayer_command(mpx, "mute");
	return r;
}


/* volume <dir>        Increase/decrease volume 
	down = 0
	up = non-zero
	returns -1 on error, 0 on success
*/
int mplayer_volume(struct mp *mpx, int direction) {
	int r = -1;

	if (mpx->status == MP_PAUSED) {
		if (mplayer_pause(mpx) == -1) return -1;
	}

	if (direction == 0) {
		r = mplayer_command(mpx, "volume 0");
	} else {
		r = mplayer_command(mpx, "volume 100");
	}
	return r;
}

/* vo_fullscreen
	Switch to fullscreen mode.
	returns -1 on error, 0 on success
*/
int mplayer_fullscreen(struct mp *mpx) {
	int r;

	if (mpx->status == MP_PAUSED) {
		if (mplayer_pause(mpx) == -1) return -1;
	}

	r = mplayer_command(mpx, "vo_fullscreen");
	return r;
}
