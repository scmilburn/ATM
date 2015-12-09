#include "bank.h"
#include "ports.h"
#include "functions.h"
#include "hash_table.h"
#include "list.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

int MAX_BUF_SIZE = 5000;

Bank* bank_create(char *filename)
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

    bank->ht = hash_table_create(1000);
    bank->pin = hash_table_create(1000);
    bank->failed_attempts = hash_table_create(1000);
    bank->card_info = hash_table_create(1000);

    char *key = malloc(sizeof(char)*51);
    FILE *fp = fopen(filename, "r");
    if( fp == NULL ) {
      printf("Error opening bank initialization file\n");
      return NULL;
    }
    fgets(key, 51, fp);
    key[50] = '\0';

    bank->key = key;
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
    // TODO: Implement the bank's local commands
    char create_user[] = "create-user";
    char deposit[] = "deposit";
    char balance[] = "balance";
	
    //create a new string with a null terminator

    char args[strlen(command)+1];
    strncpy(args,command,strlen(command));
    args[strlen(command)] = '\0';

    int num_words = numWords(args);
    char **split = splitCommand(args);

    //The command is to create a user
    if(strcmp(create_user,split[0])==0){
        if(num_words != 4){
             printf("%s\n","Usage: create-user <user-name> <pin> <balance>"); 
             return;
        }
        //Checks for valid username
        if(isValidUsername(split[1])==1){
            //Checks for valid pin
            if(isValidPin(split[2])==1){
                //Checks for valid balance
                if(isValidBalance(split[3])==1){
                    //Checks if user already exists
		    if(hash_table_find(bank->ht,split[1])!=NULL){
                        printf("Error: user %s already exists\n",split[1]);
                        return;
                    }
                    //User does not exist
                    else{

			//Create <username>.card file
                        char file_name[strlen(split[1]) + 6];
			strncpy(file_name, split[1], strlen(split[1]));
                        strncat(file_name,".card", 5);

                        FILE *card = fopen(file_name,"w");
                        if(card == NULL){
                            printf("Error creating card file for user %s\n", split[1]);
                            hash_table_del(bank->ht,split[1]);
                            return;
                        }

			//generate random card number
			srand((unsigned)time(NULL));
                        char *num_vals = "0123456789";
                        char *card_number = malloc(sizeof(char) * 17);
                        int i;
                        for (i = 0; i < 16; i++){
                         	int letter = rand() % strlen(num_vals);
                          	card_number[i] = num_vals[letter];
                        }
                        card_number[16] = '\0';
			
			//write number to card
			fprintf(card, "%s", card_number);

			//add card number to user table
			hash_table_add(bank->card_info, split[1], card_number);

                        //add to hashtable with balance
			unsigned int *amt = malloc(sizeof(unsigned int));
			*amt= strtoul( split[3], NULL, 10) ;
                        hash_table_add(bank->ht, split[1], amt);

                        //add to hashtable with pin
                        hash_table_add(bank->pin,split[1],split[2]);

                        //add to hashtable with failed attempts
                        unsigned int *zero_tries = malloc(sizeof(unsigned int));
                        *zero_tries = 0;
                        hash_table_add(bank->failed_attempts, split[1], zero_tries);

                        //print success
                        printf("Created user %s\n", split[1]);
			fclose(card);
			return;
                    }
                }
                else{
                    printf("%s\n","Usage: create-user <user-name> <pin> <balance>"); 
                    return; 
                }
            }
            else{
                printf("%s\n","Usage: create-user <user-name> <pin> <balance>"); 
                return;
            }
        }
        else{
            printf("%s\n","Usage: create-user <user-name> <pin> <balance>"); 
            return;  
        }
    }
    else if(strcmp(deposit,split[0])==0){
        //The command is to make a deposit
        if(num_words != 3){
             printf("%s\n","Usage: deposit <user-name> <amt>"); 
             return;
        }
        //Checks for valid username
        if(isValidUsername(split[1])==1){
            //Checks for a valid amount
            if(isValidBalance(split[2])==1){
                //Checks if user is in the hashtable
                if(hash_table_find(bank->ht,split[1])==NULL){
                    printf("%s","No such user\n");
                    return;
                }
                //User is in the hashtable
                else{
                    //Finds list associated with username
                    unsigned int *cur_balance = hash_table_find(bank->ht,split[1]);
                    unsigned int *valid_balance = malloc(sizeof(unsigned int));
                    *valid_balance = *cur_balance + strtoul(split[2],NULL,10);
                    if(*valid_balance < *cur_balance){
                        printf("%s", "Too rich for this program\n");
                        return;
                    }
                    else{
                        hash_table_del(bank->ht,split[1]);
                        hash_table_add(bank->ht, split[1], valid_balance);
                        printf("$%s added to %s's account\n",split[2],split[1]);
                        return;
                    }
                }
            }
            else{
                printf("%s\n","Usage: deposit <user-name> <amt>");
                return;  
            }
        }
        else{
            printf("%s\n","Usage: deposit <user-name> <amt>");  
            return;
        }
        
    }
    else if(strcmp(balance,split[0])==0){
        if(num_words != 2){
             printf("%s\n","Usage: balance <user-name>");
             return;
        }
      
        //Checks for valid username
        if(split[1] && isValidUsername(split[1])==1){
            //Checks if user is in the hashtable
            if(hash_table_find(bank->ht,split[1])==NULL){
                printf("No such user\n");
		return;
            }
            //User is in the hashtable
            else{
                unsigned int *cur_balance = hash_table_find(bank->ht,split[1]);
                printf("$%u\n",*cur_balance);
            }
        }
        else{
            printf("%s\n","Usage: balance <user-name>"); 
            return;
        }
            
    }
    else{
        //Invalid Command
        printf("%s", "Invalid command\n");
    }
        
}

