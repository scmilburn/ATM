#include "atm.h"
#include "ports.h"
#include "functions.h"
#include "hash_table.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


int MAX_BUF_SIZE = 5000;

ATM* atm_create(char* filename)
{
    ATM *atm = (ATM*) malloc(sizeof(ATM));
    if(atm == NULL)
    {
        perror("Could not allocate ATM");
        exit(1);
    }

    // Set up the network state
    atm->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&atm->rtr_addr,sizeof(atm->rtr_addr));
    atm->rtr_addr.sin_family = AF_INET;
    atm->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&atm->atm_addr, sizeof(atm->atm_addr));
    atm->atm_addr.sin_family = AF_INET;
    atm->atm_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    atm->atm_addr.sin_port = htons(ATM_PORT);
    bind(atm->sockfd,(struct sockaddr *)&atm->atm_addr,sizeof(atm->atm_addr));

    // Set up the protocol state
    // TODO set up more, as needed
    atm->cur_user = NULL;

    char *key = malloc(sizeof(char)*52);
    FILE *fp = fopen(filename, "r");
    if( fp == NULL ) {
      printf("Error opening ATM initialization file\n");
      return NULL;
    }

    fgets(key, 51, fp);
    key[50] = '\0';

    atm->key = key;

    return atm;
}

void atm_free(ATM *atm)
{
    if(atm != NULL)
    {
        close(atm->sockfd);
        free(atm);
    }
}

ssize_t atm_send(ATM *atm, char *data, size_t data_len)
{
    // Returns the number of bytes sent; negative on error
    return sendto(atm->sockfd, data, data_len, 0,
                  (struct sockaddr*) &atm->rtr_addr, sizeof(atm->rtr_addr));
}

ssize_t atm_recv(ATM *atm, char *data, size_t max_data_len)
{
    // Returns the number of bytes received; negative on error
    return recvfrom(atm->sockfd, data, max_data_len, 0, NULL, NULL);
}

char *ping_interaction(ATM *atm){

    //send request for ping
    char *ping_code="0";
    atm_send(atm, ping_code, 2);

    //retrieve ping and return
    char *id = malloc(sizeof(char) * 51);
    id[50] = '\0';
    atm_recv(atm, id, 50);
    return id;
}

