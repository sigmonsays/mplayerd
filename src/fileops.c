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
#include "fileops.h"
#include "xmemory.h"

char *load_dir_entry(char *path, int entry) {
	struct dirent *file;
	DIR *d;
	unsigned int x = 0;
	char *p = NULL;
	int f = 0;

	d = opendir(path);               
	if (d == NULL) return NULL;
                         
	readdir(d);
	readdir(d);
                        
	while ((file = readdir(d))) {
		if (++x == entry) {
			f = 1;
			break;
		}
	}

	if (f) p = xstrdup(file->d_name);
	closedir(d);
	return p;
}

struct ll_list *load_dir(char *path, unsigned int max) {
        struct ll_list *list;
        struct dirent *file;
        DIR *d;
        unsigned int x = 0;

        d = opendir(path);               
        if (d == NULL) return NULL;
                         
        if ( (list = ll_new()) == NULL) return NULL;
                        
	readdir(d);
	readdir(d);

        while ((file = readdir(d))) {
                ll_add(list, file->d_name);
		if (x++ > max) break;
        }
                         
        closedir(d);
        return list;
}
                

int isdir(char *path) {
	int f;
        struct stat f_info;
        
        f = stat(path, &f_info);
        if (f == -1) return 0;
        
        if (f_info.st_mode & S_IFDIR) return 1;

	return 0;
}


/* check if file exists
        return 1 if file exists, 0 if not
*/      
int file_exists(char *file) {
	int f;
        struct stat f_info;
        
        f = stat(file, &f_info);
        if (f == -1) return 0;
        
        if (f_info.st_mode & S_IFDIR) return 0;

	return 1;
}
