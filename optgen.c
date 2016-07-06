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
#define PAGE PATH_MAX
#define LINE NAME_MAX

char *mainprog;

char *helpmsg = "\n\tUsage: untitled [option] file or dir\n"
  "\n\tOptions:\n"
  "\t-h outputs this help message.\n"
  "\t-???\n"
  ;

typedef struct options_t {
	char *shortname;
	char *longname;
	char *optarg;
	char *helptext;
	char *code;
} options_t;

typedef struct vars {
	char *vname;
	char *type;
	char *deflt;
} vars;

typedef struct govars_t {	// vars local to gopt.c
	char *gname;
	char *type;
	char *deflt;
} govars_t;

static void dohelp(int forced);
static int counttags(char *fro, char *to, const char *tag);
static void gettagaddress(const char *tag, char *fro, char *to,
							char *list[]);
static void getvarsdata(char *fro, char *to, vars *vardat);
static void getgvsdata(char *fro, char *to, govars_t *gvsdat);

static char *getdatabytag(const char *opn, const char *cls,
							char *fro, char *to);
static void getoptsdata(char *fro, char *to, options_t *optdat);
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
static void initoptstring(const char *optsfile, options_t opts[],
							const int ocount);
static void setovdefaults(const char *optsfile, vars  vs[],
							const int nvs);
static void setgvsdefaults(const char *optsfile, govars_t  gvs[],
							const int ngs);
static void writefixeddata(const char *optsfile, const char *datafile);
static void writelongoptlines(const char *optsfile, options_t opts[],
							const int ocount);
static void writeoptionsprocessing(const char *optsfile,
									options_t opts[],
							const int ocount);
static void gatherhelptext(const char *optsfile, options_t opts[],
							const int ocount);
static char *formathelplines(char *raw);
static void writelongoptions(const char *optsfile, options_t opts[],
								const int ocount);
static void writeshortoptions(const char *optsfile, options_t opts[],
								const int ocount);
static void stripspace_r(char *stripfrom, char *result);
static void reformatcode(char *existing, char *new, int nrtabs);
static void bufferguard(const char *buf, const char *where);

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
	int gcount = counttags(mydat.from, mydat.to, "<gname>");
	int vcount = counttags(mydat.from, mydat.to, "<vname>");
	int ocount = counttags(mydat.from, mydat.to, "<name>");

	govars_t gvs[gcount];
	options_t opts[ocount];
	vars vdat[vcount];

	char *gtaglist[gcount];
	gettagaddress("<gname>", mydat.from, mydat.to, gtaglist);
	char *vtaglist[vcount];
	gettagaddress("<vname>", mydat.from, mydat.to, vtaglist);
	char *otaglist[ocount];
	gettagaddress("<name>", mydat.from, mydat.to, otaglist);
	int i;
	for (i = 0; i < gcount; i++) {
		getgvsdata(gtaglist[i], mydat.to, &gvs[i]);
	}
	for (i = 0; i < vcount; i++) {
		getvarsdata(vtaglist[i], mydat.to, &vdat[i]);
	}
	for (i = 0; i < ocount; i++) {
		getoptsdata(otaglist[i], mydat.to, &opts[i]);
	}
	/* data gathering complete. Write the page tops for the C code with
	 * a main(), gopt.h|c
	*/
	init3files(xmlfile);
	makevarstruct("gopt.h", vdat, vcount);
	writefixeddata("gopt.h", "~/.config/gengetoptions/gopt_h1.xml");
	char *hend = "#endif\n";
	writefile("gopt.h", hend, NULL, "a");	// gopt.h done.
	writefixeddata("gopt.c", "~/.config/gengetoptions/gopt_c1.xml");
	gatherhelptext("gopt.c",  opts, ocount);
	initoptstring("gopt.c", opts, ocount);
	setgvsdefaults("gopt.c", gvs, gcount);
	setovdefaults("gopt.c", vdat, vcount);
	writefixeddata("gopt.c", "~/.config/gengetoptions/gopt_c2.xml");
	writelongoptlines("gopt.c", opts, ocount);
	writefixeddata("gopt.c", "~/.config/gengetoptions/gopt_c3.xml");
	writeoptionsprocessing("gopt.c", opts, ocount);
	writefixeddata("gopt.c", "~/.config/gengetoptions/gopt_c4.xml");
	writefixeddata(mainprog, "~/.config/gengetoptions/main_c.xml");
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

void getgvsdata(char *fro, char *to, govars_t *gvsdat)
{	// I know the tag names.
	char *res = getdatabytag("<gname>", "</gname>", fro, to);
	gvsdat->gname = dostrdup(res);
	res = getdatabytag("<type>", "</type>", fro, to);
	gvsdat->type = dostrdup(res);
	res = getdatabytag("<default>", "</default>", fro, to);
	gvsdat->deflt = dostrdup(res);
} // getgvsdata()

