
/*      optgen.c
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

char *helpmsg = "\n\tUsage: untitled [option] file or dir\n"
  "\n\tOptions:\n"
  "\t-h outputs this help message.\n"
  "\t-???\n"
  ;

typedef struct {
	char *shortname;
	char *longname;
	char *optarg;
	char *helptext;
	char *code;
} opts;

typedef struct {
	char *vname;
	char *type;
	char *deflt;
} vars;

static void dohelp(int forced);
static int counttags(char *fro, char *to, const char *tag);
static void gettagaddress(const char *tag, char *fro, char *to,
							char *list[]);
static void getvarsdata(char *fro, char *to, vars *vardat);
static char *getdatabytag(const char *opn, const char *cls,
							char *fro, char *to);
static void getoptsdata(char *fro, char *to, opts *optdat);
static void tagerror(const char *tag);


int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "You must provide an xml file.\n");
		exit(EXIT_FAILURE);
	}

	int opt;
	while((opt = getopt(argc, argv, ":h etc ")) != -1) {
		switch(opt){
		case 'h':
			dohelp(0);
		break;
		case 'x': // fill in actual options
		break;
		case ':':
			fprintf(stderr, "Option %c requires an argument\n",optopt);
			dohelp(1);
		break;
		case '?':
			fprintf(stderr, "Illegal option: %c\n",optopt);
			dohelp(1);
		break;
		} //switch()
	}//while()

	fdata mydat = readfile(argv[optind], 0, 1);
	int vcount = counttags(mydat.from, mydat.to, "<vname>");
	int ocount = counttags(mydat.from, mydat.to, "<name>");
	opts od[ocount];
	vars vdat[vcount];
	char *vtaglist[vcount];
	gettagaddress("<vname>", mydat.from, mydat.to, vtaglist);
	char *otaglist[ocount];
	gettagaddress("<name>", mydat.from, mydat.to, otaglist);
	int i;
	for (i = 0; i < vcount; i++) {
		getvarsdata(vtaglist[i], mydat.to, &vdat[i]);
	}
	for (i = 0; i < ocount; i++) {
		getoptsdata(otaglist[i], mydat.to, &od[i]);
	}


	return 0;
}//main()

void dohelp(int forced)
{
  fputs(helpmsg, stderr);
  exit(forced);
}

static int counttags(char *fro, char *to, const char *tag)
{
	int count = 0;
	char *cp = fro;
	size_t taglen = strlen(tag);
	while (cp < to) {
		cp = memmem(cp, to -cp, tag, taglen);
		if (!cp) break;
		count++;
		cp += taglen;
	}
	return count;
} // counttags()

void gettagaddress(const char *tag, char *fro, char *to, char *list[])
{
	int i = 0;
	char *cp = fro;
	size_t tlen = strlen(tag);
	while (cp < to) {
		cp = memmem(cp, to - cp, tag, tlen);
		if (!cp) break;
		list[i] = cp;
		i++;
		cp += tlen;
	}

} // gettagaddress()

void getvarsdata(char *fro, char *to, vars *vardat)
{	// I know the tag names.
	char *res = getdatabytag("<vname>", "</vname>", fro, to);
	vardat->vname = dostrdup(res);
	res = getdatabytag("<type>", "</type>", fro, to);
	vardat->type = dostrdup(res);
	res = getdatabytag("<default>", "</default>", fro, to);
	vardat->deflt = dostrdup(res);
} // getvarsdata()

void getoptsdata(char *fro, char *to, opts *optdat)
{	// tags: shortname, longname, code, optarg, helptext
	char *res = getdatabytag("<shortname>", "</shortname>", fro, to);
	optdat->shortname = dostrdup(res);
	res = getdatabytag("<longname>", "</longname>", fro, to);
	optdat->longname = dostrdup(res);
	res = getdatabytag("<code>", "</code>", fro, to);
	optdat->code = dostrdup(res);
	res = getdatabytag("<optarg>", "</optarg>", fro, to);
	optdat->optarg = dostrdup(res);
	res = getdatabytag("<helptext>", "</helptext>", fro, to);
	optdat->helptext = dostrdup(res);
} // getoptsdata()

void tagerror(const char *tag)
{
	fprintf(stderr, "Missing tag in xml file: %s\n", tag);
	exit(EXIT_FAILURE);
} //

char *getdatabytag(const char *opn, const char *cls, char *fro,
						char *to)
{
	static char buf[PATH_MAX];	// useable for opts as well as vars
	size_t len = strlen(opn);
	char *cp = fro;	// begin all searches from the top of the group.
	cp = memmem(cp, to - cp, opn, len);
	if(!cp) tagerror(opn);
	cp += len;
	char *clcp = memmem(cp, to - cp, cls, len+1);
	if(!clcp) tagerror(cls);
	size_t dlen = clcp - cp;
	strncpy(buf, cp, dlen);
	buf[dlen] = '\0';
	if(buf[dlen-1] == '	') buf[dlen-1] = '\0';;
	return buf;
} // getvardatabytag()

