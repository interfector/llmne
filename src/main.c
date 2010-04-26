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
main(int argc,char **argv)
{
	int i;
	char* file = NULL;
	char* line = NULL;

	TokenCtx tokens;
	struct llmne_sym * sym;

	init_signal();

	o_stream = stdout;
	
	while((i = getopt_long(argc, argv, "svho:", long_options, NULL)) > 0)
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

	llmne.symbols = resolveSymbols(&llmne.syms_len);

	line = xmalloc(256);

	i = 0;

	llmne.instr = xmalloc(sizeof(struct llmne_instr));

	while(fgets(line,256,i_stream))
	{
		nline++;
		line[strlen(line)-1] = '\0';

#ifdef _DEBUG
		printf("%d:%s\n",nline,line);
#endif

		if(line[0] == '#' || line[0] == '\0')
			continue;

		TokenParse(&tokens,line);

		llmne.instr = realloc(llmne.instr,++i * sizeof(struct llmne_instr));
		llmne.instr[i-1] = InstrParse(&tokens);

		if(llmne.instr[i-1].instr)
		{
			if(llmne.instr[i-1].instr_code == 27)
			{
				if((sym = searchSymbols(llmne.instr[i-1].ctx.args[0])))
				{
					if(sym->offset != llmne.instr[i-1].code)
						printf("%d  %s %s+%d\n",llmne.instr[i-1].opcode,
										    llmne.instr[i-1].instr,
										    llmne.instr[i-1].ctx.args[0],
										    llmne.instr[i-1].code - sym->offset);
				} else {
					printf("%d  %s %s\n",llmne.instr[i-1].opcode,
									 llmne.instr[i-1].instr,
									 llmne.instr[i-1].ctx.args[0]);
				}
			} else {
			printf("%d  %s %s,%s\n",llmne.instr[i-1].opcode,
							    llmne.instr[i-1].instr,
							    llmne.instr[i-1].ctx.args[0],
							    llmne.instr[i-1].ctx.args[1]);
			}
		}

		free(tokens.args);
		free(tokens.instr);
	}

	free(llmne.symbols);
	free(llmne.instr);

	if(o_stream != stdout)
		fclose(o_stream);

	return 0;
}