void getvarsdata(char *fro, char *to, vars *vardat)
{	// I know the tag names.
	char *res = getdatabytag("<vname>", "</vname>", fro, to);
	vardat->vname = dostrdup(res);
	res = getdatabytag("<type>", "</type>", fro, to);
	vardat->type = dostrdup(res);
	res = getdatabytag("<default>", "</default>", fro, to);
	vardat->deflt = dostrdup(res);
} // getvarsdata()

void getoptsdata(char *fro, char *to, options_t *optdat)
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
	static char buf[PAGE];	// useable for options_t as well as vars
	char wrk [PAGE];
	size_t len = strlen(opn);
	char *cp = fro;	// begin all searches from the top of the group.
	cp = memmem(cp, to - cp, opn, len);
	if(!cp) tagerror(opn);
	cp += len;
	char *clcp = memmem(cp, to - cp, cls, len+1);
	if(!clcp) tagerror(cls);
	size_t dlen = clcp - cp;
	strncpy(wrk, cp, dlen);
	wrk[dlen] = '\0';	// get rid of any trailing tab
	if(wrk[dlen-1] == '\t') wrk[dlen-1] = '\0';
	char *wp = wrk;
	while(*wp == '\n') wp++;	// remove leading '\n' but keep '\t'.
	strcpy(buf, wp);
	return buf;
} // getdatabytag()

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
					"char *optstring;\nchar *helptext;", sd.from);
	hl1 = "#include \"fileops.h\"";
	hl2 = "#include \"stringops.h\"";
	writeinitfile("gopt.c", user, yy, email, hl1, hl2,
					"#include \"gopt.h\"", sd.from);
	mainprog = cfilefromxml(xmlfile);
	writeinitfile(mainprog, user, yy, email, hl1, hl2,
					"#include \"gopt.h\"", sd.from);
} // init3files()

char *getthisyear(void)
{
	time_t now;
	struct tm *tp;
	now = time (NULL);
	static char yy[5];
	tp = localtime(&now);
	sprintf(yy, "%d", tp->tm_year + 1900);
	return yy;
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
	static char buf[LINE];
	strcpy(buf, xmlfile);
	char *cp = strstr(buf, ".xml");
	*cp = '\0';
	strcat(buf, ".c");
	return buf;
} // cfilefromxml()

void makevarstruct(const char *optheader, vars vs[], const int nvs)
{
	int i;
	vars localvs;
	char buf[PAGE];
	strcpy(buf, "typedef struct opstruct {\n");
	for (i = 0; i < nvs; i++) {
		localvs = vs[i];
		char line[LINE];
		sprintf(line, "%s %s;\n", localvs.type, localvs.vname);
		strcat(buf, line);
	}
	strcat(buf, "} options_t;\n");
	bufferguard(buf, "makevarstruct");
	writefile(optheader, buf, buf + strlen(buf), "a");
} // makevarstruct()

void initoptstring(const char *optsfile, options_t opts[],
						const int ocount)
{
	char retbuf[PAGE], wrkbuf[PAGE];
	wrkbuf[0] = '\0';
	int i;
	strcpy(retbuf, "\n\toptstring = ");
	options_t localopt;
	for (i = 0; i < ocount; i++) {
		localopt = opts[i];
		if (strlen(localopt.shortname)) {
			strcat(wrkbuf, localopt.shortname);
		} // if()
	} // for()
	if (strlen(wrkbuf)) { // no short options at all is possible
		strcat(retbuf, "\":");
		strcat(retbuf, wrkbuf);
		strcat(retbuf, "\"");
	} else {
		strcat(retbuf, "\"\""); // man 3 getopt
	}
	strcat(retbuf, ";\n");
	bufferguard(retbuf, "initoptstring");
	writefile(optsfile, retbuf, NULL, "a");
} // initoptstring()

void setovdefaults(const char *optsfile, vars  vs[], const int nvs)
{
	char buf[PAGE];
	strcpy(buf, "\n\t/* set up defaults for opt vars. */\n");
	strcat(buf, "\toptions_t opts;\n");
	int i;
	for (i = 0; i < nvs; i++) {
		vars localvs = vs[i];
		char wrk[LINE];
		char line[LINE];
		strcpy(wrk, localvs.deflt);
		stripspace_r(wrk, wrk);
		sprintf(line, "\t%s\n", wrk);
		strcat(buf, line);
	} // for()
	bufferguard(buf, "setovdefaults");
	writefile(optsfile, buf, NULL, "a");
} // setovdefaults()

