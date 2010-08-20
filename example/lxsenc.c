#include <stdio.h>
#include <stdlib.h>

char payload[] = "0001\n0000\n0004\n1307\n3602\n1402\n1201\n1316\n2100\n1216\n1307\n2801\n3016\n3507\n3509\n2707\n";
int  p_size = sizeof(payload) / 5;

int i_need[] = {
	10,11,12,13,14,15,16,17,
	18,19,20,21,22,23,24,25,
	27,28,29,30,31,32,35,36,
	37,39,40
};

int n_need = sizeof(i_need) / sizeof(int);

int
getLine(FILE *fp)
{
	int ch,c = 0;

	while((ch = fgetc(fp)) != EOF)
		if(ch == '\n')
			c++;

	rewind( fp );

	return c;
}

int
main(int arg,char *argv[])
{
	int i,instr,fnd,lines;
	FILE* fp;
	char buf[BUFSIZ];

	if(!argv[1])
		return 1;

	fp = fopen(argv[1],"r");

	lines = getLine( fp );

	payload[13] = lines % 10 + '0';
	payload[12] = (lines /= 10) % 10 + '0';
	payload[11] = (lines /= 10) % 10 + '0';
	payload[10] = (lines /= 10) % 10 + '0';

	printf("%s", payload );

	while(fgets(buf,BUFSIZ,fp))
	{
		if(buf[0] == '#' || buf[0] == '\n' || buf[0] == 0)
			continue;

		instr = atoi( buf );

		for(i = fnd = 0;i < n_need;i++)
		{
			if((instr / 100) == i_need[i])
			{
				printf("%04d\n", (instr + 16) ^ 1);

				fnd = 1;
			}
		}

		if(!fnd)
			printf("%04d\n", instr ^ 1);
	}

	fclose(fp);

	return 0;
}
