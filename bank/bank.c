#include "bank.h"
#include "ports.h"
#include "hash_table.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <openssl/evp.h>


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

void bank_process_local_command(Bank *bank, char *command, size_t len, HashTable *users)
{
    //creating larger buffers to prevent overflow
    char arg1[MAX_ARG1_LEN], arg2[MAX_ARG2_LEN], arg3[MAX_ARG3_LEN], arg4[MAX_ARG4_LEN];
    char arg1buff[MAX_LINE_LEN], arg2buff[MAX_LINE_LEN], arg3buff[MAX_LINE_LEN], arg4buff[MAX_LINE_LEN];

	memset(arg1,'\0',MAX_ARG1_LEN);
        memset(arg2,'\0',MAX_ARG2_LEN);
	memset(arg3,'\0',MAX_ARG3_LEN);
	memset(arg4,'\0',MAX_ARG4_LEN);
    

    //full command too long
    if (strlen(command) >= MAX_LINE_LEN){
        printf("Invalid command\n");
        return;
    }
       
    int n = sscanf(command, "%s %s %s %s", arg1buff, arg2buff, arg3buff, arg4buff);
    
    //null input
    if (strlen(arg1buff) < 1 || n==1){
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
        strncpy(arg1, arg1buff, MAX_ARG1_LEN);
    }      
    
    // MAIN BRANCHING BASED ON ARG1


    //create-user <user-name> <pin> <balance>
    if (strcmp(arg1, "create-user") == 0){
        if(n != 4){ //checks if scanf read correct number of args
       	    printf("Invalid command\n"); 
	    return;
    	}
        
        //empty
        if (strlen(arg2buff) < 1 || strlen(arg3buff) < 1 || strlen(arg4buff) < 1){ 
	    printf("empty arguments\n");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }
        
        //username max 
        if (strlen(arg2buff) > 250){
	    printf("username length too large\n");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }else{
            strncpy(arg2, arg2buff, strlen(arg2buff));
        }
        
        //valid user name
        if (!valid_user(arg2)){
	    printf("user not valid\n");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }
        
        //user exists

        if (user_exists(arg2,users)){
            printf("Error: user %s already exists", arg2);
            return;
        }
        
        //pin len max
        if (strlen(arg3buff) != 4){
	    printf("pin not long enough\n");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }else{
            strncpy(arg3, arg3buff, strlen(arg3buff));    
        } 
        
        //valid pin
        if(!valid_pin(arg3)){
	    printf("not valid pin\n");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }
        
        //balance max
        if (strlen(arg4buff) > 5){
	    printf("balance too large");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        }else{
            strncpy(arg4, arg4buff, strlen(arg4buff));
        }
        
        //valid balance
        if (!valid_balance(arg4)){
	    printf("balance not valid\n");
            printf("Usage: create-user <user-name> <pin> <balance>\n");
            return;
        } else{
            //unsigned int balance = strtol(arg4, NULL, 5);
        }
        
	char *user_name_cpy=malloc(strlen(arg2)); //this for some reason does not get freed
	strncpy(user_name_cpy,arg2,strlen(arg2));
	printf("user_name copy is %s\n",user_name_cpy);

	char *user_name=malloc(strlen(arg2));
	strncpy(user_name,arg2,strlen(arg2));
	char* temp=strcat(arg2,";");
	char* card=strcat(temp,arg3);
	char *file=strcat(user_name_cpy,".card");
	printf("creating card file %s\n",file);
	//encrypt char *card
	
	unsigned char key[32];
	int n;
	time_t t;
        const char charset[]="abcdefghijklmnopqrstuvwxyz";
        srand((unsigned)time(&t));
        for (n = 0; n < 32; n++){
            int k = rand() % 26;
            key[n]=charset[k];
        }
        key[32]='\0';

	printf("The key for this card file is %s\n",key);
	FILE *card_file=fopen(file,"w");
	if (card_file==0){
		printf("Error creating card file for user %s\n",user_name);
	}else{
		//encrypt with key and write to card file
		//add encryption to hash
		//add user to hash
		EVP_CIPHER_CTX ctx;
		unsigned char encrypted[256];
		unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		EVP_CIPHER_CTX_init(&ctx);
		EVP_EncryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key, iv);
		int len1;
		if(!EVP_EncryptUpdate(&ctx,encrypted,&len1,card,strlen(card))){
			printf("Encrypt Update Error\n");
		}
		if(!EVP_EncryptFinal(&ctx,encrypted+len1,&len1)){
			printf("Encrypt Final Error\n");
						
		}
		printf("%s\n",encrypted);

		fwrite(encrypted,1,sizeof(encrypted),card_file);
		printf("Created user %s\n", user_name);
		EVP_CIPHER_CTX_cleanup(&ctx);
		
		fclose(card_file);

		hash_table_add(users, user_name, key);
	}
	free(user_name_cpy);
	free(user_name);
	user_name_cpy=NULL;
	user_name=NULL;
        memset(arg1,'\0',MAX_ARG1_LEN);
        memset(arg2,'\0',MAX_ARG2_LEN);
	memset(arg3,'\0',MAX_ARG3_LEN);
	memset(arg4,'\0',MAX_ARG4_LEN); 
	memset(file,'\0',strlen(file));
	memset(card,'\0',strlen(card));
	memset(temp,'\0',strlen(temp)); 
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
        memset(arg1,'\0',MAX_ARG1_LEN);
        memset(arg2,'\0',MAX_ARG2_LEN);
	memset(arg3,'\0',MAX_ARG3_LEN);
	memset(arg4,'\0',MAX_ARG4_LEN);
    }        
}

