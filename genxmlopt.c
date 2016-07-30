
/*      gopt.c
 *
 *  Copyright Robert L Parker 2016 rlp1938@gmail.com
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
#include "genxmlopt.h"


options_t process_options(int argc, char **argv)
{
	synopsis =
  "\tSYNOPSIS\n"
  "\tgenxml xmlfilename\n"
  "\tWhen run without options, initialises xmlfilename with -h option\n"
  "\tprocessing.\n"
  "\t\n"
  "\tgenxml -a OPTNAME,shortname,longname xmlfilename\n"
  "\t\n"
  "\tgenxml -c OPTNAME,shortname,longname xmlfilename\n"
  "\t\n"
  ;
	helptext =
  "\t-h, --help\n"
  "\tPrint this message and exit.\n"
  "\t-a, --add-xml\n"
  "\tRequires an optarg of the form, NAME,shortname,longname. Shortname or\n"
  "\tlongname may be zero length but not both. Appends an xml section to the\n"
  "\tnamed xml file. A non-existent xml file is an error.\n"
  "\t\n"
  "\t-c, --close-xml\n"
  "\tDoes the same as --add-xml but also adds the closing tag to the xml\n"
  "\tfile.\n"
  ;

	optstring = ":ha:c:";

	/* declare and set defaults for local variables. */

	/* set up defaults for opt vars. */
	options_t optdat;
	optdat.init = 1;
	optdat.add = 0;
	optdat.optnames = NULL;
	optdat.cls = 0;

	int c;
	int digit_optind = 0;

	while(1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
		{"help",	0,	0,	'h' },
		{"add-xml",	1,	0,	'a' },
		{"close-xml",	1,	0,	'c' },
		{0,	0,	0,	0 }
			};


		c = getopt_long(argc, argv, optstring,
                        long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 0:
			switch (option_index) {
			} // switch()
		break;
		case 'h':
		dohelp(0);
		break;
		case 'a':
			optdat.init = 0;
			optdat.add = 1;
			optdat.optnames = dostrdup(optarg);
		break;
		case 'c':
			optdat.cls = 1;
			optdat.init = 0;
			optdat.optnames = dostrdup(optarg);
		break;

		case ':':
			fprintf(stderr, "Option %s requires an argument\n",
					argv[this_option_optind]);
			dohelp(1);
		break;
		case '?':
			fprintf(stderr, "Unknown option: %s\n",
					 argv[this_option_optind]);
			dohelp(1);
		break;
		} // switch()
	} // while()
	return optdat;
} // process_options()

void dohelp(int forced)
{
  if(strlen(synopsis)) fputs(synopsis, stderr);
  fputs(helptext, stderr);
  exit(forced);
} // dohelp()

