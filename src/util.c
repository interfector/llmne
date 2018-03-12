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

char* trim(char*);
void llmne_preprocess( char* );
void llmne_parse_all( char* );

void
handle(int sig,siginfo_t *si, void* unused)
{
	printf("\nSIGSEGV at <0x%lx>.\n",(long)si->si_addr);
	
	exit(1);
}

void
init_signal(void)
{
	struct sigaction sa;

	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = handle;

	if(sigaction(SIGSEGV,&sa,NULL) == -1)
		xdie("sigaction");
}

void*
xmalloc(int size)
{
	void* p = malloc(size);

	if(!p)
		xdie("malloc");

	return p;
}

void
die(char* fmt,...)
{
	va_list v;

	va_start(v,fmt);

	vfprintf(stderr,fmt,v);

	va_end(v);

	exit(1);
}

void
banner(void)
{
	printf(
		" _ _                \n"
		"| | |_ __  _ _  ___ \n"
		"| | | '  \\| ' \\/ -_)\n"
		"|_|_|_|_|_|_||_\\___|\n"
		"                    \n");
}

void
varprintf( char* buf )
{
	int i,j;
	int len;
	char* tmp;
	extern char* i_file;

	if( !buf )
		return;

	len = strlen( buf );

	for(i = 0;i < len;i++)
	{
		if( buf[i] == '$' )
		{
			if ( strchr( buf + i, ' ' ) )
				j = (strchr( buf + i, ' ')) - ( buf + i );
			else
				j = len - i;

			tmp = malloc( j );
			memset( tmp, 0, j );
			memcpy( tmp, buf + i + 1, j - 1 );

			if(!strcmp( tmp, "LINE" ))
				printf("%d", nline );
			else if (!strcmp(tmp, "IFILE"))
				printf("%s", i_file );
			else {
				if(searchSymbols(tmp))
					printf("%d", searchSymbols(tmp)->offset );
				else
					putchar('$');
			}

			i += strlen( tmp );
		} else
			putchar( buf[i] );
	}
}

char*
strreplace( char* start, char* nid, char* rep )
{
	char* replaced;
	char* temp;
	char* found;

	found = strstr( start, nid );

	if( !found )
		return NULL;

	replaced = found;

	temp = strdup( found + strlen(nid) );

	replaced = malloc( (found - start) + strlen(rep) + strlen(temp) + 1);

	memcpy( replaced, start, (found - start) );
	strcat( replaced, rep );
	strcat( replaced, temp );

	free( temp );

	return replaced;
}

void
parseIncludeFile( char* file )
{
	char* line = xmalloc(256);
	FILE* in;

	if(!(in = fopen(file, "r")))
		die("Couldn't open include file '%s'.\n", file );

	while(fgets(line, 256, in))
	{
		line[strlen(line)-1] = 0;

		if( line[0] == 0 || line[0] == '#' )
			continue;

		if(strchr(line,'#'))
			*((char*)strchr(line,'#')) = 0;

		#ifdef _DEBUG
			printf("%d:%s\n", nline, line);
		#endif

		if( line[0] == '@')
		{
			llmne_preprocess( line );
			continue;
		}

		llmne_parse_all( line );
	}

	fclose( in );
}

struct llmne_instr*
getMacroInstr( int nline, int *size, int arg, char** args )
{
	char* line = xmalloc( 256 );
	struct llmne_instr* instr = xmalloc(sizeof(struct llmne_instr));
	TokenCtx in;
	int i;
	char* tmp, *rep;

	*size = 0;

	tmp = xmalloc( 10 );
	memset( tmp, 0, 10 );

	while(fgets(line, 256, i_stream))
	{
		if(!strncmp( line, "@ENDM", 4 ))
			break;

		line[strlen(line)-1] = '\0';

		if(strchr(line, '#'))
			*((char*)strchr(line, '#')) = 0;
/*
		if( line[0] == '%' )
			die("* Syntax error, couldn't use recursive macro at line %d.\n",nline);
*/
		line++;

		for(i = 0;i < arg;i++)
		{
			memset( tmp, 0, strlen(tmp) );

			sprintf(tmp, "$%d", i );

			if(strstr(line, tmp))
			{
				rep = strreplace( line , tmp, args[i] );

				line = strdup(rep);
			}
		}

		TokenParse( &in, line );

		instr = realloc( instr, ++*size * sizeof(struct llmne_instr));
		instr[*size - 1] = InstrParse( &in );
	}

	return instr;
}

