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


#include <llmne.h>
#include <util.h>
#include <sys/stat.h>

int suppress_error = 0,nline = 0;

struct llmne_file llmne;

int
stroff(char* str,char c)
{
	int i;

	for(i = 0;i < strlen(str);i++)
		if(str[i] == c)
			return i;

	return -1;
}

int
main(int argc,char **argv)
{
	int i;
	char* file = NULL;
	char* line = NULL;
	int   execute = 0;

	init_signal();

	o_stream = stdout;
	
	while((i = getopt_long(argc, argv, "svho:x", long_options, NULL)) > 0)
	{
		switch(i)
		{
			case 's':
				suppress_error = 1;
				break;
			case 'v':
				die(VPRINT);
			case 'o':
				file = strdup(optarg);
				break;
			case 'x':
				execute = 1;
				break;
			case '?':
			case 'h':
			default:
				die(USAGE,argv[0]);
				break;
		}
	}

	banner();

	if(optind >= argc)
		die(USAGE,argv[0]);

	if(!(i_stream = fopen(argv[optind],"r")))
		xdie("fopen");

	if(file && !(o_stream = fopen(file,"w")))
		xdie("fopen");

	if(o_stream != stdout)
	{
		printf("Output redirected to %s...\n",file);

		close(1);
		if(dup(fileno(o_stream)) < 0)
			xdie("dup");
	}

	llmne.symbols = malloc(sizeof(struct llmne_sym));
	llmne.syms_len = 0;

	resolveSymbols();

	line = xmalloc(256);
	llmne.instr = xmalloc(sizeof(struct llmne_instr));
	llmne.instr_len = i = 0;

	while(fgets(line,256,i_stream))
	{
		line[strlen(line)-1] = '\0';

#ifdef _DEBUG
		printf("[DEBUG]  %d:%s\n",nline,line);
#endif

		if(line[0] == '#' || line[0] == '\0')
			continue;

		if(stroff(line,'#') != -1)
			line[stroff(line,'#')] = '\0';

		llmne_parse_all(line);
	}

#ifdef _DEBUG
	dump_symbols();
#endif

	printInstr();

	if(execute)
		lxs_execute();
	
	free(llmne.symbols);
	free(llmne.instr);
	free(line);
	free(file);

	if(o_stream != stdout)
		fclose(o_stream);

	return 0;
}
