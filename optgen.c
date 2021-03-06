/*      optgen.c
 *
 *  Copyright 2016 Bob Parker rlp1938@gmail.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
*/

#include "fileops.h"
#include "stringops.h"
#include <malloc.h>
#define PAGE PATH_MAX
#define LINE NAME_MAX

char *mainprog;

typedef struct optdata_t {
	char *shortname;
	char *longname;
	char *optarg;
	char *helptext;
	char *synopsis;
	char *code;
} optdata_t;

typedef struct vars {
	char *vname;
	char *type;
	char *deflt;
} vars;

typedef enum { false, true } bool;

typedef struct govars_t {   // vars local to gopt.c
	char *gname;
	char *type;
	char *deflt;
} govars_t;

static int counttags(char *fro, char *to, const char *tag);
static void gettagaddress(const char *tag, char *fro, char *to,
							char *list[]);
static void getvarsdata(char *fro, char *to, vars *vardat);
static void getgvsdata(char *fro, char *to, govars_t *gvsdat);
static void freegvsdata(govars_t *gvsdat);
static void freevarsdata(vars *vardat);
static void freeoptsdata(optdata_t *optdat);

static char *getdatabytag(const char *opn, const char *cls,
							char *fro, char *to);
static void getoptsdata(char *fro, char *to, optdata_t *optdat,
						int index);
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
static void initoptstring(const char *optsfile, optdata_t optdat[],
							const int ocount);
static void setovdefaults(const char *optsfile, vars  vs[],
							const int nvs);
static void setgvsdefaults(const char *optsfile, govars_t  gvs[],
							const int ngs);
static void writefixeddata(const char *optsfile, const char *datafile);
static void writelongoptlines(const char *optsfile, optdata_t optdat[],
							const int ocount);
static void writeoptionsprocessing(const char *optsfile,
									optdata_t optdat[],
							const int ocount);
static void gatherhelptext(const char *optsfile, optdata_t optdat[],
							const int ocount);
static void writelongoptions(const char *optsfile, optdata_t optdat[],
								const int ocount);
static void writeshortoptions(const char *optsfile, optdata_t optdat[],
								const int ocount);
static void stripspace_r(char *stripfrom, char *result);
static void reformatcode(char *existing, char *new, int nrtabs);
static void bufferguard(const char *buf, const char *where);
static void emitsynopsis(const char *optsfile,  optdata_t optdat[], 
							const int oindex);
static void spaceclustertosingle(char *in, char *out);
static void group(char *in, char *out);
static void cleanuptext_r(char *in, char *out);
static void groupsetwidth(const char *before, char *in,
							const char *after, char *out);
static void validatexml(const char *datafile);
static bool testbyline(char *fro, char *to, char *maintag, char *vartag, 
						int *lineno);
static char *gettagdata(const char *opn, const char *cls, 
						const char *fro, const char *limit);
