/* 
 * The main program for the ATM.
 *
 * You are free to change this as necessary.
 */

#include "atm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char prompt[] = "ATM: ";

int main(int argc,char*argv[])
{
    FILE *file;
    char user_input[1000];
    file=fopen(argv[1],"r");
    if(file==0){
    	printf("Error opening ATM initialization file\n");
	return 64;
    }
    
    ATM *atm = atm_create();

    printf("%s", prompt);
    fflush(stdout);
	//printf("HERE\n");

    
    while (fgets(user_input, 10000,stdin) != NULL)
    {
        char *ans= atm_process_command(atm, user_input);
	if(!strcmp(ans,"")){
        	printf("%s", prompt);
	}else{
		printf("%s (%s)", prompt,ans);
	}
        fflush(stdout);
	//printf("%s", prompt);
    }
	return EXIT_SUCCESS;
}
