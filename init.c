#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>

void getKey(){

}

int main(int argc, char** argv){
    FILE *bank, *atm;
    char *atm_fn, *bank_fn    
    atm_fn = ;    
    bank_fn = ;
    //if not single arg, usage err
    if(argc != 2){
        printf("Usage: init <filename>\n");
        return 62;
    }

    //init files already exist
    if(already exists){
        printf("Error: one of the files already exists\n");
        return 63;
    }    

    //init failure
    if((atm = fopen(atm_fn, "w")) == NULL || (bank = fopen(bank_fn, "w")) == NULL){
        printf("Error creating initialization files\n");
        return 64;
    }
    getKeys();
    
    //write keys to initfiles
    //fwrite....


    printf("Successfully initialized bank state\n");
    return 0;   
}   
