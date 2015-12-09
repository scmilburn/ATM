/* 
 * The main program for the ATM.
 *
 * You are free to change this as necessary.
 */

#include "atm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

static const char prompt[] = "ATM: ";

int main(int argc, char**argv)
{
    char user_input[1000];

    if(argc == 1){
	printf("Error opening ATM initialization file\n");
	return 64;
   }

   int len = strlen(argv[1]);
   char *isBank = malloc(sizeof(char) * len);
   memcpy(isBank,argv[1], len);
   isBank[len] = '\0';

   FILE *fp;
   //check that bank file exists
   if((fp = fopen(isBank,"r")) == NULL) {
	printf("Error opening ATM initialization file\n");
	return 64;
   }
   fclose(fp);

    ATM *atm = atm_create();

    printf("%s", prompt);
    fflush(stdout);

    while (fgets(user_input, 300,stdin) != NULL)
    {	
	fflush(stdout);
	if(user_input[0] == '\n'){
		
	}else{
		atm_process_command(atm, user_input);
		if(atm->Session == 0){
			printf("%s", prompt);
		}
		else{
			printf("ATM (%s):", atm->name);
		}
		fflush(stdout);
	}
    }
    free(isBank);
	atm_free(atm);
	return EXIT_SUCCESS;
}
