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
#include "debug.h"
#include "xmemory.h"

extern int debug;
extern int logging;
extern int logging_fd;

void debug_msg(const int level, const char *fmt, ...) {

	/* Guess we need no more than 100 bytes. */
	int n, size = 100, buf_len;
	char *p;
	va_list ap;
	if ((p = xmalloc (size)) == NULL) return;

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

		if ((p = realloc (p, size)) == NULL) return;
	}

	if (debug >= level) {
		if (logging) {
			buf_len = strlen(p);
			write(logging_fd, p, buf_len);
		}

		if (debug) printf("DEBUG%d: ", level);
		printf("%s", p);
		fflush(stdout);
	}
	xfree(p);
}

