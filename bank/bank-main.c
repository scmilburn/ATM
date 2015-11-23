/* 
 * The main program for the Bank.
 *
 * You are free to change this as necessary.
 */

#include <string.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include "bank.h"
#include "ports.h"
#include "hash_table.h"

static const char prompt[] = "BANK: ";

int main(int argc, char**argv)
{
   int n;
   char sendline[10000];
   char recvline[10000];
   char key[32];
   unsigned char decrypted[1000];
   
   FILE *file;
    //memset(recvline,'\0',10000);
   //char user_input[1000];
   file=fopen(argv[1],"r");
   if(file==0){
    	printf("Error opening bank initialization file\n");
	return 64;
   }
   fread(key,sizeof(key),32,file);
   printf("bank file contents: %s\n",key);
   HashTable *users = hash_table_create(100);
   HashTable *balance = hash_table_create(100);   
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
	   printf("listening for local commands only\n");
	    memset(recvline,'\0',10000);
           fgets(sendline, 10000,stdin);
           bank_process_local_command(bank, sendline, strlen(sendline),users,balance);
	   printf("Users hash is now size: %d\n",hash_table_size(users));
	   printf("Balance hash is now size: %d\n",hash_table_size(balance));
           printf("%s", prompt);
           fflush(stdout);
       }
       else if(FD_ISSET(bank->sockfd, &fds))
       {
	   memset(recvline,'\0',1000);
	   memset(decrypted,'\0',1000);

           n = bank_recv(bank, recvline, 1000);

            EVP_CIPHER_CTX ctx;
	    int decrypt_len;
	    unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	    EVP_CIPHER_CTX_init(&ctx);
	    EVP_DecryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key, iv);
	    int len1;
	    if(!EVP_DecryptUpdate(&ctx,decrypted,&len1,recvline,strlen(recvline))){
		printf("Decrypt Update Error\n");
	    }
	    decrypt_len=len1;
	    if(!EVP_DecryptFinal(&ctx,decrypted+len1,&len1)){
		printf("Decrypt Final Error\n");
						
	    }

	    printf("%s\n",decrypted);
	    char * message=strtok(decrypted,"\n");

            bank_process_remote_command(bank, message, n, users,key,balance);
 	    
      	    printf("%s", prompt);
            fflush(stdout);
	}
    	//printf("bob's balance is now %u\n",(unsigned int)hash_table_find(balance,"bob"));
   }
   hash_table_free(balance);
   hash_table_free(users); //never executes
   bank_free(bank);
  //fclose(argv[1]);

   return EXIT_SUCCESS;
}