char * atm_process_command(ATM *atm, char *command)
{
   char begin_session[] = "begin-session";
   char withdraw[] = "withdraw";
   char balance[] = "balance";
   char end_session[] = "end-session";
    //create a new string with a null terminator
    char args[strlen(command)+1];
    strncpy(args,command,strlen(command));
    args[strlen(command)] = '\0';

    int num_words = numWords(args);
    char **split = splitCommand(args);
    
   //If command is begin-session
   if(strcmp(begin_session,split[0])==0){
      //Check if there is already a user logged in
      if (atm->cur_user != NULL){
         printf("%s", "A user is already logged in\n");
         return atm->cur_user;
      } 

      //Check if number of arguments is correct
      if(num_words != 2){
	  printf("Usage: begin-session <user-name>\n");
          return NULL;
      }
      
      //Check if username is valid
      if(isValidUsername(split[1])){
	 char* id = ping_interaction(atm);

         //make string "validate <username>\0"
	 char * validate = "validate ";
	 int msgLength = 9 + strlen(split[1]);
	 char interaction[msgLength + 1];
	 strcpy(interaction, validate);
	 strncat(interaction, split[1], strlen(split[1]));
	 interaction[msgLength] = '\0';
         
	 //send request to validate user
	 int cipher_length;  
	 char * cipher = encrypt_msg(&cipher_length, interaction, atm->key, id);
	 atm_send(atm, cipher, cipher_length);
         
	 //retrieve response ciphertext
	 char r_ciphertext[MAX_BUF_SIZE];
         int r_length;
         char *r_plaintext;
         r_length = atm_recv(atm, r_ciphertext, MAX_BUF_SIZE);
        
         //decrypt ciphertext and put message in plaintext
         r_plaintext = decrypt_msg(r_length, r_ciphertext, atm->key, id);
         
	 //return NULL on tampered data
         if(r_plaintext == NULL || strcmp(r_plaintext, "tampered") == 0){
	     return NULL;
	 }
	 
	 //return NULL if no such user found
	 if(strcmp(r_plaintext, "no such user") == 0){
	     printf("No such user\n");
	     return NULL;
	 }

	 //return NULL if unable to access card
	 if(strcmp(r_plaintext, "unable to access") == 0){
             printf("Unable to access %s's card\n",split[1]);
	     return NULL;
	 }
         
	 if(strcmp(r_plaintext, "blacklisted") == 0){
              printf("This user is blacklisted\n");
              return NULL;
         }
	 
	 //return NULL if .card file does not match bank records
	 if(strcmp(r_plaintext, "not authorized") == 0){
             printf("Not authorized\n");
	     return NULL;
	 }
	 //return NULL if there has been tampered data
	 if(strcmp(r_plaintext, "user exists") != 0){
	     return NULL;
	 }
         
	 //Check if pin is valid
         char read[6];
         printf("%s", "PIN? ");
         fgets(read, 6,stdin);
               
         //Create a new string without a new line and  null terminator
         char pin[strlen(read)];
         strncpy(pin,read,strlen(read)-1);
         pin[strlen(read)-1] = '\0';	

         //Check if pin is valid
         if(isValidPin(pin)){

           char* pin_id = ping_interaction(atm);
          
           //Build command to bank
           char * pin_str = "pin ";
           char * space = " ";    
          

           
           int msgLength = strlen(pin)+5+strlen(split[1]);
           
           char send_pin[msgLength+1];
          
           strcpy(send_pin, pin_str);
          
           strncat(send_pin,split[1],strlen(split[1]));
           
           strncat(send_pin, space, 1);
           
           strncat(send_pin,pin,strlen(pin));
          
           send_pin[strlen(send_pin)]='\0';
          
           //send request to validate pin
	   int pin_cipher_length;  
	   char * pin_cipher_text = encrypt_msg(&pin_cipher_length, send_pin, atm->key, pin_id);
	   atm_send(atm, pin_cipher_text, pin_cipher_length);

           //retrieve response ciphertext
	   char pin_r_ciphertext[MAX_BUF_SIZE];
           int pin_r_length;
           char *pin_r_plaintext;
           pin_r_length = atm_recv(atm, pin_r_ciphertext, MAX_BUF_SIZE);

	   pin_r_plaintext = decrypt_msg(pin_r_length, pin_r_ciphertext, atm->key, pin_id);
	 
           //return NULL on tampered data
           if(pin_r_plaintext == NULL || strcmp(pin_r_plaintext, "tampered") == 0){
	                        
	       return NULL;
	   }

           //checks if pin is authorized
           if(strcmp(pin_r_plaintext, "authorized") == 0){
               printf("Authorized\n");

               char *username = malloc(sizeof(char) * (strlen(split[1]) + 1));
	       strncpy(username, split[1], strlen(split[1]));
               username[strlen(split[1])] = '\0';
	       atm->cur_user = username;
               return atm->cur_user;
           }
	   else{
		printf("%s", "Not authorized\n");
		return NULL;
	   }
	 }
         //Pin formatting is not correct
         else{
            printf("%s", "Not authorized\n");
            return NULL;
         } 
      }
      //invalid username -> Usage.
      else{
          printf("%s", "Usage: begin-session <user-name>\n");
          return NULL;
      }
   }
   //If command is withdraw
   else if(strcmp(withdraw,split[0])==0){

      //Check if there is already a user logged in
      if (atm->cur_user == NULL){
          printf("%s", "No user logged in\n");
	  return NULL;
      }

      //Check if number of arguments is correct
      if(num_words != 2){
          printf("Usage: withdraw <amt>\n"); 
          return atm->cur_user;
      }

      //Check for valid balance input
      if(isValidBalance(split[1])){

         //If there is not user logged in
         if(atm->cur_user == NULL){
            printf("%s","No user logged in\n");
            return NULL;
         }

         char* id = ping_interaction(atm);

         //Build command to bank
         char * withdraw_str = "withdraw ";
         char * space = " ";            
         int msgLength = strlen(atm->cur_user)+10+strlen(split[1]);
         char send_withdraw[msgLength+1];
         strcpy(send_withdraw, withdraw_str);
         strncat(send_withdraw,atm->cur_user,strlen(atm->cur_user));
         strncat(send_withdraw, space, 1);
         strncat(send_withdraw,split[1],strlen(split[1]));
         send_withdraw[strlen(send_withdraw)]='\0';

         //Send to bank command from atm
         int cipher_length;  
         char * cipher = encrypt_msg(&cipher_length, send_withdraw, atm->key, id);
         atm_send(atm, cipher, cipher_length);
            
	 //recieve and decrypt ciphertext
	 char ciphertext_w[MAX_BUF_SIZE];
	 int n = atm_recv(atm, ciphertext_w, MAX_BUF_SIZE);
   	 char *message = decrypt_msg(n, ciphertext_w, atm->key, id);
           
	 //return back to ATM if data has been tampered with
         if(message == NULL || strcmp(message, "tampered") == 0)
            return atm->cur_user;
         
         //If Bank sends insufficient funds
         if(strcmp(message,"insufficient funds")==0){
            printf("%s", "Insufficient funds\n");
            return atm->cur_user;
         }
         //User has sufficient funds
         else if(strcmp(message,"money dispensed")==0){
            printf("$%s dispensed\n",split[1]);
            return atm->cur_user;
         }
         else{
	    return atm->cur_user;
         }
      }
      else{
          printf("Usage: withdraw <amt>\n"); 
          return atm->cur_user;
      }
   }
   //If command is balance
   else if(strcmp(balance,split[0])==0){

      if(atm->cur_user == NULL){
          printf("No user logged in\n");
          return NULL;
      }

      if(num_words != 1){
          printf("Usage: balance\n"); 
          return atm->cur_user;
      }

      char* id = ping_interaction(atm);

      //create balance string
      char *balance_string = "balance ";
      int msgLength = 8 + strlen(atm->cur_user);
      char interaction[msgLength + 1];
      strcpy(interaction, balance_string);
      strncat(interaction, atm->cur_user, strlen(atm->cur_user));
      interaction[msgLength] = '\0';
      
      //send request to retrieve balance
      int cipher_length;  
      char *cipher = encrypt_msg(&cipher_length, interaction, atm->key, id);
      atm_send(atm, cipher, cipher_length);

      //retrieve response ciphertext
      char r_ciphertext[MAX_BUF_SIZE];
      int r_length;
      char *r_plaintext;
      r_length = atm_recv(atm, r_ciphertext, MAX_BUF_SIZE);

      //decrypt ciphertext
      r_plaintext = decrypt_msg(r_length, r_ciphertext, atm->key, id);
      
      //return NULL on tampered data
      if(r_plaintext == NULL || strcmp(r_plaintext, "tampered") == 0){
	  return NULL;
      }

      return atm->cur_user;

   }
   //If command is end-session
   else if(strcmp(end_session,split[0])==0){

      if(num_words != 1){
          printf("Usage: end-session\n");  
          return atm->cur_user;
      }

      //Checks if user is logged in
      if(atm->cur_user == NULL){
         printf("%s", "No user logged in\n");
         return NULL;
      }
      //User is logged in
      else{
         free(atm->cur_user);
         atm->cur_user = NULL;
         printf("%s","User logged out\n");
         return NULL;
      }
   }
   else{
       //Invalid Command
       printf("%s", "Invalid command\n");
       if(atm->cur_user == NULL){
           return NULL;
       }
       else{
           return atm->cur_user;
       }
   }
   return NULL;

}
