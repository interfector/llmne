/* Disasassemble LXS pseudo-code to mnemonic instruction for LLMNE */
#include <stdio.h>
#include <stdlib.h>

#define MAGIC "SYM\x7a"

char symbol_start[] = "_START";

struct lxs_mne {
	int instr, opcode;

	char* mne;
};

struct lxs_mne instruction_set[] = {
	{ 10, 1, "READ" },
	{ 11, 1, "WRITE" },
	{ 12, 1, "POP" },
	{ 13, 1, "PUSH" },
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
	{ 33, 0, "EXIT" },
	{ 34, 0, "DISPLAY" },
	{ 35, 1, "INC" },
	{ 36, 1, "DEC" },
	{ 37, 1, "CALL" },
	{ 38, 0, "RET" },
	{ 39, 1, "STPUSH" },
	{ 40, 1, "STPOP" },
	{ 41, 1, "ADDSP" },
	{ 42, 1, "SUBSP" }
};

int i_size = sizeof(instruction_set) / sizeof(struct lxs_mne);

struct lxs_mne*
getStruct( int instr )
{
/*
	int i;

	for(i = 0;i < i_size;i++)
		if( instr == instruction_set[i].instr )
			return &instruction_set[i];
*/

	if( instr >= 10 && instr <= 42 )
		return &instruction_set[ instr - 10 ];
	
	return NULL;
}

int
main(int argc,char **argv)
{
	FILE *fp;
	char buf[BUFSIZ];

	int instr, opcode;

	struct lxs_mne* get;

	if(!argv[1])
		return 1;

	fp = fopen(argv[1],"r");

	if(!fp)
		return 1;

	printf("%s:\n", symbol_start );

	while(fgets(buf,BUFSIZ,fp))
	{
		if( buf[0] == '#' || buf[0] == '\n' || buf[0] == '\0')
			continue;

		instr = atoi( buf ) / 100;
		opcode = atoi( buf ) % 100;

		if(instr < 10 || instr > 42)
		{
			printf("%02d%02d # VAR\n", instr, opcode);

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
			printf( "%s+%d\n", symbol_start, opcode);
		else
			printf("%d\n", opcode);
	}

	fclose( fp );

	return 0;
}
