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
#include "hash_table.h"

static const char prompt[] = "BANK: ";

int main(int argc, char**argv)
{
   int n;
   char sendline[1000];
   char recvline[1000];
   
   FILE *file;
   char user_input[1000];
   file=fopen(argv[1],"r");
   if(file==0){
    	printf("Error opening bank initialization file\n");
	return 64;
   }
   Bank *bank = bank_create();
   HashTable *users = hash_table_create(100);

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
	   printf("listening for local commands only\n");
           fgets(sendline, 10000,stdin);
           bank_process_local_command(bank, sendline, strlen(sendline),users);
           printf("%s", prompt);
           fflush(stdout);
       }
       else if(FD_ISSET(bank->sockfd, &fds))
       {
	   printf("socket successfully created\n");
           n = bank_recv(bank, recvline, 10000);
           bank_process_remote_command(bank, recvline, n, users);
       }
   }
   hash_table_free(users);

   return EXIT_SUCCESS;
}
