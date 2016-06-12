/*      genxml.c
 *
 *	Copyright 2016 Bob Parker rlp1938@gmail.com
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


#include "fileops.h"
#include "stringops.h"

char *helptext =
  "\tSYNOPSIS\n"
  "\tgenxml xmlfilename\n"
  "\tWhen run without options, initialises xmlfilename with -h option\n"
  "\tprocessing.\n\n"
  "\tgenxml -a OPTNAME,shortname,longname xmlfilename\n"
  "\tAdds a skeleton options section to xmlfilename, initialised with\n"
  "OPTNAME, shortname and longname. Shortname, longname may be \"\"\n"
  "\tbut not both. xmlfilename must exist.\n"
  "\t-h\tPrints help message then exits.\n"
  ;

static void dohelp(int forced);
static void writeinitial(const char *fn);
static void writeadd(const char *fn);
static void writeclose(const char *fn);

int main(int argc, char **argv)
{
	// defaults
	int add = 0;
	int cls = 0;
	int init = 1;
	int opt;
	// options
	char *myoptstring = ":ha:c";
	while ((opt = getopt(argc, argv, myoptstring)) != -1) {
		switch(opt) {
		case 'h':
		dohelp(0);
		break;
		case 'a':
		add = 1;
		init = 0;
		break;
		case 'c':
		cls = 1;
		init = 0;
		break;
		case ':':
		fprintf(stderr, "Option %s requires an argument\n",
                argv[optind]);
        dohelp(1);
		break;
		case '?':
		fprintf(stderr, "Unknown option: %s\n",
				argv[optind]);
        dohelp(1);
		break;
		}
	}

	char *fn = argv[optind];
	if (init) {
		writeinitial(fn)
	} else if (add) {
		writeadd(fn);
	} else if (cls) {
		writecls(fn);
	}
	return 0;
}

void dohelp(int forced)
{
	fputs(helptext, stdout);
	exit(forced);
} // dohelp()

void writeinitial(const char *fn)
{
	fdat mydat = readfile("~/.config/gengetoptions/help.xml");
} // writeinitial()

void writeadd(const char *fn)
{
} // writeadd()

void writeclose(const char *fn)
{
} // writeclose()

