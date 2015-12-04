/*
 * The main program for the Bank.
 *
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
    memset(key,'\0',32);
    unsigned char decrypted[10000];

    FILE *file;
    //memset(recvline,'\0',10000);
    //char user_input[1000];
    file=fopen(argv[1],"r");
    char *split=strtok(argv[1],".");
    split=strtok(NULL,".");
    if(file==0 || strcmp(split,"bank")){
        printf("Error opening ATM initialization file\n");
        return 64;
    }

    fread(key,sizeof(key),32,file);
    key[32]='\0';
    //printf("bank file contents: %s\n",key);
    HashTable *users = hash_table_create(100);
    HashTable *balance = hash_table_create(100);
    Bank *bank = bank_create();
    //bank->users = list_create();
    //bank->usr_key = hash_table_create(100);
    //bank->usr_bal = hash_table_create(100);
    printf("%s", prompt);
    fflush(stdout);

    while(1)
    {
        memset(decrypted,'\0',10000);
        memset(recvline,'\0',10000);
        //memset(sendline,'\0',10000);
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(bank->sockfd, &fds);
        select(bank->sockfd+1, &fds, NULL, NULL, NULL);

        if(FD_ISSET(0, &fds)){
            fgets(sendline, 10000,stdin);
            bank_process_local_command(bank, sendline, strlen(sendline),users,balance);
            printf("%s", prompt);
            fflush(stdout);
        }else if(FD_ISSET(bank->sockfd, &fds)){;
            //int flag = 0;

            n = bank_recv(bank, recvline, 10000);


            //int flag=decrypt(recvline,key,decrypted,n);

            if(!decrypt(recvline,key,decrypted,n)){ //this means that it has not been decrypted correctly so it will return a null packet
                unsigned char encrypted[10000];
                char packet[10000];
                sprintf(packet,"<%s>",NULL);
                int out_size =0;
                encrypt(packet,key,encrypted,&out_size);
                bank_send(bank, encrypted, out_size);
                printf("%s", prompt);
                fflush(stdout);
                continue;
            }

            //printf("%s\n",decrypted);
            char * message=strtok(decrypted,"\n");

            bank_process_remote_command(bank, message, n, users,key,balance);

        }

    }
    hash_table_free(balance);
    hash_table_free(users); //never executes
    bank_free(bank);
    //fclose(file);

    return EXIT_SUCCESS;
}