void writefixeddata(const char *optsfile, const char *datafile)
{
	char *xmlfile = get_realpath_home(datafile);
	fdata mydat = readfile(xmlfile, 0, 1);
	strdata sd = getdatafromtagnames(mydat.from, mydat.to, "text");
	writefile(optsfile, sd.from, sd.to, "a");
} // writefixeddata()

void writelongoptlines(const char *optsfile, options_t opts[],
						const int ocount)
{
	char *fmt = "\t\t{\"%s\",\t%s,\t%s,\t%s },\n";
	char buf[PAGE];
	buf[0] = '\0';
	options_t localopts;
	int i;
	for (i = 0; i < ocount; i++) {
		char line[LINE];
		localopts = opts[i];
		if (strlen(localopts.longname)) {
			char snbuf[8];
			if (strlen(localopts.shortname)) {
				strcpy(snbuf, "'");
				strcat(snbuf, localopts.shortname);
				strcat(snbuf, "'");
			} else {
				strcpy(snbuf, "0");
			}
			sprintf(line, fmt, localopts.longname, localopts.optarg,
						"0", snbuf);
		}
		strcat(buf, line);
	}
	strcat(buf, "\t\t{0,\t0,\t0,\t0 }\n\t\t\t};\n");
	bufferguard(buf, "writelongoptlines");
	writefile(optsfile, buf, NULL, "a");
} // writelongoptlines()

void writeoptionsprocessing(const char *optsfile, options_t opts[],
							const int ocount)
{
	writelongoptions(optsfile, opts, ocount);
	writeshortoptions(optsfile, opts, ocount);
} // writeoptionsprocessing()

static void gatherhelptext(const char *optsfile, options_t opts[],
							const int ocount)
{
	char buf[PAGE];
	int i;
	options_t localopts;
	buf[0] = '\0';
	for (i = 0; i < ocount; i++) {
		char line[PAGE];
		localopts = opts[i];
		char *helptext = formathelplines(localopts.helptext);
		char *fmt;
		if (strlen(localopts.shortname) && strlen(localopts.longname)) {
			fmt = "  \"\\t-%s, --%s\\n\"\n%s";
			sprintf(line, fmt, localopts.shortname, localopts.longname,
					helptext);
		} else if (strlen(localopts.shortname)) {
			fmt = "  \"\\t-%s\\n\"\n\"%s\\n";
			sprintf(line, fmt, localopts.shortname, helptext);
		} else {
			fmt = "  \"\\t--%s\\n\"\n\"%s\\n";
			sprintf(line, fmt, localopts.longname, helptext);
		}
		strcat(buf, line);
	}
	strcat(buf, "  ;\n");
	bufferguard(buf, "gatherhelptext");
	writefile(optsfile, buf, NULL, "a");
} // gatherhelptext()

char *formathelplines(char *raw)
{
	static char buf1[PAGE];
	char buf2[PAGE], buf3[PAGE];
	const size_t linelen = 72;
	strcpy(buf3, raw);
	char *cp = buf3;
	size_t ll = strlen(buf3);
	char *ep = cp + ll -1;
	while(isspace(*cp) && cp < ep) cp++;	// bye leading whitespace
	while(isspace(*ep) && ep > cp) *ep = '\0'; // bye trailing ditto
	strcpy(buf2, cp);
	cp = buf2;
	while (*cp != '\0') { // all whitespace becomes a simple ' '.
		if(isspace(*cp)) *cp = ' ';
		cp++;
	}
	int inspace = 0;
	cp = buf2;
	memset(buf3, 0, PAGE);	// guard against garbage data
	ep = buf3;
	while (*cp != '\0') { // reduce clusters of ' ' to 1 ' '.
		if (inspace && *cp == ' ') {
			cp++;	// no copy ' '
		} else if (!inspace && *cp == ' ') {
			inspace = 1;
			*ep = *cp;	// copy this ' '
			ep++; cp++;
		} else {
			inspace = 0;
			*ep = *cp;
			ep++; cp++;
		}
	} // while()
	*ep = '\0';
	ll = strlen(buf3);
	buf1[0] = 0;
	char *bol = "  \"\\t";
	char *eol = "\\n\"\n";
	while (ll > linelen) {
		ep = buf3 + linelen - 1;
		while(!isspace(*ep)) ep--;
		// If user provides insane data he is welcome to the segfault.
		*ep = '\0';
		/* All that happens here is that bunches of words, 72 chars long
		 * or less, and wrapped thus '  "\thelptext words... \n"'
		 * are returned to the caller.
		*/
		strcat(buf1, bol);
		strcat(buf1, buf3);
		strcat(buf1, eol);	// a line to display
		ep++;
		strcpy(buf2, ep);	// make the next line ready
		memset(buf3, 0, PAGE);
		strcpy(buf3, buf2);
		ll = strlen(buf3);
	} // while(ll ..)
	if (strlen(buf3)) {
		strcat(buf1, bol);
		strcat(buf1, buf3);
		strcat(buf1, eol);
	}
	return buf1;
} // formathelplines()

