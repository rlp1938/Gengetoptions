
/*      optgenopt.c
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
#include "optgenopt.h"


options_t process_options(int argc, char **argv)
{
	synopsis =
  "\tSYNOPSIS\n"
  "\toptgen xmlfilename\n"
  "\n"
  "\toptgen --help \n"
  "\n"
  "\tDESCRIPTION\n"
  "\toptgen before generating any C code validates the xmlfile and\n"
  "\taborts with an error message for each error found.\n"
  "\n"
  "\tThere is an xml section for each and every option. The sections\n"
  "\tbegin with a <name>xyz</name> pair where the text between tags is\n"
  "\tjust a documentary entry. This is followed by 'shortname' and\n"
  "\t'longname' tag pairs that contain the short and long option names\n"
  "\trespectively. Either name may be zero length but not both. These\n"
  "\tvalues have been set at creation time but the remainder must be\n"
  "\tinput by editing the file before generating any code.\n"
  "\n"
  "\tThis is followed by: <optvariable>\n"
  "\t<vname>xyz</vname>\n"
  "\t<type>xyz</type>\n"
  "\t<default>\n"
  "\t\t???\n"
  "\t</default>\n"
  "\t</optvariable>\n"
  "\n"
  "\tNow this group must be repeated for each and every options\n"
  "\tvariables that you want to specify. But if there are no options\n"
  "\tvariables needed then the intermediate tags must be removed.\n"
  "\tFailure to do that will cause a validate time error. All input\n"
  "\tbetween those tags must be legal C code including the terminating\n"
  "\t';' on the default assignment statement.\n"
  "\tThe next tag pair is <code></code>. Insert the C code to execute\n"
  "\twhen this option is selected. All options variables will be part\n"
  "\tof an options_t struct named 'opts' so all references to these\n"
  "\tvariables must be prefixed by 'opts.'\n"
  "\tThis is followed by <optarg></optarg>. If an options argument is\n"
  "\trequired '1' must be inserted here. By default the initial value\n"
  "\tis '0'.\n"
  "\tThis is then followed by <helptext></helptext>. This text is\n"
  "\treformatted to be 66 chars or less set between __\"\\t and \"\n"
  "\twhere __ is 2 spaces. You may force a line break anywhere you\n"
  "\twant by inserting '\\n' in the text. An empty line is designated\n"
  "\tby '\\n' on a line alone.\n"
  "\n"
  ;
	helptext =
  "\t-h, --help\n"
  "\tPrints this help message and exits.\n"
  ;

	optstring = ":h";

	/* declare and set defaults for local variables. */

	/* set up defaults for opt vars. */
	options_t opts;

	int c;
	int digit_optind = 0;

	while(1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
		{"help",	0,	0,	'h' },
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
	return opts;
} // process_options()

void dohelp(int forced)
{
  if(strlen(synopsis)) fputs(synopsis, stderr);
  fputs(helptext, stderr);
  exit(forced);
} // dohelp()