static void maketags_r(const char *tagname, char * opntag, char *clstag);
static void generatemakefile(char *xmlfile, const char *template);

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "You must provide an xml file.\n");
		exit(EXIT_FAILURE);
	}

	char *xmlfile = dostrdup(argv[optind]);
	/* Some errors in formatting the xml file can give rise to errors 
	 * that are abysmally hard to find. They can cause optgen to crash
	 * and also the generated main program. Hopefully, the validatexml
	 * function will turn that situation into a doddle.
	*/
	validatexml(xmlfile);	// TODO validatexml() not catching errors
	fdata mydat = readfile(xmlfile, 0, 1);

	int gcount = counttags(mydat.from, mydat.to, "gname");
	int vcount = counttags(mydat.from, mydat.to, "vname");
	int ocount = counttags(mydat.from, mydat.to, "name");

	govars_t gvs[gcount];
	optdata_t optdat[ocount];
	vars vdat[vcount];

	char *gtaglist[gcount];
	gettagaddress("gname", mydat.from, mydat.to, gtaglist);
	char *vtaglist[vcount];
	gettagaddress("vname", mydat.from, mydat.to, vtaglist);
	char *otaglist[ocount];
	gettagaddress("name", mydat.from, mydat.to, otaglist);

	int i;
	for (i = 0; i < gcount; i++) {
		getgvsdata(gtaglist[i], mydat.to, &gvs[i]);
	}

	for (i = 0; i < vcount; i++) {
		getvarsdata(vtaglist[i], mydat.to, &vdat[i]);
	}

	for (i = 0; i < ocount; i++) {
		getoptsdata(otaglist[i], mydat.to, &optdat[i], i);
	}
	/* data gathering complete. Write the page tops for the C code with
	 * a main(), gopt.h|c
	*/

	init3files(xmlfile);
	makevarstruct("gopt.h", vdat, vcount);
	writefixeddata("gopt.h", "~/.config/genxml/gopt_h1.xml");
	char *hend = "#endif\n";

	writefile("gopt.h", hend, NULL, "a");   // gopt.h done.
	writefixeddata("gopt.c", "~/.config/genxml/gopt_c1.xml");
	emitsynopsis("gopt.c",  optdat, 0);	// used at optdat[0] only.
	gatherhelptext("gopt.c",  optdat, ocount);
	initoptstring("gopt.c", optdat, ocount);
	setgvsdefaults("gopt.c", gvs, gcount);
	setovdefaults("gopt.c", vdat, vcount);
	writefixeddata("gopt.c", "~/.config/genxml/gopt_c2.xml");
	writelongoptlines("gopt.c", optdat, ocount);
	writefixeddata("gopt.c", "~/.config/genxml/gopt_c3.xml");
	writeoptionsprocessing("gopt.c", optdat, ocount);
	writefixeddata("gopt.c", "~/.config/genxml/gopt_c4.xml");
	writefixeddata(mainprog, "~/.config/genxml/main_c.xml");
	generatemakefile(xmlfile, "~/.config/genxml/makefile.xml");
	free(xmlfile);
	for (i = 0; i < gcount; i++) {
		freegvsdata(&gvs[i]);
	}
	for (i = 0; i < vcount; i++) {
		freevarsdata(&vdat[i]);
	}
	for (i = 0; i < ocount; i++) {
		freeoptsdata(&optdat[i]);
	}
	free(mydat.from);
	return 0;
}//main()

static int counttags(char *fro, char *to, const char *tag)
{
	int count = 0;
	char *cp = fro;
	char opn[LINE], cls[LINE];	/* will not bother putting these on the
									heap */
	sprintf(opn, "<%s>", tag);
	sprintf(cls, "</%s>", tag);
	size_t taglen = strlen(opn);
	while (cp < to) {
		cp = memmem(cp, to - cp, opn, taglen);
		if (!cp) break;
		char *ep = memmem(cp, to - cp, cls, taglen+1);
		if (!ep) break;
		count++;
		size_t varlen = ep - cp - taglen;
		if (varlen == 0) {
			fprintf(stderr,
					"You have a null %s in your master xml file.\n"
					"Please rectify that and run this program again.\n",
					tag);
			exit(EXIT_FAILURE);
		}
		cp = ep;
	}
	return count;
} // counttags()

/* gettagaddress()
 * returns list[] which comprises simple char *. These are not C strings
 * so if you need to view them using any of the printf() family or
 * trace() then use a format something like "%.20s" to restrict the
 * number of chars shown.
*/
void gettagaddress(const char *tag, char *fro, char *to, char *list[])
{
	int i = 0;
	char *cp = fro;
	char opn[LINE];	// not on the heap
	sprintf(opn, "<%s>", tag);
	size_t tlen = strlen(opn);
	while (cp < to) {
		cp = memmem(cp, to - cp, opn, tlen);
		if (!cp) break;
		list[i] = cp;
		i++;
		cp += tlen;
	}
} // gettagaddress()

