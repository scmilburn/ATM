#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <openssl/evp.h>

//counts the number of words separated by a space
int numWords(char* string){
   int i, num_words;
   num_words = 0;
   for(i = 0; i < strlen(string); i++){
      if(string[i] == ' '){
          num_words++;
      }
   }
   return num_words +1;
}

//splits the command into an array of strings
char** splitCommand(char* string)
{
    
    char *stringp = string;
    const char *delim = " ";
    char *token;
    char *element;
    char **res = NULL;
    int num_args = 0;

    while(stringp){
       token = strsep(&stringp, delim);
       res = realloc(res,sizeof(char*) * ++num_args);
       if(res == NULL)
          exit(-1);

	//printf("token: %s\n", token);
	element = malloc(sizeof(char) * (strlen(token)+1));
        strncpy(element, token, strlen(token));
        element[strlen(token)] = '\0';

        res[num_args -1] = element;
    }
    return res;
}

//checks the validity of a pin. return 1 if valid, 0 if not
int isValidPin(char* string)
{
    char *stringp = string;
    int reti;
    regex_t regex;
    reti = regcomp(&regex, "[0-9][0-9][0-9][0-9]",0);
    reti = regexec(&regex, stringp, 0, NULL,0);

    if(!reti){
        return 1;
    }
    else{
        return 0;
    }
}

//checks validity of the balance
int isValidBalance(char* string)
{
    char *stringp = string;
    int reti;
    regex_t regex;
    reti = regcomp(&regex, "^[0-9]+$",REG_EXTENDED);
    reti = regexec(&regex, stringp, 0, NULL,0);

    if(!reti){
	    //if the string has more than 10 digits, it is invalid
	    if(strlen(string) > 10){
		return 0;
	    } 	
	    if(strlen(string) == 10){
		
		char *max_val = "4294967295";
		int x = strncmp(string, max_val, 10);        
		if(x <=0)
		   return 1;
		else
		   return 0;
	    }
	    else{
		unsigned int amt = strtoul( string , NULL, 10);
		return (amt >= 0);
	    } 	
    }
    else{
        return 0;
    }


}

//checks the validity of a username. return 1 if valid, 0 if not
int isValidUsername(char* username){
   regex_t regex;
   int status;
  
   //false if username is more than 250 characters
   if(strlen(username) > 250 ){
      return 0;
   }

   status = regcomp( &regex, "^[a-zA-z]+", REG_EXTENDED);

   if(status){
      return 1;
   }	   

   status = regexec( &regex, username, 0, NULL, 0);
   if(!status){
      return 1;
   }
   else{
      return 0;   
   }
}

//encrypts a message. returns NULL on error, which probably means someones been hacking. WE SEE YOU SUCKA
char *encrypt_msg(int* length, char *msg, char* key, char* iv){

	//initialize openssl api stuff
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init(&ctx);
	EVP_EncryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, key, iv);
	

	unsigned char cipher[5000];
	int cipherlen = 0;
	int outlen;
	
	if(!EVP_EncryptUpdate(&ctx, cipher, &outlen, msg, strlen(msg))) {
		/* Error */
		EVP_CIPHER_CTX_cleanup(&ctx);
		return NULL;
	}
	//strncat(cipher, outbuf, outlen);
	cipherlen += outlen;
	
	if(!EVP_EncryptFinal_ex(&ctx, cipher + outlen, &outlen)){
        	/* Error */
        	EVP_CIPHER_CTX_cleanup(&ctx);
		return NULL;
        }	
	//strncat(cipher, outbuf, outlen);
	cipherlen += outlen;
	
	EVP_CIPHER_CTX_cleanup(&ctx);



	//cipher is only a local variable. allocate memory and return encrypted string
	char *to_return = malloc(sizeof(char) * (cipherlen+1));
	strncpy(to_return, cipher, cipherlen);
	to_return[cipherlen] = '\0';
	*length = cipherlen;

	return to_return;
}
//decrypts a message. returns NULL on error, which probably means someones been hacking. WE SEE YOU SUCKA
char *decrypt_msg(int length, char *cipher, char* key, char* iv){

	//initialize openssl api stuff
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init(&ctx);
	EVP_DecryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, key, iv);
	
	unsigned char plaintext[5000];
	int msglen = 0;
	int outlen;
	
	if(!EVP_DecryptUpdate(&ctx, plaintext, &outlen, cipher, length)) {
		/* Error */
		EVP_CIPHER_CTX_cleanup(&ctx);
		return NULL;
	}
	//strncat(plaintext, outbuf, outlen);
	msglen += outlen;
	if(!EVP_DecryptFinal_ex(&ctx, plaintext + outlen, &outlen)){
        	/* Error */
        	EVP_CIPHER_CTX_cleanup(&ctx);
		return NULL;
        }
	//strncat(plaintext, outbuf, outlen);
	msglen += outlen;

	EVP_CIPHER_CTX_cleanup(&ctx);
	
	//msg is only a local variable. allocate memory and return decrypted string
	char *to_return = malloc(sizeof(char) * (msglen+1));
	strncpy(to_return, plaintext, msglen);
	to_return[msglen] = '\0';
	return to_return;
}

