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
  char *bankPath;
  FILE *bankFile;
  char *second;
  int filenamelen = 0;
  Bank *bank = bank_create();


  if(argc == 1) {
    printf("Usage: Bank <filename>\n");
    return 62; 
  } //user fails to provide exactly one argument

  second = argv[1];

  filenamelen = strlen(second);

  bankPath = malloc(1 + filenamelen + 5);

  strncpy(bankPath, second, strlen(second));

  bankPath[filenamelen] = '.';
  bankPath[filenamelen+1] = 'b';
  bankPath[filenamelen+2] = 'a';
  bankPath[filenamelen+3] = 'n';
  bankPath[filenamelen+4] = 'k';
  bankPath[filenamelen+5] = 0;

  bankFile = fopen(bankPath, "r");

  if(bankFile == NULL){
    printf("Error opening bank initialization file\n");
    return 64;
  }

  fclose(bankFile);

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
       }
   }

   return EXIT_SUCCESS;
}
