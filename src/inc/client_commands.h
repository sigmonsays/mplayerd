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

#ifndef HAVE_CLIENT_COMMANDS_H
#define HAVE_CLIENT_COMMANDS_H

#include "tab_comp.h"

enum mp_commands {
	CMD_CD = 0, 
	CMD_PWD,
	CMD_LS,
	CMD_VERSION,
	CMD_STATUS,
	CMD_QUIT,
	CMD_SHUTDOWN,
	CMD_HELP,
	CMD_LOAD,
	CMD_PAUSE,
	CMD_FULLSCREEN,
	CMD_STOP,
	CMD_OSD,
	CMD_SEEK_PERCENT,
	CMD_SEEK_ABSOLUTE,
	CMD_SEEK_RELATIVE,
	CMD_LITERAL,
	CMD_VOLUME,
	CMD_MUTE,
	CMD_KILL,
	CMD_HISTORY,
	CMD_RESET,
	CMD_QLOAD,
	CMD_CLEAR,
#ifdef INSTANCE_SUPPORT
	CMD_INSTANCES,
#endif
	CMD_NEW_INSTANCE,
	CMD_DEL_INSTANCE,
	CMD_WHO,
	CMD_ARGUMENTS,
	CMD_DVD,
	CMD_LENGTH,
	CMD_ID3,
	CMD_DIRDUMP,
	CMD_FILEDUMP,
};

/*
	command argument types

	s	string
	i	integer

	n	instance
	f	file
	d	directory

	command argument types flag (they precede argument type)
	*	optional
*/

const cmd_list command_list[] = {

	{CMD_CD,	"cd",			"change directory",		"d",
			"Change to directory $directory", tab_complete_dirs
	},

	{CMD_PWD,	"pwd",			"print working directory",	"",
			"Print current working directory", NULL
	},

	{CMD_LS,	"ls",			"list files",			"*d",
			"list files in current directory or $directory", tab_complete_dirs
	},

	{CMD_VERSION,	"version",		"display version",		"",
			"displays the version of mplayerd", NULL
	},

	{CMD_STATUS,	"status",		"display status",		"*n",
			"display current status of movie/mplayerd", tab_complete_instances
	},

	{CMD_QUIT,	"quit",			"close session",		"",
			"close your session", NULL
	},

	{CMD_SHUTDOWN,	"shutdown",		"shutdown mplayerd",		"s",
			"shut down mplayerd, $string must be 'now'", NULL
	},

	{CMD_HELP,	"help",			"display help",			"s",
			"type help $string for more information on command $string", tab_complete_commands
	},

	{CMD_LOAD,	"load",			"load a file",			"*nf",
			"load $file into mplayer, use double quotes for file names with spaces", tab_complete_dirs
	},

	{CMD_PAUSE,	"pause",		"pause/unpause",		"*n",
		"	pause/unpause playback -- alias to play", tab_complete_instances
	},

	{CMD_FULLSCREEN,	"fullscreen",		"toggle fullscreen",		"*n",
				"toggle fullscreen for media playback", tab_complete_instances
	},

	{CMD_STOP,	"stop",			"stop (close) movie",		"*n",
			"stop and close the playing movie", tab_complete_instances
	},

	{CMD_OSD,	"osd",			"on screen display level",	"*ni",
		"change the OSD (On Screen Display) level $integer\r\n"
		"0 - no OSD\r\n"
		"1 - seek bar\r\n"
		"2 - seek bar + seconds\r\n"
		"3 - seek bar + seconds + time remaining", tab_complete_instances
	},

	{CMD_SEEK_PERCENT,	"seek_percent",		"seek by percent",		"ni",
				"seek to percent $integer of the movie\r\n"
				"not all media will support seeking", tab_complete_instances
	},

	{CMD_SEEK_ABSOLUTE, 	"seek_absolute",	"seek by seconds",		"ni",
				"seek to an absolute position $integer in seconds\r\n"
				"not all media will support seeking", tab_complete_instances
	},

	{CMD_SEEK_RELATIVE,	"seek_relative",	"seek relatively by seconds",	"ni",
			"seek from current position by $integer seconds\r\n"
			"not all media will support seeking", tab_complete_instances
	},

	{CMD_LITERAL,	"literal",		"send mplayer a command",	"*ns",
			"send mplayer a command directly\r\n"
			"enclose entire command $string in double quotes\r\n"
			"see man mplayer(1) for more information about SLAVE MODE PROTOCOL", tab_complete_instances
	},

	{CMD_VOLUME,	"volume",		"adjust the volume down/up",	"ni",
			"adjust the volume down/up\r\n"
			"$integer is 0 for down and 1 for up", tab_complete_instances
	},

	{CMD_MUTE,	"mute",			"mute volume",			"*n",
			"mute the volume", tab_complete_instances
	},

	{CMD_KILL,	"kill",			"kill session",			"i",
			"kill user session $integer", tab_complete_clients
	},

	{CMD_HISTORY,	"history",		"display command history",	"",
			"display previous command history", NULL
	},

	{CMD_RESET,	"reset",		"reset mplayerd",		"*n",
		"reset mplayer if something should go wrong", tab_complete_instances
	},

	{CMD_QLOAD,	"qload",		"quick load",			"*ni",
			"load file $integer from current directory", tab_complete_instances
	},

	{CMD_CLEAR,	"clear",		"clear terminal",		"",
			"clear the terminal screen", NULL
	},

	{CMD_INSTANCES,	"instances",		"display instances",		"",
			"displays a list of available instances", NULL
	},

	{CMD_NEW_INSTANCE,	"new",		"create new instance",		"",
			"create a new instance, prints the new instance id", NULL
	},

	{CMD_DEL_INSTANCE,	"delete",		"delete instance",		"n",
			"deletes specified instance", tab_complete_instances
	},

	{CMD_WHO,	"who",			"display who's online",		"",
			"display who is currently connected to mplayerd"
			"displays ID, IP and CWD", NULL
	},

	{CMD_ARGUMENTS,	"arguments",		"change arguments for mplayer",	"*ns",
			"change/show arguments for instance, enclose arguments in double quotes", tab_complete_instances
	},

	{CMD_DVD,	"dvd",	"load title from dvd",	"i",
			"Load title $integer from dvd", NULL
	},

	{CMD_LENGTH, "length", "get length of loaded file", "",
		"gets length of file loaded (in seconds), use the status command to view length", NULL
	},

	{CMD_ID3, "id3", "prints id3 tags for filename", "s",
		"prints id3 tags for filename", tab_complete_dirs
	},


	{CMD_DIRDUMP, "dirdump", "dump directory information", "s",
		"Dumps directory information recursively.", tab_complete_dirs
	},

	{CMD_FILEDUMP, "filedump", "dump file information from directory", "s",
		"Dumps file information for specified directory.", tab_complete_dirs
	},


	{ 0, NULL, NULL, NULL, NULL, NULL }

};

#endif
