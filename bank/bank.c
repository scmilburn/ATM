#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#define MAX_ARG1_LEN 12 //cmd
#define MAX_ARG2_LEN 251 //usrname
#define MAX_ARG3_LEN 6 //pin or amt
#define MAX_ARG4_LEN 6 //unsigned int max 65535
#define MAX_LINE_LEN 1001
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

void bank_process_local_command(Bank *bank, char *command, size_t len)
{
    char arg1[MAX_ARG1_LEN], arg2[MAX_ARG2_LEN], arg3[MAX_ARG3_LEN], arg4[MAX_ARG4_LEN];
    char arg1buff[MAX_LINE_LEN], arg2buff[MAX_LINE_LEN], arg3buff[MAX_LINE_LEN], arg4buff[MAX_LINE_LEN];
    

    //full command too long
    if (strlen(command) >= MAX_LINE_SIZE){
        printf("Invalid command\n");
        return;
    }
       
    sscanf(command, "%s %s %s %s", arg1buff, arg2buff, arg3buff, arg4buff);
    
    //null input
    if (strlen(arg1) < 1 || arg1 == NULL){
        printf("Invalid command\n");
        return;
    }

    //check if args are correct len
    if (strlen(arg1buff) > MAX_ARG1_LEN){
        printf("Invalid command\n");
        return;
    }else{
        strncpy(arg1, arg1buff, MAX_ARG1_LEN);
    }      
    
    // BRANCHING BASED ON ARG1


    //create-user <user-name> <pin> <balance>
    if (strcmp(arg1, "create-user") == 0){
        
        //null
        if (!arg2buff || !arg3buff || !arg4buff){
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;        
        }
        
        //empty
        if (strlen(arg2buff) < 1 || strlen(arg3buff) < 1 || strlen(arg4buff) < 1){ 
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }
        
        //username max 
        if (strlen(arg2buff) > 250){
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }else{
            strncpy(arg2, arg2buff, strlen(arg2buff));
        }
        
        //valid user name
        if (!valid_user(arg2)){
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }
        
        //user exists
        if (user_exists(arg2)){
            printf("Error: user %d already exists", arg2);
            return;
        }
        
        //pin len max
        if (strlen(arg3buff) != 4){
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }else{
            strncpy(arg3, arg3buff, strlen(arg3buff));    
        } 
        
        if(!valid_pin(arg3)){
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }
        
     



        
    }
    //deposit <user-name> <amt> 
    else if (strcmp(arg1, "deposit") == 0){

    }
    //balance <user-name>
    else if (strcmp(arg1, "balance") == 0){
    
    }else{
        printf("Invalid command\n");
        return;
    }         
}

void bank_process_remote_command(Bank *bank, char *command, size_t len)
{
    // TODO: Implement the bank side of the ATM-bank protocol

	/*
	 * The following is a toy example that simply receives a
	 * string from the ATM, prepends "Bank got: " and echoes 
	 * it back to the ATM before printing it to stdout.
	 */

    	
    char sendline[1000];
    command[len]=0;
    sprintf(sendline, "Bank got: %s", command);
    bank_send(bank, sendline, strlen(sendline));
    printf("Received the following:\n");
    fputs(command, stdout);
	
}

int valid_user(char *user_name){
    //have already check if null
    if(!user_name){
        return FALSE;
    }
    
    int i;
    for (i = 0; i < strlen(user_name); i++){
        if(user_name[i] < 65 || user_name[i] > 122){
            return FALSE;
        }else if(user_name[i] > 90 || user_name[i] < 97){
            return FALSE;
        }
    }
    return TRUE;
}

int user_exists(char *user_name){
    //check if they have a card
    //char fname = <user-name>.card 

    if (access(fname, F_OK) != -1){
        return TRUE;
    }else{
        return FALSE;
    }
}

int valid_pin(const char *pin){
    while(*pin){
        if (!isdigit(*pin++)){
            return FALSE;
        }
    }   
    
    long d = strtol(pin, NULL, 10);
    if (d < 0 || d > 9999){
        return FALSE;
    }
    return TRUE;
}