//Generate random interaction ID based on the time
char *generate_id(){
    srand((unsigned)time(NULL));
    char *alphanum = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    char *key;
    int i;
    key = malloc(sizeof(char) * 51);
    for (i = 0; i < 50; i++){
	int letter = rand() % strlen(alphanum);
	key[i] = alphanum [letter];
    }
    key[50] = '\0';
    return key;
}

//Withdrawal parameters are validated prior to this function call
void withdraw_money(Bank *bank, char *username, char *amount, char*id){
   
     //Finds list associated with username
     unsigned int *cur_balance = hash_table_find(bank->ht,username);

     unsigned int *new_balance = malloc(sizeof(unsigned int));
     *new_balance = *cur_balance - strtoul(amount,NULL,10);

     //Withdrawal is valid prior to this function call
     hash_table_del(bank->ht,username);

     hash_table_add(bank->ht, username, new_balance);

     char sendline[] = "money dispensed";
     int cipher_length;  
     char * cipher = encrypt_msg(&cipher_length, sendline, bank->key, id);
     bank_send(bank, cipher, cipher_length);
     return;
}

//sends a message to the atm to notify that data has been tampered with
void send_tampered_error(Bank *bank, char *id){

    char *error_msg="tampered";

    int cipher_length;  
    char * cipher = encrypt_msg(&cipher_length, error_msg, bank->key, id);
    
    bank_send(bank, cipher, cipher_length);
}

