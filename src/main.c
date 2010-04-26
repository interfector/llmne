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

		InstrParse(&tokens);

		free(tokens.args);
		free(tokens.instr);
	}

	if(o_stream != stdout)
		fclose(o_stream);

	return 0;
}
