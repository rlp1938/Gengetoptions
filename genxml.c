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
#include "firstrun.h"

char *helptext =
  "\tSYNOPSIS\n"
  "\tgenxml xmlfilename\n"
  "\tWhen run without options, initialises xmlfilename with -h option\n"
  "\tprocessing.\n\n"
  "\tgenxml -a OPTNAME,shortname,longname xmlfilename\n"
  "\tAdds a skeleton options section to xmlfilename, initialised with\n"
  "\tOPTNAME, shortname and longname. Shortname, longname may be \"\"\n"
  "\tbut not both. xmlfilename must exist.\n"
  "\tOPTIONS\n"
  "\t-h\tPrints help message then exits.\n"
  "\t-a\tRequires an optarg of the form, NAME,shortname,longname.\n"
  "\t\tShortname or longname may be zero length but not both.\n"
  "\t\tAppends an xml section to the named xml file. A non-existent\n"
  "\t\txml file is an error.\n"
  "\t-c\tDoes the same as -a but also adds the closing tag to the xml\n"
  "\t\tfile.\n"
  ;

static void dohelp(int forced);
static void writeinitial(const char *fn);
static void writeadd(const char *fn, char *addargs);
static void writeclose(const char *fn, char *addargs);
static void splitaddargs(char * addargs, char *name, char *sn,
							char *ln);

int main(int argc, char **argv)
{
	// defaults
	int add = 0;
	int cls = 0;
	int init = 1;
	int opt;
	char addargs[NAME_MAX];
	// options
	char *myoptstring = ":ha:c:";
	while ((opt = getopt(argc, argv, myoptstring)) != -1) {
		switch(opt) {
		case 'h':
		dohelp(0);
		break;
		case 'a':
		add = 1;
		init = 0;
		strcpy(addargs, optarg);
		break;
		case 'c':
		cls = 1;
		init = 0;
		strcpy(addargs, optarg);
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
	if (checkfirstrun("genxml")) {
		firstrun("genxml", "gopt_c1.xml", "gopt_c2.xml", "gopt_c3.xml",
					"gopt_c4.xml", "gopt_h1.xml", "pagetop.xml",
					"main_c.xml", "add.xml", "help.xml", "ggoconfig",
					NULL);
		fprintf(stdout, "Configuration files installed.\n"
		"Please edit $HOME/.config/gengetoptions/ggoconfig\n");
		exit(EXIT_SUCCESS);
	}

	char *fn = argv[optind];
	if (init) {
		writeinitial(fn);
	} else if (add) {
		writeadd(fn, addargs);
	} else if (cls) {
		writeclose(fn, addargs);
	}
	return 0;
}

void dohelp(int forced)
{
	fputs(helptext, stdout);
	exit(forced);
} // dohelp()

void writeinitial(const char *fn)
{	/* "~/anything" always bombs "no such file" */
	char *opntag = "<options>";
	fdata mydat = readfile(get_realpath_home(
	"~/.config/gengetoptions/help.xml"), 0, 1);
	strdata sd = getdatafromtagnames(mydat.from, mydat.to, "text");
	writefile(fn, opntag, NULL, "w");
	writefile(fn, sd.from, sd.to, "a");
	free(mydat.from);
} // writeinitial()

void writeadd(const char *fn, char *addargs)
{	/* "~/anything" always bombs "no such file" */
	if ((fileexists(fn) == -1)) {
		fprintf(stderr, "No such file: %s\n", fn);
		exit(EXIT_FAILURE);
	}
	fdata mydat = readfile(get_realpath_home(
	"~/.config/gengetoptions/add.xml"), 0, 1);
	strdata sd = getdatafromtagnames(mydat.from, mydat.to, "text");
	*sd.to = '\0';	// sd.from now a C string.
	char name[NAME_MAX], sn[NAME_MAX], ln[NAME_MAX];
	splitaddargs(addargs, name, sn, ln);
	char buf[PATH_MAX];
	sprintf(buf, sd.from, name, sn, ln);
	writefile(fn, buf, NULL, "a");
	free(mydat.from);
} // writeadd()

void writeclose(const char *fn, char* addargs)
{
	writeadd(fn, addargs);
	char *endtag = "</options>\n";
	writefile(fn, endtag, NULL, "a");
} // writeclose()

void splitaddargs(char * addargs, char *name, char *sn,	char *ln)
{
	char buf[NAME_MAX];
	strcpy(buf, addargs);	// preserve addargs
	char *cp = strchr(buf, ',');
	if (!cp) {
		fprintf(stderr, "No separator found: %s\n", buf);
		exit(EXIT_FAILURE);
	}
	*cp = '\0';
	strcpy(name, buf);
	char *nxtarg = cp + 1;
	cp = strchr(nxtarg, ',');
	if (!cp) {
		fprintf(stderr, "No separator found: %s\n", nxtarg);
		exit(EXIT_FAILURE);
	}
	*cp = '\0';
	strcpy(sn, nxtarg);
	nxtarg = cp + 1;
	strcpy(ln, nxtarg);
	if (strlen(sn) == 0 && strlen(ln) == 0){
		fprintf(stderr, "Both long and short names are 0 length: %s\n",
				addargs);
		exit(EXIT_FAILURE);
	}
} // splitaddargs()
