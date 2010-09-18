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
#include "history.h"
#include "xmemory.h"
#include "dbg.h"

/*
	adds a command (char *s) to the history structure
	returns the new history count, -1 on error
*/
int add_history(cmd_hist *history, char *s) {
	int i, l;

	cmd_hist_command *cmd;

	l = strlen(s);


	i = (history->count % HISTORY_SIZE);

	DBG("Adding '%s' to command history (position %d)\n", s, i);

	cmd = &history->commands[i];

	strcpy(cmd->command, s);

	history->position = i;
	history->count++;

	return history->count;
}
/*
 * this function iterates over the history and stores it in *buf,
 * it will return 0 to signal the end of history
*/
int iterate_history(cmd_hist *history, char *buf) {
	
	return 0;
}
