/* 
 * The main program for the ATM.
 *
 * You are free to change this as necessary.
 */

#include "atm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <hash_table.h>

static const char prompt[] = "ATM: ";

int main(int argc,char*argv[])
{
    FILE *file;
    char user_input[10000];
    memset(user_input,'\0',1000);
    char buffer[33];
    memset(buffer,'\0',33);
    
    file=fopen(argv[1],"r");
    char *split=strtok(argv[1],".");
    split=strtok(NULL,".");
    if(file==0 || strcmp(split,"atm")){
        printf("Error opening ATM initialization file\n");
        return 64;
    }
    fread(buffer,sizeof(buffer),32,file);
    buffer[32]='\0';

    //printf("atm file contents: %s\n",buffer);
    
    HashTable *tries = hash_table_create(100);

    ATM *atm = atm_create();

    printf("%s", prompt);
    fflush(stdout);

    char *ans;


    while (fgets(user_input, 10000,stdin) != NULL)
    {
        if(!strcmp(user_input,"\n") || !strcmp(user_input," ")){ //no input so keep looping 	
            continue;
        }
        
        ans= atm_process_command(atm, user_input,buffer, tries);
        
        if(!strcmp(ans,"")){
            printf("%s", prompt);
        }else{
            printf("ATM (%s): ",ans);
        }
        fflush(stdout);
	
    }
    hash_table_free(tries);
    atm_free(atm);
    return 0;
}
