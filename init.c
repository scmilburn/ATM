#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>

#define KEYLENGTH 32

int main(int argc, char* argv[]){
    FILE *bank_file, *atm_file;
    char *bank, *atm, *argcpy;
    int n;
    time_t t;
    
    if(argc!=2){
        printf("Usage: init <filename>\n");
        return 62;
    }
    
    
    argcpy=malloc(strlen(argv[1]));
    strcpy(argcpy,argv[1]); //////FIX 
    bank = strcat(argv[1],".bank");
    atm = strcat(argcpy,".atm");
    bank_file=fopen(bank,"r");
    atm_file=fopen(atm,"r");
    if(bank_file == 0 && atm_file == 0){ //file does not exist
        bank_file=fopen(bank,"w");
        atm_file=fopen(atm,"w");
        char key[KEYLENGTH];
        const char charset[]="abcdefghijklmnopqrstuvwxyz";
        
        srand((unsigned)time(&t));
        for (n = 0; n < KEYLENGTH; n++){
            int k = rand() % 26;
            key[n]=charset[k];
        }
        key[KEYLENGTH]='\0';

        unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        printf("Private key generated is %s\n",key);        
        fwrite(key,1,sizeof(key),bank_file);
        fwrite(key,1,sizeof(key),atm_file);
        fwrite(iv,1,sizeof(iv),bank_file);
        fwrite(iv,1,sizeof(iv),atm_file);
        fclose(atm_file);
        fclose(bank_file);
        //printf("file does not exist: Good\n");
        free(argcpy);
        return 0;
    }else{
        printf("Error: one of the files already exists\n");
        free(argcpy);       
        return 63;
    }
}