void
makeMacro( char* macro, int arg )
{
	int i, size;
	int old_nline = nline;
	struct llmne_instr* instrs;

	struct llmne_instr* backup;

	macro = trim(macro);

	for(i = 0;i < llmne.instr_len;i++)
	{
		if(!strcmp(llmne.instr[i].ctx.instr, macro))
		{
			nline = i;

			if( llmne.instr[i].ctx.argc < arg )
				die("* Syntax error, missing operand after macro at line: %d.\n",nline);

			instrs = getMacroInstr( nline, &size, arg, llmne.instr[i].ctx.args );

			backup = malloc( (llmne.instr_len - i) * sizeof(struct llmne_instr) );
			memcpy( backup, &llmne.instr[i + 1], (llmne.instr_len - i - 1 ) * sizeof(struct llmne_instr) );

			llmne.instr = realloc( llmne.instr, (llmne.instr_len + size) * sizeof(struct llmne_instr) );
			memcpy( &llmne.instr[i], instrs, size * sizeof(struct llmne_instr) );

			memcpy( &llmne.instr[i + size], backup, (llmne.instr_len - i - 1) * sizeof(struct llmne_instr) );

			llmne.instr_len += size - 1;

			free( backup );
			free( instrs );
		}
	}

	nline = old_nline;
}
			
char*
trim(char* string)
{
	char* trim = malloc(strlen(string));
	int i,k;

	memset(trim, 0, strlen(string));

	for(i = 0,k = 0;i < strlen(string);i++)
		if(string[i] != ' ' && 
		   string[i] != '\r' &&
 		   string[i] != '\n' &&
		   string[i] != '\t')
		trim[k++] = string[i];
		
	trim[k] = 0;

	return trim;
}

void
TokenParse(TokenCtx *ctx,char* line)
{
	char* token;

	ctx->line = strdup( line );

	if( line[0] == '@' )
		line++;

	token = strtok(line, " \t");

	if(!token)
	{
		if(suppress_error)
		{
			printf("* Syntax error, missing operand after instruction at line: %d.\n",nline);

			return;
		} else
			die("* Syntax error, missing operand after instruction at line: %d.\n",nline);
	}
	
	ctx->instr = strdup(token);
	ctx->args = (char**) malloc(sizeof(char*));
	ctx->argc = 0;

	while((token = strtok(NULL,",")))
	{
		ctx->args = (char**) realloc(ctx->args,sizeof(char*) * (ctx->argc + 1));
		ctx->args[ctx->argc++] = strdup(token);
	}

	if( ctx->line[0] != '@' )
	{
		if(ctx->line[strlen(ctx->line) - 1] == ':')
			handle_symbol(ctx->line, nline);
		else
			nline++;
	}
}

void
handle_symbol(char* line,int offset)
{
	char* dup;

	if(!line)
		return;

	dup = strdup(line);
	dup[strlen(dup)-1] = '\0';

//	if(searchSymbols(dup))
//		die("* Syntax error, duplicate symbols declaration at line %d.\n",offset);

	llmne.symbols = realloc(llmne.symbols,sizeof(struct llmne_sym) * ++llmne.syms_len);
	llmne.symbols[llmne.syms_len-1].name = strdup(dup);
	llmne.symbols[llmne.syms_len-1].offset = offset;

	free(dup);
}

