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
	   printf("Users hash is now size: %d\n",hash_table_size(users));
           printf("%s", prompt);
	hash_table_free(users);
           fflush(stdout);
       }
       else if(FD_ISSET(bank->sockfd, &fds))
       {
	   //printf("socket successfully created\n");
           n = bank_recv(bank, recvline, 10000);

            EVP_CIPHER_CTX ctx;
	    unsigned char decrypted[10000];
	    int decrypt_len;
	    memset(decrypted,'\0',strlen(decrypted));
	    unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	    EVP_CIPHER_CTX_init(&ctx);
	    EVP_DecryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key, iv);
	    int len1;
	    if(!EVP_DecryptUpdate(&ctx,decrypted,&len1,recvline,strlen(recvline))){
		printf("Encrypt Update Error\n");
	    }
	    decrypt_len=len1;
	    printf("length of decryption is %d\n",decrypt_len);
	    if(!EVP_DecryptFinal(&ctx,decrypted+len1,&len1)){
		printf("Encrypt Final Error\n");
						
	    }

	    //printf("%s\n",decrypted);
	    char * message=strtok(decrypted,"\n");
	    printf("message recieved is: %s\n",message);

            bank_process_remote_command(bank, message, n, users,key);
       }
	
   }
   hash_table_free(users); //never executes
   bank_free(bank);
  //fclose(argv[1]);

   return EXIT_SUCCESS;
}
