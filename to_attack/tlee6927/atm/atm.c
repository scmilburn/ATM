#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/evp.h>

ATM* atm_create()
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
    OpenSSL_add_all_digests();

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

/*Checks if a string contains only [a-zA-Z+]*/
int isChar(char *str, int len){
    int i = 0;
    for(i = 0; i < len; i++){
        char c = str[i];
        if((97 <= c && 122 >= c) || (65 <= c && 90 >= c))
            continue;
        else
            return 0;
    }
    return 1;
}

/*Checks if a string only contains [0-9]*/
int isNum(char *str, int len){
    int i = 0;
    for(i = 0; i < len; i++){
        char c = str[i];
        if(48 <= c && 57 >= c)
            continue;
        else
            return 0;
    }
    return 1;
}

int compHash(uint8_t *hash1, uint8_t *hash2){
    int i = 0;
    for(i = 0; i < 32; i++){
        if(*(hash1 + i) != *(hash2 + i))
            return 0;
    }
    return 1;
}

void atm_process_command(ATM *atm, char *command)
{

    char *token = strtok(command, " ");
    char *invalid;

    if((strcmp("begin-session", token)) == 0) {
        char *username = strtok(NULL, " "); //gets user name (aka next token)

        invalid = strtok(NULL, " "); //checks to make sure no extra words afterwards
        if((strlen(username) > 250) || (isChar(username, strlen(username)-1) == 0) || (invalid != NULL)) {
            printf("Usage: begin-session <user-name>\n");
        } //invalid inputs
        else{
            username[strlen(username)-1] = 0;
            if(atm->currentUser != NULL){
                printf("A user is already logged in\n");
            }
            else{
                uint8_t *sendData;
                int namelen = strlen(username);
                int datalen = namelen + 5;
                uint8_t retData[2];

                sendData = malloc(datalen);
                sendData[0] = 1;

                namelen = htonl(namelen);
                memcpy(sendData +1, &namelen, 4);
                memcpy(sendData + 5, username, strlen(username));
                //TODO Send and recv from bank yo

                atm_send(atm, sendData, datalen);

                atm_recv(atm, retData, 2);

                if(retData[1] == 1){
                    char pinInput[10000];
                    FILE *file;
                    char *fileName = malloc(strlen(username) + 6);


                    memcpy(fileName, username, strlen(username));
                    fileName[strlen(username)] = '.';
                    fileName[strlen(username)+1] = 'c';
                    fileName[strlen(username)+2] = 'a';
                    fileName[strlen(username)+3] = 'r';
                    fileName[strlen(username)+4] = 'd';
                    fileName[strlen(username)+5] = 0;

                    file = fopen(fileName, "r");
                    if (file != NULL) {
                        int holdnamelen;
                        char holdname[255];
                        uint8_t pinhash[32];

                        fread(&holdnamelen, 4, 1, file);
                        fread(holdname, 1, holdnamelen, file);
                        fread(pinhash, 1, 32, file);


                        printf("PIN? ");

                        fgets(pinInput, 10000, stdin);

                        if(strlen(pinInput) != 5 || isNum(pinInput, strlen(pinInput) - 1) == 0) {
                            printf("Not authorized\n");
                        }
                        else {
                            EVP_MD_CTX *ctx = EVP_MD_CTX_create();
                            uint8_t thehash[32];
                            int hashlen;

                            EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);

                            EVP_DigestUpdate(ctx, pinInput, 4);
                            EVP_DigestFinal_ex(ctx, thehash, &hashlen);
                            EVP_MD_CTX_destroy(ctx);

                            if(compHash(pinhash, thehash) == 0){
                                printf("Not authorized\n");
                            }
                            else{
                                User *user = malloc(sizeof(User));

                                user->name = malloc(strlen(username) + 1);
                                strcpy(user->name, username);

                                user->PIN[0] = pinInput[0];
                                user->PIN[1] = pinInput[1];
                                user->PIN[2] = pinInput[2];
                                user->PIN[3] = pinInput[3];

                                printf("Authorized\n");

                                atm->currentUser = user;

                            }
                        }
                    }
                    else{
                        printf("Unable to access %s's card\n", username);
                    }
                
                    free(fileName);
                }
                else{
                    printf("No such user\n");
                }
            }
        }
    }
    else{
        if((strcmp ("withdraw", token)) == 0) {
            char *amtStr = strtok(NULL, " ");
            invalid = strtok(NULL, " ");

            if(atm->currentUser == NULL) {
                printf("No user logged in\n");
            }
            else {
                if(isNum(amtStr, strlen(amtStr)-1) == 0 || invalid != NULL || (atoi(amtStr) > INT_MAX)) {
                    printf("Usage: withdraw <amt>\n");
                } //invalid inputs
                else{
                    int amount = atoi(amtStr);
                    uint8_t *message;
                    uint8_t *sendData;
                    int namelen = strlen(atm->currentUser->name);
                    int datalen = namelen + 8;
                    uint8_t retData[2];
                    uint32_t amountNL = htonl(amount);
                    EVP_MD_CTX *hashctx = EVP_MD_CTX_create();
                    uint8_t thehash[32];
                    int hashlen;

                    message = malloc(datalen);
                    message[0] = 1;

                    namelen = htonl(namelen);
                    memcpy(message, &namelen, 4);
                    memcpy(message + 4, atm->currentUser->name, strlen(atm->currentUser->name));
                    memcpy(message + 4 + strlen(atm->currentUser->name), &amountNL, 4);

                    EVP_DigestInit_ex(hashctx, EVP_sha256(), NULL);

                    EVP_DigestUpdate(hashctx, atm->currentUser->PIN,4);
                    EVP_DigestUpdate(hashctx, message, datalen);
                    EVP_DigestFinal_ex(hashctx, thehash, &hashlen);
                    EVP_MD_CTX_destroy(hashctx);

                    sendData = malloc(datalen + 33);
                    sendData[0] = 2;

                    memcpy(sendData + 1, message, datalen);
                    memcpy(sendData + 1 + datalen, thehash, 32);
                    //NEED TO TALK TO ROUTER/BANK TO GET BALANCE OF USER

                    atm_send(atm, sendData, datalen+33);

                    atm_recv(atm, retData, 2);

                    if(retData[1] == 0) {
                        printf("Insufficient funds\n");
                    }
                    else {
                        printf("$%u dispensed\n", amount);
                    } //sufficient funds in bank

                    free(message);
                    free(sendData);
                } //valid inputs
            } //user logged in

        }
        else{
            if(token != NULL)
                token[strlen(token)-1]=0;
            if((strcmp ("balance", token)) == 0) {
                invalid = strtok(NULL, " ");

                if(atm->currentUser == NULL) {
                    printf("No user logged in\n");
                }
                else {
                    if(invalid != NULL) {
                        printf("Usage: balance\n");
                    }
                    else{
                        uint8_t *message;
                        uint8_t *sendData;
                        int namelen = strlen(atm->currentUser->name);
                        int datalen = namelen + 4;
                        uint8_t retData[6];
                        EVP_MD_CTX *hashctx = EVP_MD_CTX_create();
                        uint8_t thehash[32];
                        int hashlen;

                        message = malloc(datalen);
                        message[0] = 1;

                        namelen = htonl(namelen);
                        memcpy(message, &namelen, 4);
                        memcpy(message + 4, atm->currentUser->name, strlen(atm->currentUser->name));

                        EVP_DigestInit_ex(hashctx, EVP_sha256(), NULL);

                        EVP_DigestUpdate(hashctx, atm->currentUser->PIN,4);
                        EVP_DigestUpdate(hashctx, message, datalen);
                        EVP_DigestFinal_ex(hashctx, thehash, &hashlen);
                        EVP_MD_CTX_destroy(hashctx);

                        sendData = malloc(datalen + 33);
                        sendData[0] = 3;

                        memcpy(sendData + 1, message, datalen);
                        memcpy(sendData + 1 + datalen, thehash, 32);
                        //NEED TO TALK TO ROUTER/BANK TO GET BALANCE OF USER

                        atm_send(atm, sendData, datalen+33);

                        atm_recv(atm, retData, 6);

                        if(retData[1] == 1) {
                            uint32_t balNL= *((uint32_t *)(retData+2));
                            int balance = ntohl(balNL);
                            printf("$%u\n",balance);
                        }
                        else {
                            printf("Unauthorized\n");
                        } //sufficient funds in bank

                        free(message);
                        free(sendData);
                    }
                }
            }
            else{
                if((strcmp ("end-session", token)) == 0){
                    invalid = strtok(NULL, " ");

                    if(atm->currentUser == NULL) {
                        printf("No user logged in\n");
                    }
                    else {
                        if(invalid != NULL) {
                            printf("Invalid command\n");
                        }
                        else{
                            free(atm->currentUser->name);
                            free(atm->currentUser);
                            atm->currentUser = NULL;
                            printf("User logged out\n");
                        }
                    }
                }
                else{
                    printf("Invalid command\n");
                }
            }
        }
    }

}