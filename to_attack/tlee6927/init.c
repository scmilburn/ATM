#include "ports.h"
#include "bank/bank.h"
#include "atm/atm.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <dirent.h> 

static unsigned char key[32];
static unsigned char iv[16];

int main(int argc, char**argv) {
	struct timeval blah;
	int i;
	char *second;
	char *bankPath, *atmPath;
	FILE *bankFile, *atmFile;
	int filenamelen = 0;

	if(argc != 2) {
		printf("Usage: init <filename>\n");
		return 62; 
	} //user fails to provide exactly one argument

	second = argv[1];

	gettimeofday(&blah, NULL);
	srand(blah.tv_sec);

	for(i = 0; i < 32; i++) {
		key[i] = rand() % 256;
	} //filling the key with random nums

	for(i = 0; i < 16; i++) {
		iv[i] = rand() % 256;
	} //filling the iv with random nums

	filenamelen = strlen(second);

	bankPath = malloc(1 + filenamelen + 5);
	atmPath = malloc(1 + filenamelen + 4);

	strncpy(bankPath, second, strlen(second));
	strncpy(atmPath, second, strlen(second));

	bankPath[filenamelen] = '.';
	bankPath[filenamelen+1] = 'b';
	bankPath[filenamelen+2] = 'a';
	bankPath[filenamelen+3] = 'n';
	bankPath[filenamelen+4] = 'k';
	bankPath[filenamelen+5] = 0;

	atmPath[filenamelen] = '.';
	atmPath[filenamelen+1] = 'a';
	atmPath[filenamelen+2] = 't';
	atmPath[filenamelen+3] = 'm';
	atmPath[filenamelen+4] = 0;

	if(access(bankPath, F_OK) != -1 || access(atmPath, F_OK) != -1) {
		printf("Error: one of the files already exists\n");

		free(bankPath);
		free(atmPath);

		return 63;
	} else {
	    bankFile = fopen(bankPath, "w+");
		atmFile = fopen(atmPath, "w+");

		if(bankFile != NULL || atmFile != NULL){
			fwrite(key, 1, 32, bankFile);
			fwrite(iv, 1, 16, bankFile);
			fwrite(key, 1, 32, atmFile);
			fwrite(iv, 1, 16, atmFile);

			printf("Successfully initialized bank state\n");

			fclose(bankFile);
			fclose(atmFile);
		}
		else{
			printf("Error creating initialization files\n");

			free(bankPath);
			free(atmPath);
			return 64;
		}
	}

	free(bankPath);
	free(atmPath);

	return 0;
}