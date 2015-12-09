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

int main(int argc, char**argv)
{
    char user_input[1000];
    ATM *atm = atm_create();
    char *atmPath;
    FILE *atmFile;
    char *second;
    int filenamelen = 0;


    if(argc == 1) {
    printf("Usage: ATM <filename>\n");
    return 62; 
    } //user fails to provide exactly one argument

    second = argv[1];

    filenamelen = strlen(second);

    atmPath = malloc(1 + filenamelen + 4);

    strncpy(atmPath, second, strlen(second));

    atmPath[filenamelen] = '.';
    atmPath[filenamelen+1] = 'a';
    atmPath[filenamelen+2] = 't';
    atmPath[filenamelen+3] = 'm';
    atmPath[filenamelen+4] = 0;

    atmFile = fopen(atmPath, "r");

    if(atmFile == NULL){
    printf("Error opening atm initialization file\n");
    return 64;
    }

    fclose(atmFile);

    printf("%s", prompt);
    fflush(stdout);

    while (fgets(user_input, 10000,stdin) != NULL)
    {
        atm_process_command(atm, user_input);

        if(atm->currentUser != NULL) {
            printf("ATM (%s): ", atm->currentUser->name);
        }
        else {
            printf("%s", prompt);
        }
        
        fflush(stdout);
    }
	return EXIT_SUCCESS;
}