void getgvsdata(char *fro, char *to, govars_t *gvsdat)
{   // I know the tag names.
	char *res = getdatabytag("<gname>", "</gname>", fro, to);
	gvsdat->gname = res;
	res = getdatabytag("<type>", "</type>", fro, to);
	gvsdat->type = res;
	res = getdatabytag("<default>", "</default>", fro, to);
	gvsdat->deflt = res;
} // getgvsdata()

void getvarsdata(char *fro, char *to, vars *vardat)
{   // I know the tag names.
	char *res = getdatabytag("<vname>", "</vname>", fro, to);
	vardat->vname = res;
	res = getdatabytag("<type>", "</type>", fro, to);
	vardat->type = res;
	res = getdatabytag("<default>", "</default>", fro, to);
	vardat->deflt = res;
	return;
} // getvarsdata()

void getoptsdata(char *fro, char *to, optdata_t *optdat, int index)
{   // tags: shortname, longname, code, optarg, helptext
	char *res = getdatabytag("<shortname>", "</shortname>", fro, to);
	optdat->shortname = res;
	res = getdatabytag("<longname>", "</longname>", fro, to);
	optdat->longname = res;
	res = getdatabytag("<code>", "</code>", fro, to);
	optdat->code = res;
	res = getdatabytag("<optarg>", "</optarg>", fro, to);
	optdat->optarg = res;
	res = getdatabytag("<helptext>", "</helptext>", fro, to);
	optdat->helptext = res;
	if (index == 0) {   // tag only exists for section HELP
		res = getdatabytag("<synopsis>", "</synopsis>", fro, to);
		optdat->synopsis = res;
	} else {
		optdat->synopsis = dostrdup("");
	}
} // getoptsdata()

void tagerror(const char *tag)
{
	fprintf(stderr, "Missing tag in xml file: %s\n", tag);
	exit(EXIT_FAILURE);
} //

char *getdatabytag(const char *opn, const char *cls, char *fro,
						char *to)
{	/* build buffers on heap and stop fretting about over-runs */
	char *buf;  // useable for optdata_t as well as vars
	char *wrk;
	size_t len = strlen(opn);
	char *cp = fro; // begin all searches from the top of the group.
	cp = memmem(cp, to - cp, opn, len);
	if(!cp) tagerror(opn);
	cp += len;
	char *clcp = memmem(cp, to - cp, cls, len+1);
	if(!clcp){
		tagerror(cls);
	}
	size_t dlen = clcp - cp;
	buf = docalloc(dlen+1, 1, "gettagbyname");
	wrk = docalloc(dlen+1, 1, "gettagbyname");
	strncpy(wrk, cp, dlen);
	wrk[dlen] = '\0';   // get rid of any trailing tab
	if(wrk[dlen-1] == '\t') wrk[dlen-1] = '\0';
	char *wp = wrk;
	while(*wp == '\n') wp++;    // remove leading '\n' but keep '\t'.
	strcpy(buf, wp);
	free(wrk);
	return buf;
} // getdatabytag()

