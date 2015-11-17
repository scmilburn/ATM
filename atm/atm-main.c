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
    //memset(user_input,'\0',1000);
    char buffer[32];
    file=fopen(argv[1],"r");
    if(file==0){
    	printf("Error opening ATM initialization file\n");
	return 64;
    }
    fread(buffer,sizeof(buffer),32,file);
    printf("atm file contents: %s\n",buffer);
 
    ATM *atm = atm_create();

    printf("%s", prompt);
    fflush(stdout);
	
    //char *ans;
    
    while (fgets(user_input, 1000,stdin) != NULL)
    {
	
        char *ans= atm_process_command(atm, user_input,buffer);
	
	printf("answer is %s\n",ans);
	if(!strcmp(ans,"")){
        	printf("%s", prompt);
	}else{
		printf("%s(%s)", prompt,ans);
	}
	
        fflush(stdout);
    }
	return EXIT_SUCCESS;
}
