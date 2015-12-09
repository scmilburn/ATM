/* 
 * The main program for the ATM.
 *
 * You are free to change this as necessary.
 */

#include "atm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char prompt[] = "ATM: ";

int main(int argc, char **argv)
{
    char user_input[1000];

    //todo fix to search up file created by init
    FILE *fp = fopen(argv[1], "r");
    if( fp == NULL ) {
      printf("Error opening ATM initialization file\n");
      return 64;
    }
    fclose(fp);

    ATM *atm = atm_create(argv[1]);

    printf("%s", prompt);
    fflush(stdout);

    while (fgets(user_input, 10000,stdin) != NULL)
    {
	//remove newline
	char *pos;
	if((pos=strchr(user_input, '\n')) != NULL){
	   *pos = '\0';
	}

        char *prompt_state = atm_process_command(atm, user_input);
        if(prompt_state == NULL){
           printf("%s", prompt);
        }
        else{
           printf("ATM (%s): ",prompt_state);
        }
        fflush(stdout);
    }
	return EXIT_SUCCESS;
}
