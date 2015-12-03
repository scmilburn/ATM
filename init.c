#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>

#define KEYLENGTH 32

int main(int argc, char* argv[]){
    FILE *bank_file, *atm_file;
    char *bank_path, *atm_path;
    int n;
    time_t t;
    
    if(argc!=2){
        printf("Usage: init <filename>\n");
        return 62;
    }
     
    atm_path = malloc(strlen(argv[1]) + strlen(".atm") + 1);
    bank_path = malloc(strlen(argv[1]) + strlen(".bank") + 1); 
    
    strcat(bank_path, argv[1]);
    strcat(bank_path, ".bank");
    strcat(atm_path, argv[1]);
    strcat(atm_path, ".atm");
   
    bank_file=fopen(bank_path,"r");
    atm_file=fopen(atm_path,"r");
    if(bank_file == 0 && atm_file == 0){ //file does not exist
        if ((bank_file=fopen(bank_path,"w")) == NULL || (atm_file=fopen(atm_path,"w")) == NULL){
            printf("Error creating initialization files\n");
            free(bank_path);
            free(atm_path);  
            return 64;
        }
        
        //KEY GENERATION
        char key[KEYLENGTH];
	//memset(key,'\0',KEYLENGTH);
        const char charset[]="abcdefghijklmnopqrstuvwxyz"; 
        srand((unsigned)time(&t));
        for (n = 0; n < KEYLENGTH; n++){
            int k = rand() % 26;
            key[n]=charset[k];
        }
        key[KEYLENGTH]='\0';

        unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        //printf("Private key generated is %s\n",key);        
        fwrite(key,1,strlen(key),bank_file);
        fwrite(key,1,strlen(key),atm_file);
        fclose(atm_file);
        fclose(bank_file);
        //printf("file does not exist: Good\n");
        free(bank_path);
        free(atm_path);

        printf("Successfully initialized bank state\n");
        return 0;
    }else{
        printf("Error: one of the files already exists\n");
        free(bank_path);
        free(atm_path);       
        return 63;
    }
}

