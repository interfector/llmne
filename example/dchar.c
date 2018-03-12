#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
die(char* d)
{
	perror( d );
	exit(1);
}

char*
handle_char(char c)
{
	char* s = malloc(3);

	memset(s, 0, 3);

	switch(c)
	{
		case '\n':
			strncpy(s,"\\n", 2);
			break;
		case '\t':
			strncpy(s,"\\t", 2);
			break;
		case '\r':
			strncpy(s,"\\r", 2);
			break;
		default:
			s[0] = c;
			break;
	}

	return s;
}

int
main(int argc,char *argv[])
{
	FILE *fp;
	char c;

	if(argv[1])
	{
		if(!(fp = fopen(argv[1],"r")))
			die("[ERROR] fopen");
	} else
		fp = stdin;

	while((c = fgetc(fp)) != EOF)
		printf("%04d \'%s\'\n", c, handle_char(c));

	fclose(fp);

	return 0;
}
