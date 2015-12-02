#include "bank.h"
#include "ports.h"
#include "hash_table.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <openssl/evp.h>


#define MAX_ARG1_LEN 11 //cmd
#define MAX_ARG2_LEN 250 //usrname
#define MAX_ARG3_LEN 5 //pin or amt
#define MAX_ARG4_LEN 11 //unsigned int max 4294967295
#define MAX_LINE_LEN 1000
#define TRUE 1;
#define FALSE 0;

Bank* bank_create()
{
    Bank *bank = (Bank*) malloc(sizeof(Bank));
    if(bank == NULL)
    {
        perror("Could not allocate Bank");
        exit(1);
    }

    // Set up the network state
    bank->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&bank->rtr_addr,sizeof(bank->rtr_addr));
    bank->rtr_addr.sin_family = AF_INET;
    bank->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&bank->bank_addr, sizeof(bank->bank_addr));
    bank->bank_addr.sin_family = AF_INET;
    bank->bank_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->bank_addr.sin_port = htons(BANK_PORT);
    bind(bank->sockfd,(struct sockaddr *)&bank->bank_addr,sizeof(bank->bank_addr));

    // Set up the protocol state
    // TODO set up more, as needed

    //user profile

    return bank;
}

void bank_free(Bank *bank)
{
    if(bank != NULL)
    {
        close(bank->sockfd);
        free(bank);
    }
}

ssize_t bank_send(Bank *bank, char *data, size_t data_len)
{
    // Returns the number of bytes sent; negative on error
    return sendto(bank->sockfd, data, data_len, 0,
            (struct sockaddr*) &bank->rtr_addr, sizeof(bank->rtr_addr));
}

ssize_t bank_recv(Bank *bank, char *data, size_t max_data_len)
{
    // Returns the number of bytes received; negative on error
    return recvfrom(bank->sockfd, data, max_data_len, 0, NULL, NULL);
}

