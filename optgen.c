
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
static void init3files(char *xmlfile);
static char *getthisyear(void);
static void writeinitfile(const char *fn, const char *user,
							const char *yy, const char *email,
							const char *vline1, const char *vline2,
							const char *vline3, const char *fmt);
static char *cfilefromxml(char *xmlfile);
static void makevarstruct(const char *optheader, vars  vs[],
							const int nvs);
static void initoptstring(const char *optsfile, opts od[],
							const int ocount);
static void setovdefaults(const char *optsfile, vars  vs[],
							const int nvs);

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
	char *xmlfile = argv[optind];
	fdata mydat = readfile(xmlfile, 0, 1);
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
	/* data gathering complete. Write the page tops for the C code with
	 * a main(), gopt.h|c
	*/
	init3files(xmlfile);
	makevarstruct("gopt.h", vdat, vcount);
	char *hend = "\n#endif\n";
	writefile("gopt.h", hend, NULL, "a");	// gopt.h done.
	initoptstring("gopt.c", od, ocount);
	setovdefaults("gopt.c", vdat, vcount);
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

void init3files(char *xmlfile)
{	/* gopt.h, gopt.c and if xmlfile is test.xml the third is test.c */
	char *yy = getthisyear();
	char **cfg = readcfg("~/.config/gengetoptions/ggoconfig");
	char *user = getcfgvalue("user", cfg);
	char *email = getcfgvalue("email", cfg);
	free(cfg);
	char *pt = get_realpath_home("~/.config/gengetoptions/pagetop.xml");
	fdata mydat = readfile(pt, 0, 1);
	strdata sd = getdatafromtagnames(mydat.from, mydat.to, "text");
	*sd.to = '\0';	// now a C string format statement.
	char *hl1 = "#ifndef GOPT_H";
	char *hl2 = "#define GOPT_H";
	writeinitfile("gopt.h", user, yy, email, hl1, hl2,
					"char *opstring;", sd.from);
	hl1 = "#include \"fileops.h\"";
	hl2 = "#include \"stringops.h\"";
	writeinitfile("gopt.c", user, yy, email, hl1, hl2,
					"#include \"gopt.h\"", sd.from);
	char *mainprog = cfilefromxml(xmlfile);
	writeinitfile(mainprog, user, yy, email, hl1, hl2,
					"#include \"gopt.h\"", sd.from);
} // init3files()

char *getthisyear(void)
{
	time_t now;
	struct tm *tp;
	now = time (NULL);
	static char yy[5];
/*	tp = localtime(&now);
	sprintf(yy, "%d", tp->tm_year);
	return yy;
	* This is just so fucked up! yy gets the value 116!
	* my work around below.
*/
	double secsyr = 365.25 * 24 * 60 *60;
	int yr = now / secsyr + 1970;
	sprintf(yy, "%d", yr);
	return yy;	// 2016 when written.
} // getthisyear()

void writeinitfile(const char *fn, const char *user,
					const char *yy, const char *email,
					const char *vline1, const char *vline2,
					const char *vline3, const char *fmt)
{
	FILE *fpo = dofopen(fn, "w");
	fprintf(fpo, fmt, fn, user, yy, email, vline1,
				vline2, vline3);
	dofclose(fpo);
} // writeinitfile()

static char *cfilefromxml(char *xmlfile)
{
	static char buf[NAME_MAX];
	strcpy(buf, xmlfile);
	char *cp = strstr(xmlfile, ".xml");
	*cp = '\0';
	strcat(buf, ".c");
	return buf;
} // cfilefromxml()

void makevarstruct(const char *optheader, vars vs[], const int nvs)
{
	int i;
	vars localvs;
	char buf[PATH_MAX];
	strcpy(buf, "typedef struct ops {\n");
	for (i = 0; i < nvs; i++) {
		localvs = vs[i];
		char line[NAME_MAX];
		sprintf(line, "%s %s;\n", localvs.type, localvs.vname);
		strcat(buf, line);
	}
	strcat(buf, "} opts;\n");
	writefile(optheader, buf, buf + strlen(buf), "a");
} // makevarstruct()

void initoptstring(const char *optsfile, opts od[], const int ocount)
{
	char retbuf[NAME_MAX], wrkbuf[NAME_MAX];
	wrkbuf[0] = '\0';
	int i;
	strcpy(retbuf, "\n\toptstring = ");
	opts localopt;
	for (i = 0; i < ocount; i++) {
		localopt = od[i];
		if (strlen(localopt.shortname)) {
			strcat(wrkbuf, localopt.shortname);
			if (strcmp(localopt.optarg, "1") == 0) {
				strcat(wrkbuf, ":");
			}
		} // if()
	} // for()
	if (strlen(wrkbuf)) { // no short options at all is possible
		strcat(retbuf, ":");
		strcat(retbuf, wrkbuf);
	} else {
		strcat(retbuf, "\"\""); // man 3 getopt
	}
	strcat(retbuf, ";\n");
	writefile(optsfile, retbuf, NULL, "a");
} // initoptstring()

void setovdefaults(const char *optsfile, vars  vs[], const int nvs)
{
	char buf[PATH_MAX];
	strcpy(buf, "\n\t/* set up defaults for opt vars. */\n");
	strcat(buf, "\topts os;\n");
	int ival;
	int i;
	for (i = 0; i < nvs; i++) {
		vars localvs = vs[i];
		char line[NAME_MAX];
		if (strcmp(localvs.type, "int") == 0) {
			ival = strtol(localvs.deflt, NULL, 10);
			sprintf(line, "\tos.%s = %d;\n", localvs.vname, ival);
		} else if (strcmp(localvs.type, "char *") == 0) {
			sprintf(line, "\tos.%s = dostrdup(%s);\n", localvs.vname,
						localvs.deflt);
		} else {
			sprintf(line, "\tos.%s = /* FIXME */%s;\n", localvs.vname,
						localvs.deflt);
		}
		strcat(buf, line);
	} // for()
	writefile(optsfile, buf, NULL, "a");
} // setovdefaults()

