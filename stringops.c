/*      stringops.c
 *
 *	Copyright 2016 Bob Parker Bob Parker rlp1938@gmail.com
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *	MA 02110-1301, USA.
*/

#include "stringops.h"
#include "fileops.h"
char *dostrdup(const char *str)
{	/* strdup() with error handling */
	char *dup = strdup(str);
	if (!dup) {
		perror(str);
		exit(EXIT_FAILURE);
	}
	return dup;
} // dostrdup()

char *getcfgvalue(const char *cfgname, char **cfglines)
{
	int i = 0;
	size_t len = strlen(cfgname);
	while (cfglines[i]) {
		if (strncmp(cfgname, cfglines[i], len) == 0) {
			return dostrdup(cfglines[i] + len + 1);
		}
		i++;
	}
	fprintf(stderr, "No parameter name: %s\n", cfgname);
	exit(EXIT_FAILURE);
} // getcfgvalue()

strdata getdatafromtagnames(char *fro, char *to, char *tagname)
{
	char tagfro[NAME_MAX], tagto[NAME_MAX];
	sprintf(tagfro, "<%s>", tagname);
	sprintf(tagto, "</%s>", tagname);
	strdata sd;
	sd.from = memmem(fro, to - fro, tagfro, strlen(tagfro));
	if (!sd.from) {
		fprintf(stderr, "Tag not found: %s", tagfro);
		exit(EXIT_FAILURE);
	}
	sd.from += strlen(tagfro);	// point to actual data
	sd.to = memmem(sd.from, to - sd.from, tagto,
							strlen(tagto));
	if (!sd.to) {
		fprintf(stderr, "Tag not found: %s", tagto);
		exit(EXIT_FAILURE);
	}
	return sd;
} // getdatafromtagnames()