void bank_process_local_command(Bank *bank, char *command, size_t len, HashTable *users,HashTable *balance)
{
    //creating larger buffers to prevent overflow
    char arg1[MAX_ARG1_LEN], arg2[MAX_ARG2_LEN], arg3[MAX_ARG3_LEN], arg4[MAX_ARG4_LEN];
    char arg1buff[MAX_LINE_LEN], arg2buff[MAX_LINE_LEN], arg3buff[MAX_LINE_LEN], arg4buff[MAX_LINE_LEN], end[MAX_LINE_LEN];


    memset(arg1,'\0',MAX_ARG1_LEN); 
    memset(arg2,'\0',MAX_ARG2_LEN);
    memset(arg3,'\0',MAX_ARG3_LEN);
    memset(arg4,'\0',MAX_ARG4_LEN);

    //full command too long
    if (strlen(command) >= MAX_LINE_LEN){
        printf("Invalid command\n");
        return;
    }

    size_t args_num = sscanf(command, "%s %s %s %s %s", arg1buff, arg2buff, arg3buff, arg4buff, end);

    // null input
    if (strlen(arg1buff) < 1 || args_num==1){

        printf("Invalid command\n");
        return;
    }

    //check if args are correct len
    if (strlen(arg1buff) > MAX_ARG1_LEN){
        //printf("HERE\n");
        printf("Invalid command\n");
        return;
    }else{
        //memset(arg1,'\0',MAX_ARG1_LEN);
        strcpy(arg1, arg1buff);
        //printf("\"%s\"\n",arg1);
    }      

    // MAIN BRANCHING BASED ON ARG1


    //create-user <user-name> <pin> <balance>

    if (strcmp(arg1, "create-user") == 0){
        //printf("Initial users hash is size: %d\n",hash_table_size(users));
        if(args_num != 4){
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }

        //empty
        if (strlen(arg2buff) < 1 || strlen(arg3buff) < 1 || strlen(arg4buff) < 1){ 
            //printf("empty arguments\n");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }

        //username max 
        if (strlen(arg2buff) > MAX_ARG2_LEN){  
            //printf("username length too large\n");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }else{
            //strncpy(arg2, arg2buff,strlen(arg2buff));
            strcpy(arg2, arg2buff);
        }

        //valid user name
        if (!valid_user(arg2)){
            //printf("user not valid\n");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }


        //user exists?
        if (user_exists(arg2, users)){
            printf("Error: user %s already exists\n", arg2);
            return;
        }

        //pin len max
        if (strlen(arg3buff) != 4){
            //printf("pin wrong length\n");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }else{
            strncpy(arg3, arg3buff,strlen(arg3buff));
            //strcpy(arg3, arg3buff);

        } 
        //valid pin
        if(!valid_pin(arg3)){
            //printf("not valid pin\n");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }
        //balance str max
        if (strlen(arg4buff) > MAX_ARG4_LEN){
            //printf("balance too large");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }else{
            strncpy(arg4, arg4buff,strlen(arg4buff));
            //strcpy(arg4, arg4buff);
        }

        //valid balance
        if (!valid_balance(arg4)){
            //printf("balance not valid\n");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        } 

        char *ptr;
        long long bal = strtoll(arg4, &ptr, 10);
        
        ///////////////
        //MAKING CARD//
        ///////////////

        //CARD FILENAME
        int fn_len = strlen(arg2) + strlen(".card") + 1;
        char file[fn_len];
        memset(file, '\0', fn_len);
        strncpy(file, arg2, strlen(arg2));
        strncat(file, ".card", strlen(".card"));

        //char *user_name[strlen(arg2)];
        char *user_name = malloc(strlen(arg2));
        char *user_name_card = malloc(strlen(arg2));
        memset(user_name, '\0', strlen(arg2));
        memset(user_name_card, '\0', strlen(arg2));
        strcpy(user_name, arg2);
        strcpy(user_name_card, arg2);

        //making card
        unsigned char *key= malloc(32); 
        generate_key(key);
        FILE *card_file=fopen(file,"w");

        if (card_file==0){
            printf("Error creating card file for user %s\n",user_name);
            remove(card_file);
        }else{
            //encrypt with key and write to card file
            //add encryption to hash
            //add user to hash

            //CARD CONTENTS

            char* semi = strcat(arg2, ";");
            char* card = strcat(semi, arg3);
            printf("writing %s to card\n",card);
            unsigned char encrypted[10000];
            int out_size =0;
            encrypt(card,key,encrypted,&out_size);

            fwrite(encrypted,1,out_size,card_file);

            //INSERTING INTO BANK
            printf("Inserting \"%s\" => \"%s\"\n", user_name, key);
            //list_add(bank->users,user_name,key);
            hash_table_add(users, user_name, key);
            printf("Inserting \"%s\" => \"%u\"\n",user_name, (unsigned int)bal);
            hash_table_add(balance, user_name, (unsigned int)bal);

            printf("Created user %s\n", user_name);
            fclose(card_file);
            free(user_name_card);
        }
    }else if (strcmp(arg1, "deposit") == 0){

        //null
        if (!arg2buff || !arg3buff){
            printf("Usage: deposit <user-name> <amt>\n");
            return;
        }
        if(args_num != 3){
            printf("Invalid command\n"); 
            return;
        }

        //empty
        if (strlen(arg2buff) < 1 || strlen(arg3buff) < 1){
            printf("Usage: deposit <user-name> <amt>\n");
            return;
        }   

        //user max
        if (strlen(arg2) > MAX_ARG2_LEN){
            printf("Usage: deposit <user-name> <amt>\n");
            return;
        }else{
            strncpy(arg2, arg2buff, strlen(arg2buff));
        }

        //valid user
        if (!valid_user){
            printf("Usage: deposit <user-name> <amt>\n");
            return;
        }

        //user DNE    
        if (!user_exists(arg2, users)){
            printf("No such user\n");
            return;
        }

        //amt len
        if (strlen(arg3buff) > MAX_ARG4_LEN){
            printf("Usage: deposit <user-name> <amt>\n");
            return;
        }

        //all digits
        if (!all_digits(arg3buff)){
            printf("Usage: deposit <user-name> <amt>\n");
            return;
        }

        //printf("PASSED ALL THE CHECKS\n");

        char *p;
        long long new_bal;

        long long amt = strtoll(arg3buff, &p, 10);
        if (amt < 0){
            printf("Usage: deposit <user-name> <amt>\n");
            return;
        } 

        unsigned int curr_bal = hash_table_find(balance, arg2);
        
        if (amt > UINT_MAX || (amt + (long long)curr_bal) > UINT_MAX){
           printf("Too rich for this program\n");
           return; 
        }

        new_bal = amt + (long long)curr_bal;
        printf("%lld\n", new_bal);

        char *user = malloc(251);
        memset(user,'\0',251);
        strncpy(user,arg2,strlen(arg2));

        //printf("about to add %lu to %s's current balance of %u\n", amt, arg2, curr_bal);
        if(new_bal < 0 || new_bal < curr_bal || new_bal > UINT_MAX){
            printf("Too rich for this program\n");
            return;
        }

        //valid balance if able to deposit
        if (!valid_balance(arg3buff)){
            printf("Usage: deposit <user-name> <amt>\n");
            return;      
        }

        //CHANGE THE AMOUNT IN BANK
        //printf("curr balance: %u\n", curr_bal);
        //printf("amount: %lu\n", amt);
        //printf("lets add: %u\n", amt+curr_bal);

        //printf("before deposit %s's balance is %u\n", arg2, curr_bal);
        //printf("arg2: %s\n", arg2);

        hash_table_del(balance, user);
        hash_table_add(balance, user, (unsigned int)new_bal);
        printf("new_bal: %u\n", (unsigned int)new_bal);
        printf("%s's balance is now %u\n", arg2, hash_table_find(balance, arg2));
        
        printf("$%lld added to %s's account\n", amt, user);
        free(user);
        return;
    }else if (strcmp(arg1, "balance") == 0){

        //null
        //if (!arg2buff){printf("Usage: balance <user-name>\n"); return;}
        if(args_num != 2){
            printf("Usage: balance <user-name>\n"); 
            return;
        }        

        //empty or max len
        if (strlen(arg2buff) < 1 || strlen(arg2buff) > 250){
            printf("Usage: balance <user-name>\n");
            return;
        }

        //valid user
        if (!valid_user(arg2buff)){
            printf("Usage: balance <user-name>\n");
            return;
        }

        char *user = malloc(251);
        memset(user,'\0',251);
        strncpy(user,arg2buff,strlen(arg2buff)); 

        if (!user_exists(user, users)){
            printf("No such user\n");
            return;
        }    

        //RETRIEVE BALANCE

        printf("retrieving balance\n");
        puts(arg2);
        printf("%s's balance is now %u\n", arg2buff, hash_table_find(balance, user));
        printf("$simons balance is%u\n", hash_table_find(balance, "simon"));
        
        return;
    }else{
        printf("Invalid command\n");
    }        
}

void bank_process_remote_command(Bank *bank, char *command, size_t len, HashTable *users, char *key, HashTable *balance)
{
    //char sendline[1000];
    unsigned char encrypted[10000];
    memset(encrypted,'\0',10000);
    command[len] = 0;
    char * packet = malloc(10000);
    memset(packet,'\0',10000);

    printf("Received: %s\n",command);
    //sprintf(sendline, "Bank got: %s", command);

    char *last = &command[strlen(command)-1];  //FIX THIS
    if(!strcmp(last,">") && command[0]=='<'){
        printf("This is a full packet\n");
    }
    command=&command[1];
    char *rem_last= strtok(command,">");
    char *comm=strtok(rem_last,"|");
    printf("the function is %s\n",comm);

    //authentication <authentication|"name">
    if (!strcmp(comm,"authentication")){
        comm = strtok(NULL,"|");
        if(comm ==NULL){
            printf("ERROR packet not in correct format\n");
        }
        //printf("asking for authentification for %s\n",comm);
        char* card_key = hash_table_find(users, comm);
        printf("The card_key for %s is %s\n",comm,card_key);
        if(card_key==NULL){
            //printf("key not found\n");
            char *str="<authentication|not found>";
            sprintf(packet,"%s",str);
        }else{
            sprintf(packet,"<authentication|%s>",card_key);       
        }
        printf("sending packet: %s\n",packet);
        int out_size=0;
        encrypt(packet,key,encrypted,&out_size);
        bank_send(bank, encrypted, out_size);
    }

    //withdraw <withdraw|"name"|"amount">
    else if (!strcmp(comm,"withdraw")){
        comm = strtok(NULL,"|");
        if(comm ==NULL){
            printf("ERROR packet not in correct format\n");
            //generate response
        }
        char *user=malloc(250); //SIZE OF USERNAME
        memset(user,'\0',250);
        strncpy(user,comm,strlen(comm));
        comm = strtok(NULL,"|");
        if(comm ==NULL){
            printf("ERROR packet not in correct format\n");
            //generate response
        }
        //printf("Withdrawing %s from %s\n",comm,user);

        printf("balance for %s is %u\n",user, hash_table_find(balance,user));

        unsigned int val = malloc(sizeof(unsigned int));
        unsigned int tmp = (unsigned int)hash_table_find(balance,user);
        unsigned int withdraw_amt=atol(comm);
        //unsigned int withdraw_amt=(unsigned int)temp;
        if(withdraw_amt < 0 || withdraw_amt > tmp){
            strcpy(packet,"<Insufficient funds>");
            printf("sending packet: %s\n",packet);
        }else{
            val=tmp-withdraw_amt;
            //printf("The new val is %u\n",val);
            hash_table_del(balance,user);
            hash_table_add(balance,user,val);
            strcpy(packet,"<withdraw_successful>");
            printf("sending packet: %s\n",packet);
            printf("balance for %s is %u\n",user, hash_table_find(balance,user));

        }
        int out_size=0;
        encrypt(packet,key,encrypted,&out_size);
        bank_send(bank, encrypted, out_size);
        //free(user);
        //user=NULL;
    }

    //balance <balance|"name"|amt>
    else if (strcmp(comm,"balance") == 0){
        comm = strtok(NULL,"|");
        if(comm == NULL){
            printf("ERROR packet not in correct format\n");
        }
        char *user = malloc(251);
        memset(user,'\0',251);
        strncpy(user,comm,strlen(comm));
        unsigned int *bal = hash_table_find(balance,user);
        printf("Looking for balance for %s  It is %u\n",user,bal);
        sprintf(packet,"<balance|%s|%u>",user,hash_table_find(balance,user));
        printf("sending packet: %s\n",packet);
        int out_size=0;
        encrypt(packet,key,encrypted,&out_size);
        bank_send(bank, encrypted, out_size);
        free(user);
        user=NULL;
    }

    free(packet);
}

int valid_user(char *user_name){
    //have already check if null
    if(!user_name){
        return 0;
    }

    int i;
    for (i = 0; i < strlen(user_name); i++){
        int ascii=(int)user_name[i];
        //printf("%d\n",ascii);
        if(ascii < 65 || ascii > 122){
            return 0;
        }else if(ascii > 90 &&  ascii < 97){
            return 0;
        }
    }
    return 1;
}

int user_exists(char *user_name,HashTable *users){
    if(hash_table_find(users, user_name)==NULL){
        //printf("user does not exist already\n");
        return 0;
    }
    return 1;
}

int valid_pin(char *pin){
    if (!all_digits(pin)){
        printf("not all digits\n");
        return FALSE;
    }
    char *ptr;

    unsigned long num = strtoul(pin, &ptr, 10);
    if (num < 0 || num > 9999){
        return FALSE;
    }
    return TRUE;
}

int valid_balance(char *bal){
    if (!all_digits(bal)){
        return FALSE;
    }
    char *ptr;
    long long num = strtoll(bal, &ptr, 10);

    if (num < 0 || num > UINT_MAX){
        return FALSE;
    }
    return TRUE;
}

int all_digits(char *number){
    int i;
    for(i=0; i<strlen(number); i++){
        int ascii = (int)number[i];
        if (ascii < 48 || ascii > 57){
            return 0;
        }
    }
    return 1;
}
void encrypt(char *message,char*key,unsigned char*encrypted,int *out_size){
    EVP_CIPHER_CTX ctx;
    memset(encrypted,'\0',10000);
    unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    EVP_CIPHER_CTX_init(&ctx);
    EVP_EncryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key, iv);
    int len1,tmplen;
    if(!EVP_EncryptUpdate(&ctx,encrypted,&len1,message,strlen(message))){
        printf("Encrypt Update Error\n");
    }
    if(!EVP_EncryptFinal(&ctx,encrypted+len1,&tmplen)){
        printf("Encrypt Final Error\n");
    }
    len1+=tmplen;
    *out_size=len1;
    EVP_CIPHER_CTX_cleanup(&ctx);

}

int decrypt(unsigned char *message,char*key, unsigned char*decrypted, int cipher_size){
    //printf("ciphertext size if %d\n",cipher_size);
    EVP_CIPHER_CTX ctx;
    int ret =1;
    memset(decrypted,'\0',10000);
    unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    EVP_CIPHER_CTX_init(&ctx);
    EVP_DecryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key, iv);
    int len1;
    if(!EVP_DecryptUpdate(&ctx,decrypted,&len1,message,cipher_size)){
        printf("Decrypt Update Error\n");
        ret=0;
    }
    if(!EVP_DecryptFinal(&ctx,decrypted+len1,&len1)){
        printf("Decrypt Final Error\n");
        printf("%s\n",decrypted);
        ret=0;

    }
    //char * mess=strtok(decrypted,"\n");
    //decrypted=mess;
    EVP_CIPHER_CTX_cleanup(&ctx);
    return ret;
}

void generate_key(char *key){
    int n;
    time_t t;
    const char charset[]="abcdefghijklmnopqrstuvwxyz";
    srand((unsigned)time(&t));
    for (n = 0; n < 32; n++){
        int k = rand() % 26;
        key[n]=charset[k];
    }
    key[32]='\0';
}