void init3files(char *xmlfile)
{   /* gopt.h, gopt.c and if xmlfile is test.xml the third is test.c */
	char *yy = getthisyear();
	char **cfg = readcfg("~/.config/genxml/ggoconfig");
	char *user = getcfgvalue("user", cfg);
	char *email = getcfgvalue("email", cfg);
	int i = 0;
	while (cfg[i]) {
		free(cfg[i]);
		i++;
	}
	free(cfg);
	char *pt = get_realpath_home("~/.config/genxml/pagetop.xml");
	fdata mydat = readfile(pt, 0, 1);
	strdata sd = getdatafromtagnames(mydat.from, mydat.to, "text");
	*sd.to = '\0';  // now a C string format statement.
	char *hl1 = "#ifndef GOPT_H";
	char *hl2 = "#define GOPT_H";
	char *hl3 = "char *optstring;\nchar *helptext;\nchar* synopsis;";
	writeinitfile("gopt.h", user, yy, email, hl1, hl2, hl3, sd.from);
	hl1 = "#include \"fileops.h\"";
	hl2 = "#include \"stringops.h\"";
	writeinitfile("gopt.c", user, yy, email, hl1, hl2,
					"#include \"gopt.h\"", sd.from);
	mainprog = cfilefromxml(xmlfile);
	writeinitfile(mainprog, user, yy, email, hl1, hl2,
					"#include \"gopt.h\"", sd.from);
	free(email);
	free(user);
	free(mydat.from);
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
	strcpy(buf, "typedef struct options_t {\n");
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

void initoptstring(const char *optsfile, optdata_t optdat[],
						const int ocount)
{
	char retbuf[PAGE], wrkbuf[PAGE];
	wrkbuf[0] = '\0';
	int i;
	strcpy(retbuf, "\n\toptstring = ");
	optdata_t localopt;
	for (i = 0; i < ocount; i++) {
		localopt = optdat[i];
		if (strlen(localopt.shortname)) {
			strcat(wrkbuf, localopt.shortname);
			if (localopt.optarg[0] == '1') {
				strcat(wrkbuf, ":");	// wants optarg
			}
			
		} // if()
	} // for()
	if (strlen(wrkbuf)) { // It is possible that there are 0 short opts.
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
	strcpy(buf, "\n\t/* set up defaults for opts vars. */\n");
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
	free(mydat.from);
} // writefixeddata()

void writelongoptlines(const char *optsfile, optdata_t optdat[],
						const int ocount)
{
	char *fmt = "\t\t{\"%s\",\t%s,\t%s,\t%s },\n";
	char *buf;
	size_t bs = 0;
	int i;
	for (i = 0; i < ocount; i++) {
		bs += strlen(fmt)/* fixed */ + 32 /*safety*/
				+ strlen(optdat[i].longname);
	}
	
	buf = docalloc(bs + 32, 1, "writelongoptlines()");
	buf[0] = '\0';
	optdata_t localopts;
	for (i = 0; i < ocount; i++) {
		size_t ls = strlen(fmt) + 32 + strlen(optdat[i].longname);
		char *line = docalloc(ls, 1, "writelongoptlines()");
		localopts = optdat[i];
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
		free(line);
	}
	strcat(buf, "\t\t{0,\t0,\t0,\t0 }\n\t\t};\n");
	bufferguard(buf, "writelongoptlines");
	writefile(optsfile, buf, NULL, "a");
	free(buf);
} // writelongoptlines()

void writeoptionsprocessing(const char *optsfile, optdata_t optdat[],
							const int ocount)
{
	writelongoptions(optsfile, optdat, ocount);
	writeshortoptions(optsfile, optdat, ocount);
} // writeoptionsprocessing()

static void gatherhelptext(const char *optsfile, optdata_t optdat[],
							const int ocount)
{
	char *buf;
	int i;
	size_t bs = 0;
	// Estimate size needed by buf;
	for (i = 0; i < ocount; i++) {
		bs += 40;	// allow for option names
		bs += strlen(optdat[i].helptext);
	}
	bs += LINE;	// leave room for helptext to grow when grouped.
	buf = docalloc(bs, 1, "gatherhelptext()");
	optdata_t localopts;
	buf[0] = '\0';
	writefile(optsfile, "\thelptext =\n", NULL, "a");
	for (i = 0; i < ocount; i++) {
		char *line, *helptext;
		size_t ls;
		localopts = optdat[i];
		ls = strlen(localopts.helptext) + LINE;
		helptext = docalloc(ls, 1, "gatherhelptext()");
		line = docalloc(ls + 40, 1, "gatherhelptext()");
		group(localopts.helptext, helptext);
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
		free(line);
		free(helptext);
	}
	strcat(buf, "  ;\n");
	writefile(optsfile, buf, NULL, "a");
	free(buf);
} // gatherhelptext()

void writelongoptions(const char *optsfile, optdata_t optdat[],
								const int ocount)
{
	char *buf;
	// caculate buffer sizes
	int i;
	size_t ls, bs;
	ls = 0;
	bs = LINE;	// plenty breathing room
	for (i = 0; i < ocount; i++) {
		optdata_t localopts = optdat[i];
		if (strlen(localopts.shortname) == 0) {
			ls += 30 + 32;	// 32 arbitrary breathing room.
			ls += strlen(localopts.code);
		}
		
	}
	bs += ls;	// which may be 0.
	buf = docalloc(bs, 1, "writelongoptions()");
	strcpy(buf, "\t\tcase 0:\n\t\t\tswitch (option_index) {\n");
	for (i = 0; i < ocount; i++) {
		optdata_t localopts = optdat[i];
		if (strlen(localopts.shortname) == 0) {
			char *line;
			line = docalloc(ls, 1, "writelongoptions()");
			sprintf(line, "\t\t\tcase %d:\n", i);
			strcat(buf, line);
			reformatcode(localopts.code, line, 1);
			strcat(buf, line);
			strcpy(line, "\t\t\tbreak;\n");
			strcat(buf, line);
			free(line);
		}
	} // for()
	strcat(buf, "\t\t\t} // switch()\n\t\tbreak;\n");
	writefile(optsfile, buf, NULL, "a");
	free(buf);
} // writelongoptions()

void writeshortoptions(const char *optsfile, optdata_t optdat[],
								const int ocount)
{
	char *buf;
	size_t blen = 0;
	int i;
	for (i = 0; i < ocount; i++) {
		blen += 32 + 32 + strlen(optdat[i].code);
	}
	buf = docalloc(blen, 1, "writeshortoptions()");
	buf[0] = '\0';
	optdata_t localopts;
	for (i = 0; i < ocount; i++) {
		localopts = optdat[i];
		if (strlen(localopts.shortname)) {
			size_t llen = 32 + 32 + strlen(localopts.code);
			char *line = docalloc(llen, 1, "writeshortoptions()");
			sprintf(line, "\t\tcase '%s':\n", localopts.shortname);
			strcat(buf, line);
			reformatcode(localopts.code, line, 0);
			strcat(buf, line);
			strcpy(line, "\t\tbreak;\n");
			strcat(buf, line);
			free(line);
		}
	} // for()
	writefile(optsfile, buf, NULL, "a");
	free(buf);
} // writeshortoptions()

void setgvsdefaults(const char *optsfile, govars_t  gvs[],
							const int ngs)
{
	char *buf;
	int i;
	size_t bs = 0;
	for (i = 0; i < ngs; i++) {
		bs += strlen(gvs[i].deflt);
		bs += strlen(gvs[i].gname);
		bs += strlen(gvs[i].type);
		bs += 16;	// format chars + safety
	}
	// Write comment first
	char *cmnt = 
	"\n\t/* declare and set defaults for local variables. */\n";
	bs += strlen(cmnt) +1;	// needed +1 when ngs is 0
	buf = docalloc(bs, 1, "setgvsdefaults()");
	strcpy(buf, cmnt);
	govars_t localgvs;
	for (i = 0; i < ngs; i++) {
		char *line;
		char *fmt = "\t%s %s;\n\t%s\n";
		localgvs = gvs[i];
		char typ[LINE], gname[LINE], deflt[LINE];
		stripspace_r(localgvs.type, typ);
		stripspace_r(localgvs.gname, gname);
		stripspace_r(localgvs.deflt, deflt);
		size_t ls = strlen(typ) + strlen(gname) + strlen(deflt) +
					strlen(fmt) + 8;
		line = docalloc(ls, 1, "setgvsdefaults()");
		sprintf(line, fmt, typ, gname, deflt);
		strcat(buf, line);
		free(line);
	}
	bufferguard(buf, "setgvsdefaults");
	writefile(optsfile, buf, NULL, "a");
	free(buf);
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
{   /* prepends nrtabs '\t' to each line of code */
	char line[LINE];
	char wrk[LINE];
	char fivetabs[16];
	strcpy(fivetabs, "\t\t\t\t\t");
	fivetabs[nrtabs] = 0;
	memset(wrk, 0, LINE);
	line[0] = 0;
	strcpy(wrk, existing);
	char *le = wrk + strlen(wrk) - 1;
	char *guard = le;
	while (*le == '\t') le--;
	*le = 0;    // get rid of extraneous tabs on the end
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
{   /* puts out a warning on stderr if a buffer is 50% or more of
	 * the size represented by PAGE
	*/
	size_t len = strlen(buf);
	double pctd = (double) len / PAGE + 0.5;
	size_t pct = pctd;
	if (pct >= 50) {
		fprintf(stderr, "Buffer is %lu per cent full at function %s.\n",
					(unsigned long)pct, where);
	}
} // bufferguard()

void emitsynopsis(const char *optsfile,  optdata_t optdat[], 
							const int oindex)
{   /* Differs from helptext in that there is no insertion of help 
		chars. Otherwise the processing is the same. */
	char buf[PAGE];
	optdata_t localopts = optdat[oindex];
	writefile(optsfile, "\tsynopsis =\n", NULL, "a");
	group(localopts.synopsis, buf);
	strcat(buf, "  ;\n");
	bufferguard(buf, "emitsynopsis");
	writefile(optsfile, buf, NULL, "a");
} // emitsynopsis()

void spaceclustertosingle(char *in, char *out)
{
	char buf[PAGE], obuf[PAGE];
	memset(buf, 0, PAGE);
	memset(obuf, 0, PAGE);
	strcpy(buf, in);
	char *cp = buf;
	char *op = obuf;
	int inspace = 0;
	while(*cp != 0) {
		if (*cp == ' ' && !inspace) {
			inspace = 1;
			*op = *cp;
			op++;
		} else if (*cp != ' ') {
			inspace = 0;
			*op = *cp;
			op++;          
		}
		cp++;
	}
	strcpy(out, obuf);
} // spaceclustertosingle()

void cleanuptext_r(char *in, char *out)
{	/* strips off all leading and trailing white space chars, converts
		all interior white space chars to ' ' then converts all clusters
		of ' ' to a single ' '. */
	char wrk[PAGE], buf[PAGE];
	strcpy(wrk, in);
	stripspace_r(wrk, wrk);
	char *cp = wrk;
	while (*cp) {
		if (isspace(*cp)) {
			*cp = ' ';	// \n, \t etc to be ' '
		}
		cp++;
	}
	spaceclustertosingle(wrk, buf);
	/* if the original string comprises only whitespace, \n\t etc, then
	 * the end result will be a single ' '. So get rid of it.
	*/
	cp = buf;
	while (*cp == ' ') cp++;	// lose single ' '
	strcpy(out, cp);
} // cleanuptext_r()

void group(char *in, char *out)
{	/* take an arbitrary string and break it into pieces at a space,
	72 chars long or less. Wrap the strings in '  " .. data .. \n"' */
	/* my buffer sizes will be over estimated because the linefeeds are
	 * already in the strlen count, as are the forced \\n. No matter,
	 * the grouping is going to shorten the lines, hence use extra
	 * line ends.
	*/
	char *buf, *wrk;
	size_t bsize = strlen(in) + LINE;	// input will grow somewhat;
	buf = docalloc(bsize, 1, "group()");
	wrk = docalloc(bsize, 1, "group()");
	char *cp, *lf;
	const char *before = "  \"\\t";
	const char *after = "\\n\"\n";
	const char *fmt = "%s%s%s";
	const char *mustbreak = "\\n";
	const char *emptyline = "  \"\\n\"\n";
	strcpy(wrk, in);
	cleanuptext_r(wrk, wrk);
	cp = wrk;
	lf = strstr(cp, mustbreak);
	char line[PAGE];
	buf[0] = 0;	// buf is the formatted result. Cat everything on to it.
	while (lf) {
		*lf = 0;
		char inside[PAGE];
		if (strlen(cp)) {
			groupsetwidth(before, cp, after, inside);
			strcat(buf, inside);
		} else {
			strcat(buf, emptyline);
		}
		cp = lf + 2;
		while(isspace(*cp)) cp++;
		lf = strstr(cp, mustbreak);
	}
	if (strlen(cp)) {
		sprintf(line, fmt, before, cp, after);
	}
	strcpy(out, buf);
	free(wrk);
	free(buf);
} // group()

void groupsetwidth(const char *before, char *in, const char *after,
					char *out)
{
	const unsigned int linelen = 66;
	char wrk[PAGE], buf[PAGE], line[LINE];
	buf[0] = 0;
	memset(wrk, 0, PAGE);	// will be looking beyond input data
	strcpy(wrk, in);
	char *cp = wrk;
	while (strlen(cp) >= linelen) {
		char *ep = cp + linelen - 1;
		while(*ep == 0 || !isspace(*ep)) ep--;
		*ep = 0;
		sprintf(line, "%s%s%s", before, cp, after);
		strcat(buf, line);
		cp = ep + 1;
	}
	if (strlen(cp)) {
		sprintf(line, "%s%s%s", before, cp, after);
		strcat(buf, line);
	}
	strcpy(out, buf);
} // groupsetwidth()

bool testbyline(char *fro, char *to, char *maintag, char *vartag, 
				int *lineno)
{
	char *lp = fro;
	char mopn[LINE], mcls[LINE], vopn[LINE], vcls[LINE];
	maketags_r(maintag, mopn, mcls);
	maketags_r(vartag, vopn, vcls);
	int ln = 0;
	size_t molen = strlen(mopn);
	size_t volen = strlen(vopn);

	while (1) {
		ln++;	// make line numbers match those in the editor
		*lineno = ln;
		char *ep = memchr(lp, '\n', to - lp);
		if (!ep) ep = to;	// file has no '\n' at end
		size_t llen = ep - lp;
		char *mtp = memmem(lp, llen, mopn, molen);	// in line only
		if (mtp) {
			char *line = gettagdata(mopn, mcls, mtp, to);
			if (strlen(line)) {	// line == "" is fine, nothing to do
				char *vp = memmem(mtp, to - mtp, vopn, volen);
				char *sline = gettagdata(vopn, vcls, vp, to);
				/* I have data between maintags but no data between 
				 * vartags. This is the error I want to catch. */
				if (!strlen(sline)) {
					free(sline);
					return false;
				}
				free(sline);
			}
			free(line);
		}
		lp = ep + 1;
		if (lp > to) break;
	} // while(1)
	return true;
} // testbyline()

char *gettagdata(const char *opn, const char *cls, 
					const char *fro, const char *limit)
{	/* It is easy to have a pair of tags like this:
	*	<sometag>\n\t\t</sometag>
	* where there are many whitespace chars in between but no
	* actual data.
	* This deals with such a case.
	*/
	char *line = NULL;
	char *back = NULL;
	char *ret;
	size_t tlen = strlen(opn);
	char *op = memmem(fro, limit - fro, opn, tlen);
	if (!op) tagerror(opn);	// will abort
	char *clp = memmem(op, limit - op, cls, tlen+1);
	if (!clp) tagerror(cls);	// will abort
	op += tlen;
	size_t buflen = clp - op;
	if (buflen) {
		line = docalloc(buflen + 1, 1, "gettagdata()");
		back = docalloc(buflen + 1, 1, "gettagdata()");
		strncpy(line, op, clp - op);
		cleanuptext_r(line, back);
		ret = dostrdup(back);
	} else {
		ret = dostrdup("");
	}
	if (line) free(line);
	if (back) free(back);
	return ret;
} // gettagdata

void maketags_r(const char *tagname, char * opntag, char *clstag)
{
	sprintf(opntag, "<%s>", tagname);
	sprintf(clstag, "</%s>", tagname);
} // maketags_r()

void validatexml(const char *datafile)
{	/* handles the formatting and results of the test. */
	bool result;
	int lineno;
	fdata filedat = readfile(datafile, 0, 1);
	result = testbyline(filedat.from, filedat.to, "goptvariable",
						"gname", &lineno);
	if (!result) {
		fprintf(stderr, 
		"In file %s after line %d empty <%s> exists\n",
				datafile, lineno, "gname");
		exit(EXIT_FAILURE);
	}
	result = testbyline(filedat.from, filedat.to, "optvariable",
						"vname", &lineno);
	if (!result) {
		fprintf(stderr, 
		"In file %s after line %d empty <%s> exists\n",
				datafile, lineno, "vname");
		exit(EXIT_FAILURE);
	}
	
	// check that no more options have been added after the closing tag.
	lineno = 1;
	char *cp = filedat.from;
	while (1) {
		char *el = memchr(cp, '\n', filedat.to - cp);
		if (!el) {
			el = filedat.to;
		}
		lineno++;
		char *op = 
				memmem(cp, el - cp, "</options>", strlen("</options>"));
		if (op) {
			op += 10;
			char *extra = 
						memmem(op, filedat.to - op, "<name>", 
						strlen("<name>"));
			if (extra) {
				fprintf(stderr, "You have written options past "
								"</options> tag at line: %d\n", lineno);
				fprintf(stderr, "Please edit %s to put </options> tag "
								"on the last line of the file.\n", 
								datafile);
				exit(EXIT_FAILURE);
			} else {
				break;	// this potential problem doesn't exist.
			}
		}
		cp = el + 1;
		if (cp > filedat.to) break;
	}	// while(1)	
	free(filedat.from);
}  // validatexml()

void generatemakefile(char *xmlfile, const char *template)
{
	char *datafile = get_realpath_home(template);
	fdata mydat = readfile(datafile, 0, 1);
	strdata sd = getdatafromtagnames(mydat.from, mydat.to, "text");
	*sd.to = 0;	// now it's a format string
	char exefile[LINE];
	strcpy(exefile, xmlfile);
	char *ftyp = strstr(exefile, ".xml");
	*ftyp = 0;
	size_t fnlen = strlen(exefile);
	const int numexrefs = 9;
	size_t outlen = sd.to - sd.from + numexrefs * fnlen;
	char *outbuf = docalloc(outlen, 1, "generatemakefile");
	/* Below: The alternative, doing it a line at a time, is even worse
	 * IMO.
	*/
	sprintf(outbuf, sd.from, exefile, exefile, exefile, exefile,
			exefile, exefile, exefile, exefile, exefile);
	writefile("makefile.opt", outbuf, NULL, "w");
	free(outbuf);
	free(mydat.from);
} // generatemakefile()

void freegvsdata(govars_t *gvsdat)
{
	free(gvsdat->deflt);
	free(gvsdat->gname);
	free(gvsdat->type);
} // freegvsdata()

void freevarsdata(vars *vardat)
{
	free(vardat->deflt);
	free(vardat->type);
	free(vardat->vname);
} // freegvsdata()
void freeoptsdata(optdata_t *optdat)
{
	free(optdat->code);
	free(optdat->helptext);
	free(optdat->longname);
	free(optdat->optarg);
	free(optdat->shortname);
	free(optdat->synopsis);
} // freeoptsdata()
