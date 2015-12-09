#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "functions.h"

int main(int argc, char**argv)
{
   if( argc != 2 ){
      printf("Usage: init <filename>\n");
      return 62;
   }
   else{
      
      //create enough space to add file extensions and \0
      char atmFile[strlen(argv[1]) + 5]; 
      strcpy(atmFile, argv[1]);
      strncat(atmFile, ".atm", 4);
      char bankFile[strlen(argv[1]) + 6];
      strcpy(bankFile, argv[1]);
      strncat(bankFile, ".bank", 5);

      if(access(atmFile, F_OK) != -1 || access(bankFile, F_OK) != -1){
         printf("Error: one of the files already exists\n");
         return 63;
      }
      else{
      
         FILE *atmFP = fopen(atmFile, "w");
	 FILE *bankFP = fopen(bankFile, "w");
         if(atmFP == NULL || bankFP == NULL){
            printf("Error creating initialization files\n");    
 	    return 64;
	 }
	 

	 //generate symmetric keys
	 srand((unsigned)time(NULL));
	 char *alphanum = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	 char key[51];
	 int i;
	 for (i = 0; i < 50; i++){
	    int letter = rand() % strlen(alphanum);
	    key[i] = alphanum [letter];
	 }
	 key[50] = '\0';

	 fprintf(atmFP, "%s", key);
	 fprintf(bankFP, "%s", key);

         fclose(atmFP);
         fclose(bankFP);

      }
   }
   printf("Successfully initialized bank state\n");  
   return 0;
}