void bank_process_remote_command(Bank *bank, char *command, size_t len)
{
    // TODO: Implement the bank side of the ATM-bank protocol
    char ciphertext[MAX_BUF_SIZE];
    int ciphersize;
    //perform a new operation
    if(strcmp(command, "0") == 0){

        //we have recieved a ping to generate and send an interaction ID
	char *id = generate_id();
        bank_send(bank, id, strlen(id));	
	//recieve ciphertext
	ciphersize = bank_recv(bank, ciphertext, MAX_BUF_SIZE);

	//decrypt ciphertext and put message in plaintext
   	char *message = decrypt_msg(ciphersize, ciphertext, bank->key, id);
      
	//error in decryption. most likely means tampered data
	if(message == NULL){
           send_tampered_error(bank,id);
           return;
	}
	
	//create a new string with a null terminator
	char args[strlen(message)+1];
	strncpy(args,message,strlen(message));
	args[strlen(message)] = '\0';

	int num_words = numWords(args);
	char **split = splitCommand(args);
	char withdraw_request[] = "withdraw";
        char pin_request[] = "pin";
       
	//validate user
	if(strcmp(split[0], "validate") == 0){
	      
	    if(num_words != 2){
		send_tampered_error(bank, id);
                return ;
	    }

	    //user is not in database
            unsigned int *cur_balance = hash_table_find(bank->ht,split[1]);
            if(cur_balance == NULL){
	        char *response = "no such user";
   	        int cipher_length;  
	        char * encrypted_response = encrypt_msg(&cipher_length, response, bank->key, id);
	        bank_send(bank, encrypted_response, cipher_length);
	        return;
            }
	    
	    //user's card is not found
	    char filename[strlen(split[1]) + 6];
	    strcpy(filename, split[1]);
	    strncat(filename, ".card", 5);
            if(access(filename, F_OK) == -1){
	       char *response = "unable to access";
   	       int cipher_length;  
	       char * encrypted_response = encrypt_msg(&cipher_length, response, bank->key, id);
	       bank_send(bank, encrypted_response, cipher_length);
               return ;
            }
            
	    //read card number from card
            FILE *card = fopen(filename,"r");
	    char *info = malloc(sizeof(char) * 50);
	    char *result = fgets(info, 50, card);
	    
            //remove newline since it causes issues
	    char *pos;
            if ((pos=strchr(info, '\n')) != NULL)
                *pos = '\0';

            unsigned int *num_tries = hash_table_find(bank->failed_attempts, split[1]);
            if(*num_tries == 3){
	        char *response = "blacklisted";
   	        int cipher_length;  
	        char *encrypted_response = encrypt_msg(&cipher_length, response, bank->key, id);
	        bank_send(bank, encrypted_response, cipher_length);
		return ;            
	    }

            if( result == NULL || strcmp(info, hash_table_find(bank->card_info,split[1])) != 0){
	        char *response = "not authorized";
   	        int cipher_length;  
	        char *encrypted_response = encrypt_msg(&cipher_length, response, bank->key, id);
	        bank_send(bank, encrypted_response, cipher_length);
		return ;
	    }
	    else{
	        char *response = "user exists";
   	        int cipher_length;  
	        char *encrypted_response = encrypt_msg(&cipher_length, response, bank->key, id);
	        bank_send(bank, encrypted_response, cipher_length);
		return ;
	    }
	   return;
      
	}
        else if(strcmp(pin_request, split[0]) == 0){
            //Checks split size & if user is in the bank database
	    if(num_words != 3 || hash_table_find(bank->ht, split[1])==NULL){	
		send_tampered_error(bank,id);
                return;
	    }
           
            //Checks if pin sent matches pin in database
            if(strcmp(split[2],hash_table_find(bank->pin,split[1])) == 0){

              //reset failed attempts for the user
              unsigned int *new_tries = malloc(sizeof(unsigned int));
              *new_tries = 0;
              hash_table_del(bank->failed_attempts,split[1]);
              hash_table_add(bank->failed_attempts, split[1], new_tries);             
 
              //user is authorized
              char *response = "authorized";
   	      int cipher_length;  
	      char * encrypted_response = encrypt_msg(&cipher_length, response, bank->key, id);
	      bank_send(bank, encrypted_response, cipher_length);
           }
           else{

              unsigned int *num_tries = hash_table_find(bank->failed_attempts, split[1]);
              unsigned int *new_tries = malloc(sizeof(unsigned int));
              *new_tries = *num_tries + 1;
              hash_table_del(bank->failed_attempts,split[1]);
              hash_table_add(bank->failed_attempts, split[1], new_tries);

              char *response = "not authorized";
              int cipher_length;  
              char * encrypted_response = encrypt_msg(&cipher_length, response, bank->key, id);
              bank_send(bank, encrypted_response, cipher_length);
 
              
           }
        }
	//withdraw
	else if(strcmp(withdraw_request,split[0]) == 0){
	     
             //Checks split size & if user is in the bank database
	     if(num_words != 3 || hash_table_find(bank->ht, split[1])==NULL){
		 send_tampered_error(bank,id);
                 return;
	     }
	     unsigned int *balance = hash_table_find(bank->ht, split[1]);

	     //Checks if withdraw amount is greater than current balance
             
	     if(strtoul(split[2],NULL,10)>*balance){
		  char sendline[] = "insufficient funds";
                  int cipher_length;  
	          char * cipher = encrypt_msg(&cipher_length, sendline, bank->key, id);
		  bank_send(bank, cipher, cipher_length);
		  return;
	      }
	      //Withdrawal is valid
	      else{
		  withdraw_money(bank,split[1],split[2], id);
		  return;
	      }
	 }
	 else if(strcmp(split[0], "balance") == 0){
             
             //Checks split size & if user is in the bank database

	     if(num_words != 2|| hash_table_find(bank->ht, split[1])==NULL){
		 send_tampered_error(bank,id);
                 return;
	     }

	     unsigned int *balance_val = hash_table_find(bank->ht,split[1]);
             char cur_balance[11];
             sprintf(cur_balance,"%u",*balance_val);
	     
	     int cipher_length;  
	     char * encrypted_response = encrypt_msg(&cipher_length, cur_balance, bank->key, id);
	     bank_send(bank, encrypted_response, cipher_length);
             return ;
         }
         //unrecognized command
         else{
	     send_tampered_error(bank, id);
             return ;
         }
    }


    /*
    printf("Bank got: %s", command);
    bank_send(bank, sendline, strlen(sendline));
    printf("Received the following:\n");
    fputs(command, stdout);
    */
	
    
}
