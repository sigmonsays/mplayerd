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
#include "parse_config.h"
#include "xmemory.h"
#include "debug.h"

/*
	parse config file
*/
mpd_config *parse_config(char *cfile) {
	FILE *f;
	mpd_config *config;
	char buf[1024], ctype[65], var[65], val[256];

	memset(ctype, 0, 65);

	f = fopen(cfile, "r");

	config = (mpd_config *) xmalloc(sizeof(mpd_config));
	if (config == NULL) return NULL;

	/* default values */
	config->port = DEFAULT_PORT;
	strcpy(config->mplayer_path, "mplayer");
	strcpy(config->mplayer_vo, "x11");
	memset(config->mplayer_flags, 0, 256);

	config->allow_ips = NULL;
	strcpy(config->default_home, "/");
	strcpy(config->root, "/");
	config->max_instances = 4;

	if (f == NULL) {
		debug_msg(0, "Couldn't open config file '%s', using defaults.\n", cfile);
		return config;
	}

	while (fgets(buf, 1024, f)) {
		sscanf(buf, "[%64[a-zA-Z0-9.-]]", ctype);

		// [mplayer] section
		if (strcmp("mplayer", ctype) == 0) {
			if (sscanf(buf, "%64s = %255[^\n]", var, val) == 2) {

				if (strcmp("path", var) == 0) {
					strcpy(config->mplayer_path, val);

				} else if (strcmp("vo", var) == 0) {
					strcpy(config->mplayer_vo, val);

				} else if (strcmp("flags", var) == 0) {
					strcpy(config->mplayer_flags, val);

				} else if (strcmp("max_instances", var) == 0) {
					config->max_instances = strtol(val, NULL, 0);
				}
			}
		
		}

		// [daemon] section
		if (strcmp("daemon", ctype) == 0) {
			if (sscanf(buf, "%64s = %255[^\n]", var, val) == 2) {

				if (strcmp("port", var) == 0) {
					config->port = strtol(val, NULL, 0);

				} else if (strcmp("allow", var) == 0) {
					config->allow_ips = parse_ips(val);

				} else  if (strcmp("default_home", var) == 0) {
					strcpy(config->default_home, val);

				} else if (strcmp("root", var) == 0) {
					strcpy(config->root, val);
				}
			}
		}
	}

	fclose(f);

	return config;
}


/*
        return ll of ip address seperated by spaces
        null on error
*/
struct ll_list *parse_ips(char *ips) {
        char *p;
        struct ll_list *list;
		  char *toker;
        
        p = strtok_r(ips, " ", &toker);
        if (p) {
                list = ll_new();
                if (list == NULL) return NULL;
                
                while (p) {
                        ll_add(list, p);
                        p = strtok_r(NULL, " ", &toker);
                }
        }
        return list;
}

