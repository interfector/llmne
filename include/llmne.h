/*
* LLMNE, last lol mnemonic
* Copyright (C) 2010 nex
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _LLMNE_
#define _LLMNE_

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

struct llmne_sym {
	int   offset;
	char* name;
};

struct llmne_instr {
	char*  instr;
	char** args;
	int argc;

	int instr_code;
	int opcode;
};

struct llmne_file {
	int lenght;

	struct llmne_instr * instr;
	int instr_len;

	struct llmne_sym * symbols;
	int syms_len;
};

static const struct option long_options[] =
{
	{"suppress", no_argument, 0, 's' },
	{"output", required_argument, 0, 'o'},
	{"version", no_argument, 0, 'v'},
	{"help", no_argument, 0, 'h'}
};

FILE* o_stream;
FILE* i_stream;

#define USAGE "Usage: %s [-n] [-o <file>] [-v] [-h] input_file\n" \
		    "\t-n|--suppress\tDo not exit finding an error.\n" \
		    "\t-o|--output\tSet the output file.\n"  \
		    "\t-v|--version\tShow the program version.\n" \
		    "\t-h|--help\tShow this help.\n"

#define VPRINT "llmne-"VERSION" Copyright(C) 2010 by nex\n" \
				"Released under the GPL v3.0 license.\n" \
			"This is free software: you are free to change and redistribute it.\n" \
			"There is NO WARRANTY, to the extent permitted by law.\n"

#endif