/*
void
resolveSymbols( FILE* stream )
{
	char* line = xmalloc(256);
	TokenCtx ctx;

	llmne.symbols = realloc(llmne.symbols,sizeof(struct llmne_sym) * ++llmne.syms_len);
	llmne.symbols[llmne.syms_len-1].name = strdup("$$");
	llmne.symbols[llmne.syms_len-1].offset = 0;

	while(fgets(line,256,stream))
	{
		line[strlen(line)-1] = '\0';

		if(line[0] == '#' || line[0] == '\0' || line[0] == '@')
			continue;

		TokenParse(&ctx,line);
	}

	free(line);

	nline = 0;

	fseek(stream,0,SEEK_SET);
}
*/

void
relocateAllSymbols( void )
{
	int i;
	char* ptr;
	int offset;
	char* tmp;

	for(i = 0;i < llmne.instr_len;i++)
	{
		offset = 0;

		ptr = strtok(strdup(llmne.instr[i].ctx.line), " \t");
		ptr = strtok(NULL, " \t");

		if(!ptr)
			continue;

		if(strstr(ptr, "$$"))
			continue;

		if((tmp = strchr(ptr,'+')))
		{
			offset = atoi( tmp + 1 );

			tmp[0] = 0;
		}

		if(searchSymbols(ptr))
			llmne.instr[i].opcode = llmne.instr[i].instr_code * 100 + searchSymbols(ptr)->offset + offset;
	}
}

struct llmne_sym*
searchSymbols(char* name)
{
	int i,k;
	char* ptr;

	if(strchr(name,' ') || strchr(name,'\t'))
	{
		ptr = malloc(strlen(name));

		memset(ptr,0,strlen(name));

		for(i = 0,k = 0;i < strlen(name);i++)
			if(name[i] != ' ' && name[i] != '\t')
				ptr[k++] = name[i];
	} else
		ptr = strdup(name);

#ifdef _DEBUG
	printf("[DEBUG] Searched symbols: %s.\n",ptr);
#endif

	for(i = 0;i < llmne.syms_len;i++)
		if(!strcmp(llmne.symbols[i].name,ptr))
			return &llmne.symbols[i];

	return NULL;
}

struct llmne_instr
newInstr(TokenCtx *ctx,int op,int code)
{
	struct llmne_instr instr;

	instr.instr = strdup(ctx->instr);

	memcpy(&instr.ctx,ctx,sizeof(TokenCtx));
	
	instr.instr_code = op;
	instr.code = code;
	instr.opcode = op * 100 + code;

	return instr;
}

struct llmne_instr
InstrParse(TokenCtx* ctx)
{ /* TODO adds STORE,PRINT,SET and fix 
	the multiplce action instruction and 
	the address calculation */

	int i, offset = 0;
	char* ptr = NULL;

	if(searchSymbols("$$"))
		(searchSymbols("$$"))->offset = nline - 1;

	int size = sizeof(instruction_set) / sizeof(struct lxs_mne);

	if( !strcmp( ctx->instr, "DISPLAY" ) )
	{
		if(!strcmp(ctx->args[0],"INT"))
			offset = 0;
		else if(!strcmp(ctx->args[0],"HEX"))
			offset = 1;
		else if(!strcmp(ctx->args[0],"BIN"))
			offset = 2;
		else if(!strcmp(ctx->args[0],"CHAR"))
			offset = 3;
		else if(!strcmp(ctx->args[0],"STRING"))
			offset = 4;
		else
			offset = 0;

		return newInstr(ctx, 34, offset);
	} else if( !strcmp( ctx->instr, "DEFSYNTAX" ) ) {
		if( ctx->argc < 2 )
			die("Error parsing DEFSYNTAX function at %d line.\n", nline);

		syntaxCommand[syntax_counter].name = strdup(ctx->args[0]);
		syntaxCommand[syntax_counter].opcode = syntax_counter+MAX_CALL+1;

		syntax_counter++;

		return newInstr(ctx, MAX_CALL /* opcode for DEFSYNTAX ALWAYS */, atoi(ctx->args[1]));
	} else if( !strcmp( ctx->instr, "$FETCH" ) ) {
		struct llmne_sym* sym;

		if( ctx->argc < 1 )
			die("Error parsing FETCH function at %d line.\n", nline);

		return newInstr(ctx, 0, 0);
	}

	for(i = 0;i < size;i++)
	{
		if( !strcmp( ctx->instr, instruction_set[i].mne ) )
		{
			if( instruction_set[i].opcode )
			{
				if((ptr = strchr(ctx->args[0],'+')))
				{
					offset = atoi( ptr + 1);
					ptr[0] = '\0';
				}

				if(searchSymbols(ctx->args[0]))
					return newInstr(ctx, instruction_set[i].instr, (searchSymbols(ctx->args[0]))->offset + offset);
				else
					return newInstr(ctx, instruction_set[i].instr , atoi(ctx->args[0]));
			} else
				return newInstr(ctx, instruction_set[i].instr , instruction_set[i].instr);
		}
	}

	for(i = 0;i < syntax_counter;i++)
	{
		if( !strcmp( ctx->instr, syntaxCommand[i].name ) )
			return newInstr(ctx, syntaxCommand[i].opcode, atoi(ctx->args[0]));
	}

	ptr = strdup(ctx->instr);

	int argc = ctx->argc;
	char** args = malloc( argc * sizeof(char**) );

	for(i = 0;i < argc;i++)
		args[i] = trim(ctx->args[i]);

	return (struct llmne_instr) { "VAR" ,{ "VAR",  ptr , args , argc }, 0, 0, atoi(ptr) };
}

