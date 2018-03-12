/* Disasassemble LXS pseudo-code to mnemonic instruction for LLMNE */
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <memory.h>

#define MAGIC "SYM\x7a"

char symbol_start[] = "_START";

struct lxs_mne {
	int instr, opcode;

	char* mne;
};

struct llmne_sym {
	char* name;
	int pointer;
};

struct lxs_mne instruction_set[] = {
	{ 10, 1, "READ" },
	{ 11, 1, "WRITE" },
	{ 12, 1, "SAVE" },
	{ 13, 1, "LOAD" },
	{ 14, 1, "ADD" },
	{ 15, 1, "SUB" },
	{ 16, 1, "MUL" },
	{ 17, 1, "DIV" },
	{ 18, 1, "MOD" },
	{ 19, 1, "AND" },
	{ 20, 1, "OR"  },
	{ 21, 1, "XOR" },
	{ 22, 1, "NOT" },
	{ 23, 1, "SHL" },
	{ 24, 1, "SHR" },
	{ 25, 1, "DEL" },
	{ 26, 0, "NOP" },
	{ 27, 1, "JMP" },
	{ 28, 1, "CMP" },
	{ 29, 1, "JN"  },
	{ 30, 1, "JZ"  },
	{ 31, 1, "JM"  },
	{ 32, 1, "JG"  },
	{ 33, 1, "EXIT" },
	{ 34, 0, "DISPLAY" },
	{ 35, 1, "INC" },
	{ 36, 1, "DEC" },
	{ 37, 1, "CALL" },
	{ 38, 0, "RET" },
	{ 39, 1, "STPUSH" },
	{ 40, 1, "STPOP" },
	{ 41, 1, "ADDSP" },
	{ 42, 1, "SUBSP" },
	{ 43, 1, "ONESC" },
	{ 44, 1, "INT3" },
	{ 45, 1, "PUSHA" },
	{ 46, 1, "POPA" },
	{ 47, 1, "DEFSYNTAX" }

};

#define MAX_CALL 47
#define MAX_SYNTAX 10

char* syntaxCommand[MAX_SYNTAX];
int   syntax_counter = 0;

int maxCall = MAX_CALL;

struct lxs_mne*
getStruct( int instr )
{
	if( instr >= 10 && instr <= MAX_CALL )
		return &instruction_set[ instr - 10 ];
	
	return NULL;
}

struct llmne_sym*
parseSymbolTable( FILE* fp, int* n )
{
	char buf[BUFSIZ];
	int dim = 10;

	struct llmne_sym* syms = malloc(sizeof(struct llmne_sym) * dim);

	while(fgets(buf,BUFSIZ,fp))
	{
		if(strncmp(buf, "#SYM:",5))
			continue;

		if(*n == dim)
		{
			struct llmne_sym* tmp = malloc(sizeof(struct llmne_sym) * dim * 2);

			memcpy( tmp, syms, sizeof(struct llmne_sym) * dim );

			free( syms );

			syms = tmp;
			dim *= 2;
		}

		syms[*n].pointer = atoi(buf+5);
		syms[*n].name = strdup(strchr(buf,'|')+1);
		syms[*n].name[strlen(syms[*n].name)-1] = '\0';

		*n = *n + 1;
	}

	rewind( fp );

	return syms;
}

char*
searchSymbol( struct llmne_sym* symbols, int size, int offset )
{
	int i;

	for(i = 0;i < size;i++)
		if( symbols[i].pointer == offset )
			return symbols[i].name;

	return NULL;
}

int
main(int argc,char **argv)
{
	FILE *fp;
	char buf[BUFSIZ];
	char* ptr_name;

	int instr, opcode, rcode, nsymbol, current_ptr = 0;

	struct lxs_mne* get;
	struct llmne_sym* symbols;

	if(!argv[1])
		return 1;

	fp = fopen(argv[1],"r");

	if(!fp)
		return 1;

	symbols = parseSymbolTable( fp, &nsymbol );

	printf("%s:\n", symbol_start );

	while(fgets(buf,BUFSIZ,fp))
	{
		if( buf[0] == '#' || buf[0] == '\n' || buf[0] == '\0')
			continue;

		if( (ptr_name = searchSymbol( symbols, nsymbol, current_ptr )) )
			printf("%s:\n", ptr_name );

		rcode = atoi( buf );
		instr = rcode / 100;
		opcode = rcode % 100;

		if(instr < 10 || instr > maxCall)
		{
			if( 33 <= rcode && rcode <= 127 )
				printf("%02d%02d # VAR ( '%c' )\n", instr, opcode, rcode);
			else
				printf("%02d%02d # VAR\n", instr, opcode);

			current_ptr++;

			continue;
		}

		if( instr == 34 )
		{
			printf("DISPLAY ");

			switch( opcode )
			{
				case 0:
					puts("INT");
					break;
				case 1:
					puts("HEX");
					break;
				case 2:
					puts("BIN");
					break;
				case 3:
					puts("CHAR");
					break;
				default:
					puts("INT");
					break;
			}

			current_ptr++;

			continue;
		} else if ( instr == 45 ) {
			printf("DEFSYNTAX FOO%d, %02d\n", syntax_counter, opcode);

			syntaxCommand[syntax_counter] = malloc(sizeof(char)*10);
			sprintf(syntaxCommand[syntax_counter], "FOO%d", syntax_counter );

			syntax_counter++;
			maxCall++;

			continue;
		} else if (instr > MAX_CALL ) {
			printf("%s %d\n", syntaxCommand[instr-MAX_CALL-1], opcode );

			continue;
		}

		get = getStruct( instr );

		if(!get)
		{
			printf("* Instruction not found.\n");
			exit(1);
		}

		printf("%s ", get->mne );

		if(get->opcode)
		{
			if( (ptr_name = searchSymbol( symbols, nsymbol, opcode )) )
				printf( "%s\n", ptr_name);
			else if ( abs(opcode-current_ptr) <= 3 )
				printf( "$$%c%d\n", (opcode - current_ptr >= 0) ? '+' : '-', abs(opcode - current_ptr) );
			else
				printf( "%s+%d\n", symbol_start, opcode);
		} else
			putchar('\n');

		current_ptr++;
	}

	fclose( fp );

	return 0;
}