void bank_process_remote_command(Bank *bank, char *command, size_t len, HashTable *users)
{
    //begin-session <user-name>
        //withdraw <amt>
        //balance
        //end-session

	/*
	 * The following is a toy example that simply receives a
	 * string from the ATM, prepends "Bank got: " and echoes 
	 * it back to the ATM before printing it to stdout.
	 */

    // should be encrypting anything that is sent / decrypting anything taht is received
    //command is encrypted
     
    char sendline[1000];
    command[len]=0;

    
    printf("Received: %s\n",command);
    //fputs(command, stdout);
    sprintf(sendline, "Bank got: %s", command);
    //DECRYPT
    char packet[256];
    int n = sscanf(command, "<%s", packet);
    printf("the function is %s\n",packet);
    char * split=strtok(packet,"|");
    if (!strcmp(split,"authentication")){
        printf("asking for authentification\n");
    }
    

    bank_send(bank, sendline, strlen(sendline));
   
 /*   
    if (strcmp(arg1, "begin-session") == 0){
        if (successful login){
            if (strcmp(arg1, "withdraw") == 0){ 
                
            }else if (strcmp(arg1, "balance") == 0){
                //check if valid
                if (!valid_balance){
                    printf("Usage: balance");
                }else{
                    printf("$%d", get_balance);
                }
            }else if (strcmp(arg1, "end-session") == 0){
                printf("User logged out\n");   
            }else{
                bank_send(bank, failure, strlen(failure));
                printf("atm sent an invalid command:\n");
                fputs(command, stdout);
            }
        }else{
            printf("login failure");
        }
    }else if (strcmp(arg1, "withdraw") == 0){
        printf("No user logged in\n");    
    }else if (strcmp(arg1, "balance") == 0){
        printf("No user logged in\n");   
    }else if (strcmp(arg1, "end-session") == 0){
        printf("No user logged in\n");   
    }else{
        bank_send(bank, failure, strlen(failure));
        printf("atm sent an invalid command:\n");
        fputs(command, stdout);
    }

    
    //encrypt sendline before sending it
    bank_send(bank, sendline, strlen(sendline));
    printf("Received the following:\n");
    fputs(command, stdout);
*/

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
    printf("Checking is %s exists\n",user_name);
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
    
    long num = strtol(pin, NULL, 10);
    printf("Balance : %lu\n",num);
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

