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
#define MAX_ARG4_LEN 5 //unsigned int max 65535
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

    bank->users = list_create();
    bank->usr_bal = hash_table_create(100);    
    bank->usr_pin = hash_table_create(100);

    return bank;
}

void bank_free(Bank *bank)
{
    if(bank != NULL)
    {
        list_free(bank->users);
        hash_table_free(bank->usr_bal);
        hash_table_free(bank->usr_pin);
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
    char arg1[MAX_ARG1_LEN+1], arg2[MAX_ARG2_LEN+1], arg3[MAX_ARG3_LEN+1], arg4[MAX_ARG4_LEN+1];
    char arg1buff[MAX_LINE_LEN], arg2buff[MAX_LINE_LEN], arg3buff[MAX_LINE_LEN], arg4buff[MAX_LINE_LEN];


    //checking command length before splitting
    if (strlen(command) >= MAX_LINE_LEN){
        printf("Invalid command\n");
        return;
    }

    size_t args_num = sscanf(command, "%s %s %s %s", arg1buff, arg2buff, arg3buff, arg4buff);

    //null input
    if (strlen(arg1buff) < 1 || args_num==1){
        printf("Empty value\n");
        printf("Invalid command\n");
        return;
    }

    //check if args are correct len
    if (strlen(arg1buff) > MAX_ARG1_LEN){
        printf("args too long\n");
        printf("Invalid command\n");
        return;
    }else{
        strcpy(arg1, arg1buff);
    }      

    // MAIN BRANCHING BASED ON ARG1

    //create-user <user-name> <pin> <balance>
    if (args_num == 4){
        if (strcmp(arg1, "create-user") == 0){
            printf("Initial users hash is size: %d\n", hash_table_size(users));

            //empty
            if (strlen(arg2buff) < 1 || strlen(arg3buff) < 1 || strlen(arg4buff) < 1){ 
                printf("empty arguments\n");
                printf("Usage: create-user <user-name> <pin> <balance>\n");
                return;
            }

            //username max 
            if (strlen(arg2buff) > MAX_ARG2_LEN){  
                printf("username length too large\n");
                printf("Usage: create-user <user-name> <pin> <balance>\n");
                return;
            }else{
                strcpy(arg2, arg2buff);
            }

            //valid user name
            if (!valid_user(arg2)){
                printf("user not valid\n");
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
                printf("pin wrong length\n");
                printf("Usage: create-user <user-name> <pin> <balance>\n");
                return;
            }else{
                strcpy(arg3, arg3buff);    
            } 

            //valid pin
            if(!valid_pin(arg3)){
                printf("not valid pin\n");
                printf("Usage: create-user <user-name> <pin> <balance>\n");
                return;
            }

            //balance max
            if (strlen(arg4buff) > MAX_ARG4_LEN){
                printf("balance too large");
                printf("Usage: create-user <user-name> <pin> <balance>\n");
                return;
            }else{
                strcpy(arg4, arg4buff);
            }

            //valid balance
            if (!valid_balance(arg4)){
                printf("balance not valid\n");
                printf("Usage: create-user <user-name> <pin> <balance>\n");
                return;
            } else{
                //unsigned int balance = strtol(arg4, NULL, 5);
            }

            //CREATING CARD

            //CARD FILENAME
            //username.card
            int fn_len = strlen(arg2) + strlen(".card") + 1;
            char file[fn_len];
            memset(file, '\0', fn_len);
            strncpy(file, arg2, strlen(arg2));
            strncat(file, ".card", strlen(".card"));
            
            //getting key and making file
            printf("creating card file %s\n",file);
            unsigned char key[32];
            generate_key(key);
            printf("The key for this card file is %s\n",key);
            FILE *card_file=fopen(file,"w");
            
            if (card_file==0){
                printf("Error creating card file for user %s\n", arg2);
                remove(card_file);

            }else{
                //encrypt with key and write to card file
                //add encryption to hashtable
                //add user to hashtable
                
                //CARD CONTENTS
                //username;
                char* temp=strcat(arg2,";");
                //username;pin
                char* card=strcat(temp,arg3);
                printf("THE CARD CONTENTS %s\n", card);

                unsigned char encrypted[10000];
                encrypt(card,key,encrypted);

                fwrite(encrypted,1,sizeof(encrypted),card_file);
                printf("Inserting \"%s\" => \"%s\"\n", arg2, key);
                hash_table_add(users, arg2, key);
                printf("Inserting \"%s\" => \"%s\"\n", arg2,arg4);
                hash_table_add(balance, arg2, atoi(arg4));

                printf("Created user %s\n", arg2);
                fclose(card_file);
            }
        }
    }
    //deposit <user-name> <amt> 
    /*else if (strcmp(arg1, "deposit") == 0){
    //user-name;
    char* temp=strcat(arg2,";");

    //user-name;pin  
    char* card=strcat(temp,arg3); //info to put on card

    //user-name.card
    char *file=strcat(user_name_cpy,".card");
    printf("creating card file %s with information %s\n",file,card);
    unsigned char key[32];
    generate_key(key);
    printf("The key for this card file is %s\n",key);

    FILE *card_file=fopen(file,"w");
    if (card_file==0){
    printf("Error creating card file for user %s\n", user_name);
    }else{
    //encrypt with key and write to card file
    //add encryption to hash
    //add user to hash
    unsigned char encrypted[10000];
    encrypt(card,key,encrypted);

    fwrite(encrypted,1,sizeof(encrypted),card_file);
    char * arr[3];
    arr[0]=arg3;
    arr[1]=arg4;
    arr[2]=key;
    hash_table_add(users, user_name, arr);
    printf("Created user %s\n", user_name);


    fclose(card_file);

    }
    free(user_name_cpy);
    free(user_name);
    //user_name_cpy=NULL;
    //user_name=NULL;

    }
    //deposit <user-name> <amt> 
    /*else if (strcmp(arg1, "deposit") == 0){

    //null
    if (!arg2buff || !arg3buff){
    printf("Usage: deposit <user-name> <amt>\n");
    return;
    }
    if(n != 3){
    printf("Invalid command\n"); 
    return;
    }

    //empty
    if (strlen(arg2buff) < 1 || strlen(arg3buff) < 1){
    printf("Usage: deposit <user-name> <amt>\n");
    return;
    }   

    //user max
    if (strlen(arg2) < 250){
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
    if (!user_exists(arg2)){
    printf("No such user\n");
    return;
    }

    //amt len
    if (strlen(arg3buff) > 5){
    printf("Usage: deposit <user-name> <amt>\n");
    return;
    }

    //all digits
    if (!all_digits(arg3buff)){
    printf("Usage: deposit <user-name> <amt>\n");
    return;
    }else if(strtol(arg3buff, NULL, 10) > UINT_MAX){
    printf("Too rich for this program\n");
    return;
    }

    strncpy(arg3, arg3buff, strlen(arg3buff));

    //valid balance if able to deposit
    if (!valid_balance){
    printf("Usage: deposit <user-name> <amt>\n");
    return;      
    }

    //check that new deposit won't overflow the balance

    //CHANGE THE AMOUNT IN BANK
    //figure out how to communicate with bank storage and make card with new balance        

    printf("$%s added to %s's account\n", arg3, arg2);
    return;
    }
    //balance <user-name>
    else if (strcmp(arg1, "balance") == 0){

    //null
    //if (!arg2buff){printf("Usage: balance <user-name>\n"); return;}
    if(n != 2){
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
    }else{
        strncpy(arg2, arg2buff, strlen(arg2buff));
    }

    if (!user_exists(arg2)){
        printf("No such user");
        return;
    }    

    //RETRIEVE BALANCE

    printf("retrieving balance\n");

    //get-balance(arg2);
    //printf("$%d\n", balance);
    return;
}*/else{
    printf("Invalid command\n");
}        
}

void bank_process_remote_command(Bank *bank, char *command, size_t len, HashTable *users,char *key,HashTable *balance)
{
    printf("Users hash is initial size: %d\n",hash_table_size(users));
    char sendline[1000];
    unsigned char encrypted[1000];
    command[len]=0;
    char * packet = malloc(1000);
    memset(packet,'\0',strlen(packet));

    printf("Received: %s\n",command);
    //sprintf(sendline, "Bank got: %s", command);

    char *last = &command[strlen(command)-1];
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
        char *name=comm;
        if(name ==NULL){
            printf("ERROR packet not in correct format\n");
        }
        printf("asking for authentification for %s\n",name);
        char* card_key = hash_table_find(users, name);
        if(card_key==NULL){
            printf("key not found\n");
        }else{
            sprintf(packet,"<authentication|%s>",card_key);
            printf("sending packet: %s\n",packet);

            encrypt(packet,key,encrypted);
            bank_send(bank, encrypted, strlen(encrypted));
        }
        char* val = hash_table_find(users, name);
        if(val==NULL){
            printf("key not found\n");
        }else{
            printf("The value is %s\n",val);
        }
    }

    //withdraw <withdraw|"name"|"amount">
    else if (!strcmp(comm,"withdraw")){
        comm = strtok(NULL,"|");
        char *user=comm;
        if(comm ==NULL){
            printf("ERROR packet not in correct format\n");
        }
        comm = strtok(NULL,"|");
        if(comm ==NULL){
            printf("ERROR packet not in correct format\n");
        }
        //printf("Withdrawing %s from %s\n",comm,user);
        int val = hash_table_find(balance,user);
        int withdraw_amt=atoi(comm);
        int new_balance=val-withdraw_amt;
        if(withdraw_amt < 0 || withdraw_amt > val){
            strcpy(packet,"<Insufficient funds>");
            printf("sending packet: %s\n",packet);
        }else{
            hash_table_del(balance,user);
            hash_table_add(balance,user,new_balance);
            printf("New hash value is %d\n",hash_table_find(balance,user));
            strcpy(packet,"<withdraw_successful>");
            printf("sending packet: %s\n",packet);

        }
        encrypt(packet,key,encrypted);
        bank_send(bank, encrypted, strlen(encrypted));
    }

    //balance <balance|"name"|amt>
    else if (!strcmp(comm,"balance")){
        comm = strtok(NULL,"|");
        if(comm ==NULL){
            printf("ERROR packet not in correct format\n");
        }
        printf("Looking for balance for %s\n It is %d\n",comm,hash_table_find(balance,comm));
        sprintf(packet,"<balance|%s|%d>",comm,hash_table_find(balance,comm));
        printf("sending packet: %s\n",packet);
        encrypt(packet,key,encrypted);
        bank_send(bank, encrypted, strlen(encrypted));
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
    //printf("Checking if %s exists\n",user_name);
    //printf("Finding -> %s\n", (hash_table_find(users, user_name) == NULL ? "Not Found" : "FAIL"));
    if(hash_table_find(users, user_name)==NULL){
        printf("user does not exist already\n");
        return 0;
    }
    return 1;
}

int valid_pin(char *pin){
    if (!all_digits(pin)){
        printf("not all digits\n");
        return FALSE;
    }

    long num = strtol(pin, NULL, 10);
    if (num < 0 || num > 9999){
        return FALSE;
    }
    return TRUE;
}

int valid_balance(char *bal){
    if (!all_digits(bal)){
        return FALSE;
    }
    long num = strtol(bal, NULL, 10);

    if (num < 0 || num >= UINT_MAX){
        return FALSE;
    }
    return 1;
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
void encrypt(char *message,char*key,unsigned char*encrypted){
    EVP_CIPHER_CTX ctx;
    memset(encrypted,'\0',1000);
    unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    EVP_CIPHER_CTX_init(&ctx);
    EVP_EncryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key, iv);
    int len1;
    if(!EVP_EncryptUpdate(&ctx,encrypted,&len1,message,strlen(message))){
        printf("Encrypt Update Error\n");
    }
    if(!EVP_EncryptFinal(&ctx,encrypted+len1,&len1)){
        printf("Encrypt Final Error\n");
    }
    EVP_CIPHER_CTX_cleanup(&ctx);

}

void decrypt(unsigned char *message,char*key, unsigned char*decrypted){
    EVP_CIPHER_CTX ctx;
    //unsigned char encrypted[10000];
    unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    EVP_CIPHER_CTX_init(&ctx);
    EVP_DecryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key, iv);
    int len1;
    if(!EVP_DecryptUpdate(&ctx,decrypted,&len1,message,strlen(message))){
        printf("Encrypt Update Error\n");
    }
    if(!EVP_DecryptFinal(&ctx,decrypted+len1,&len1)){
        printf("Encrypt Final Error\n");

    }
    //char * mess=strtok(decrypted,"\n");
    //decrypted=mess;
    EVP_CIPHER_CTX_cleanup(&ctx);
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