void
llmne_preprocess( char* line )
{
	int i;
	TokenCtx token;

	TokenParse( &token, line );

	if(!strcmp(token.instr, "STORE"))
	{
		if( token.argc < 2 )
			die("Error parsing STORE function at %d line.\n", nline);

		if(searchSymbols(token.args[0]))
		{
			if(searchSymbols(token.args[1]))
				(searchSymbols(token.args[0]))->offset = searchSymbols(token.args[1])->offset;
			else
				(searchSymbols(token.args[0]))->offset = atoi(token.args[1]);
		} else
			die("Couldn't find the symbol \"%s\".\n", token.args[0]);
	} else if(!strcmp(token.instr, "ECHO")) {
		for(i = 0;i < token.argc;i++)
			varprintf( token.args[i] );

		putchar('\n');
	} else if(!strcmp(token.instr, "MACRO")) {
		if( token.argc < 2 )
			die("Error parsing MACRO function at %d line.\n", nline);

		makeMacro( token.args[0], atoi(token.args[1]) );
	} else if(!strcmp(token.instr, "INCLUDE")) {
		if( token.argc < 1 )
			die("Error parsing INCLUDE function at %d line.\n", nline);

		parseIncludeFile( token.args[0] );
	}
}

void
printInstr()
{
	int i;
	
	for(i = 0;i < llmne.instr_len;i++)
		printf("%04d  # %s\n",llmne.instr[i].opcode,
					   llmne.instr[i].ctx.line);
}

void
lxs_execute()
{
	FILE *fp;
	int i;

	if(!(fp = fopen("/tmp/llmne.o","w")))
		xdie("tmpfile");

	for(i = 0;i < llmne.instr_len;i++)
		fprintf(fp,"%04d  # %s\n",llmne.instr[i].opcode,
				llmne.instr[i].ctx.line);

	fclose(fp);

	putchar('\n');

	i = system("lxs -s /tmp/llmne.o");

	unlink("/tmp/llmne.o");
}

void
dump_symbols()
{
	int i;

	for(i = 0;i < llmne.syms_len;i++)
		printf("[DEBUG] |-- @ Name: %s.\n"
			  "[DEBUG] |-> @ Offset: %02d.\n",llmne.symbols[i].name,llmne.symbols[i].offset);
}

void
llmne_parse_all(char* line)
{
	TokenCtx tokens;
	
	TokenParse(&tokens,line);

	if(tokens.instr[strlen(tokens.instr)-1] != ':') {
		llmne.instr = realloc(llmne.instr,++llmne.instr_len * sizeof(struct llmne_instr));
		llmne.instr[llmne.instr_len-1] = InstrParse(&tokens);
	}

	free(tokens.args);
	free(tokens.instr);
}
