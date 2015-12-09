#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
	if(argc != 2){
		printf("Usage: init <filename>\n");
		return 62;
	}

	int len = strlen(argv[1]);
	char *isBank = malloc(sizeof(char) * (len+6));
	memcpy(isBank,argv[1], len);
	memcpy(isBank+len, ".bank", 5);
	isBank[len+5] = '\0';

	FILE *fp;
	//check that bank or atm file exists
	if((fp = fopen(isBank,"r")) != NULL) {
		fclose(fp);
		printf("Error: one of the files already exists\n");
		return 63;
	}

	char *isAtm = malloc(sizeof(char) * (len+5));
	memcpy(isAtm,argv[1], len);
	memcpy(isAtm+len, ".atm", 4);
	isAtm[len+4] = '\0';


	if((fp = fopen(isAtm,"r")) != NULL) {
		fclose(fp);
		printf("Error: one of the files already exists\n");
		return 63;
	}

	FILE *fpbank;
	fpbank = fopen(isBank, "w");
	if(fpbank == NULL){
		fclose(fpbank);
		printf("Error creating initialization files\n");
		return 64;
	}

	fclose(fpbank);

	FILE *fpatm;
	fpatm = fopen(isAtm, "w");
	if(fpatm == NULL){
		fclose(fpatm);
		printf("Error creating initialization files\n");
		return 64;
	}

	fclose(fpatm);


	printf("Successfully initialized bank state\n");
	return 0;
}
