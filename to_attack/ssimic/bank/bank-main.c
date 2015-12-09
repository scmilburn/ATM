/* 
 * The main program for the Bank.
 *
 * You are free to change this as necessary.
 */

#include <string.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include "bank.h"
#include "ports.h"

static const char prompt[] = "BANK: ";

int main(int argc, char**argv)
{
   int n;
   char sendline[1000];
   char recvline[1000];

   if(argc == 1){
	printf("Error opening bank initialization file\n");
	return 64;
   }

   int len = strlen(argv[1]);
   char *isBank = malloc(sizeof(char) * len);
   memcpy(isBank,argv[1], len);
   isBank[len] = '\0';

   FILE *fp;
   //check that bank file exists
   if((fp = fopen(isBank,"r")) == NULL) {
	printf("Error opening bank initialization file\n");
	return 64;
   }


   Bank *bank = bank_create();

   printf("%s", prompt);
   fflush(stdout);

   while(1)
   {
       fd_set fds;
       FD_ZERO(&fds);
       FD_SET(0, &fds);
       FD_SET(bank->sockfd, &fds);
       select(bank->sockfd+1, &fds, NULL, NULL, NULL);

       if(FD_ISSET(0, &fds))
       {
           fgets(sendline, 10000,stdin);
           bank_process_local_command(bank, sendline, strlen(sendline));
           printf("%s", prompt);
           fflush(stdout);
       }
       else if(FD_ISSET(bank->sockfd, &fds))
       {

           n = bank_recv(bank, recvline, 10000);
           bank_process_remote_command(bank, recvline, n);
	   fflush(stdout);
       }
   }
   free(isBank);
   bank_free(bank);
   return EXIT_SUCCESS;
}