void writelongoptions(const char *optsfile, options_t opts[],
								const int ocount)
{	/* In what follows I will be providing a case stub for each and
	* every long option regardless of whether a corresponding short
	* option exixts, without yet knowing if these stubs will even
	* execute if there is a short option in place.
	* Following on will be the case code for the short options.
	* */
	char buf[PAGE];
	options_t localopts;
	strcpy(buf, "\t\tcase 0:\n\t\t\tswitch (option_index) {\n");
	int i;
	for (i = 0; i < ocount; i++) {
		localopts = opts[i];
		char line[LINE];
		if (strlen(localopts.shortname) == 0) {
			sprintf(line, "\t\t\tcase %d:\n", i);
			strcat(buf, line);
			reformatcode(localopts.code, line, 1);
			strcat(buf, line);
			strcpy(line, "\t\t\tbreak;\n");
			strcat(buf, line);
		}
	} // for()
	strcat(buf, "\t\t\t} // switch()\n\t\tbreak;\n");
	bufferguard(buf, "writelongoptions");
	writefile(optsfile, buf, NULL, "a");
} // writelongoptions()

void writeshortoptions(const char *optsfile, options_t opts[],
								const int ocount)
{
	char buf[PAGE];
	buf[0] = '\0';
	options_t localopts;
	int i;
	for (i = 0; i < ocount; i++) {
		localopts = opts[i];
		char line[LINE];
		if (strlen(localopts.shortname)) {
			sprintf(line, "\t\tcase '%s':\n", localopts.shortname);
			strcat(buf, line);
			reformatcode(localopts.code, line, 0);
			strcat(buf, line);
			strcpy(line, "\t\tbreak;\n");
			strcat(buf, line);
		}
	} // for()
	bufferguard(buf, "writeshortoptions");
	writefile(optsfile, buf, NULL, "a");
} // writeshortoptions()

void setgvsdefaults(const char *optsfile, govars_t  gvs[],
							const int ngs)
{
	char buf[PAGE];
	int i;
	// Write comment first
	strcpy(buf, "\n\t/* declare and set defaults for local variables."
				" */\n");
	govars_t localgvs;
	for (i = 0; i < ngs; i++) {
		char line[LINE];
		char *fmt = "\t%s %s\n\t%s\n";
		localgvs = gvs[i];
		char typ[LINE], gname[LINE], deflt[LINE];
		stripspace_r(localgvs.type, typ);
		stripspace_r(localgvs.gname, gname);
		stripspace_r(localgvs.deflt, deflt);
		sprintf(line, fmt, typ, gname, deflt);
		strcat(buf, line);
	}
	bufferguard(buf, "setgvsdefaults");
	writefile(optsfile, buf, NULL, "a");
} // setgvsdefaults()

void stripspace_r(char *stripfrom, char *result)
{
	char buf[PAGE];
	strcpy(buf, stripfrom);
	char *cp = buf;
	char *ep;
	size_t len = strlen(buf);
	ep = cp + len - 1;
	while(isspace(*cp) && cp < ep) cp++;
	while(isspace(*ep) && ep > cp) {
		*ep = 0;
		ep--;
	}
	strcpy(result, cp);
} // stripspace()

void reformatcode(char *existing, char *new, int nrtabs)
{	/* prepends nrtabs '\t' to each line of code */
	char line[LINE];
	char wrk[LINE];
	char fivetabs[5];
	strcpy(fivetabs, "\t\t\t\t\t");
	fivetabs[nrtabs] = 0;
	memset(wrk, 0, LINE);
	line[0] = 0;
	strcpy(wrk, existing);
	char *le = wrk + strlen(wrk) - 1;
	char *guard = le;
	while (*le == '\t') le--;
	*le = 0;	// get rid of extraneous tabs on the end
	le = wrk;
	char *cp = wrk;
	while (1) {
		while (*le != '\n' && le <= guard) le++;
		*le = 0;
		strcat(line, fivetabs);
		strcat(line, cp);
		strcat(line, "\n");
		le++;
		cp = le;
		if (strlen(cp) == 0) break;
	}
	strcpy(new, line);
} // reformatcode()

void bufferguard(const char *buf, const char *where)
{	/* puts out a warning on stderr if a buffer is 50% or more of
	 * the size represented by PAGE
	*/
	size_t len = strlen(buf);
	double pctd = (double) len / PAGE + 0.5;
	size_t pct = pctd;
	if (pct >= 50) {
		fprintf(stderr, "Buffer is %lu per cent full at function %s.\n",
					pct, where);
	}
} // bufferguard()
