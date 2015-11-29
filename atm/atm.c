#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <openssl/evp.h>

static const char session_token[250];
unsigned char encrypted[10000];
unsigned char decrypted[10000];

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

char * atm_process_command(ATM *atm, char *command,char *key)
{
    char *str,*str1;
    str=strtok(command,"\n");
    char *packet = malloc(10000);

    //balance	
    if(strcmp(str,"balance")==0){
        
        //what does this do?
        str = strtok(NULL," ");
        if(str !=NULL){
            printf("Invalid command\n");
        }
        if(!strcmp(session_token,"")){
            printf("No user logged in\n");
        }else{
            sprintf(packet,"<balance|%s>",session_token);
            char recvline[10000];
            if(!encrypt(packet,key,encrypted)){
                printf("Usage: balance\n");
                free(packet);
                packet=NULL;
                return session_token;
            }
            printf("sending packet:%s\n",packet);

            char packet_contents[10000];
            send_and_recv(atm,packet,key,packet_contents);
            if(packet_contents==NULL){
                free(packet);
                packet=NULL;
                return session_token;
            }
            char *comm=strtok(packet_contents,"|");
            if(comm == NULL){ //there was a decrypt final error which would make comm NULL
                free(packet);
                packet=NULL;
                return session_token;
            }
            if(strcmp(comm,"balance")){
                printf("Usage: balance\n");
            }else{
                comm=strtok(NULL,"|");
                if(strcmp(comm,session_token)){
                    printf("Usage: balance\n");
                }else{
                    comm=strtok(NULL,"|");
                    if(comm==NULL){
                        printf("Usage: balance\n");
                    }else{
                        printf("$%s\n",comm);
                    }
                }
            }			
        }
        //end-session
    }else if(strcmp(str,"end-session")==0){
        //check to see if too many arguments
        str = strtok(NULL," ");
        if(str !=NULL){
            printf("Invalid command\n");
        }
        //printf("asking to end session\n");
        if(!strcmp(session_token,"")){
            printf("No user logged in\n");
        }else{
            printf("User logged out\n");
            memset(session_token,'\0',250);
        }
    }else{
        str1=strtok(str," ");
        //begin-session <username>
        if(strcmp(str1,"begin-session")==0){
            if(strcmp(session_token,"")){
                printf("A user is already logged in\n");
            }else{
                str1 = strtok(NULL," ");
                if(str1==NULL ){
                    printf("Invalid command\n");
                    free(packet);
                    packet=NULL;
                    return session_token;
                }else{
                    //check to see if too many arguments
                    char *user[sizeof(str1)];
                    memset(user,'\0',sizeof(str1));
                    strncpy(user,str1,strlen(str1));
                    str1 = strtok(NULL," ");
                    if(str1 !=NULL){
                        printf("Invalid command\n");
                        free(packet);
                        packet=NULL;
                        return session_token;
                    }
                    char sendline[10];
                    int n;
                    printf("PIN? ");
                    fgets(sendline, 10,stdin);
                    char *pin=strtok(sendline,"\n");	
                    //check pin size and all digits
                    if(strlen(sendline)!=4 || !all_digits(sendline)){
                        printf("Not Authorized\n");
                        free(packet);
                        packet=NULL;
                        return session_token;
                    }

                    sprintf(packet,"<authentication|%s>",user);
                    printf("sending packet:%s\n",packet);
                    if(authenticate(user, packet,atm,key,pin)){
                        strcpy(session_token,user);
                        printf("Authenticated\n");
                    }else{
                        printf("Not Authorized\n");
                    }
                }

            }
        //withdraw <amt>
        }else if(strcmp(str1,"withdraw")==0){
            printf("asking to withdraw\n");
            if(!strcmp(session_token,"")){
                printf("No user logged in\n");
            }else{
                str1 = strtok(NULL," ");
                if(str1==NULL){
                    printf("Invalid command\n");
                }else{
                    //check if digits and greater than or equal to zero
                    if(!all_digits(str1) || atoi(str1)< 0){
                        printf("Usage: withdraw %s\n",str1);
                        free(packet);
                        packet=NULL;
                        return session_token;
                    }
                    sprintf(packet,"<withdraw|%s|%s>",session_token,str1);
                    printf("sending packet:%s\n",packet);
                    if(atoi(str1) < 0){
                        printf("Invalid command\n");
                    }else{
                        char parsed[10000];
                        send_and_recv(atm,packet,key,parsed);
                        if(parsed == NULL){
                            printf("Invalid packet\n");
                            free(packet);
                            packet=NULL;
                            return session_token;
                        }
                        if(strcmp(parsed,"withdraw_successful")){
                            printf("Insufficient funds\n");
                        }else{
                            printf("$%s dispensed\n",str1);
                        }
                    }
                }			
            }
        }else{
            printf("Invalid command\n");
        }
    }
    free(packet);
    packet=NULL;
    return session_token;
}

//checks if string parameter is all digits

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

//authenticates the user by obtaining the key for the card file from the bank and then using it to decrypt the card file to check if the pin is correct

int authenticate(char *user_name, char *packet, ATM *atm,char *key,char *user_pin){
    int ret=0;	
    FILE *card_file;
    char *argcpy=malloc(strlen(user_name));
    strcpy(argcpy,user_name); //////FIX 
    char *c = strcat(argcpy,".card");
    card_file=fopen(c,"r");
    if(!card_file){ //card file does not exist
        printf("No such user\n");
        ret=0;
    }else{
        char buf[10000];
        size_t bytes_read;
        bytes_read=fread(buf,sizeof(buf),1,card_file);
        char packet_contents[10000];
        send_and_recv(atm,packet,key,packet_contents);
        if(packet_contents==NULL){
            free(argcpy);
            return 0;
        }		
        char *comm=strtok(packet_contents,"|");
        if(comm==NULL){
            free(argcpy);
            return 0;
        }
        //printf("command is %s\n",comm);
        if(strcmp(comm,"authentication")){
            free(argcpy);
            return 0;
        }
        comm = strtok(NULL,"|");
        if(comm == NULL){
            free(argcpy);
            return 0;
        }
        if(!strcmp(comm,"not found")){
            free(argcpy);
            return 0;
        }

        //decrypt card file with key
        char decrypt_card[10000];
        if(!decrypt(buf,comm,decrypt_card)){
            free(argcpy);
            return 0;
        }
        char *pin=strtok(decrypt_card,"\n");
        pin=strtok(decrypt_card,";");

        //check that name matches user_name
        if(strcmp(pin,user_name)){
            printf("user_name does not match name on card\n");
            ret=0;
        }
        else{ 
            pin = strtok(NULL,";");

            //check if pin matches pin
            if(atoi(pin)==atoi(user_pin)){
                ret=1;	
            }else{
                //printf("pins do not match\n");
                ret=0;
            }
        }
        fclose(card_file);
    }
    free(argcpy);
    return ret;
}

void send_and_recv(ATM *atm, char *packet,char *key, char *result){
    char recvline[10000];
    encrypt(packet,key,encrypted);	
    atm_send(atm, encrypted, strlen(encrypted));
    memset(recvline,'\0',10000);
    int n = atm_recv(atm,recvline,10000);
    recvline[n]=0;
    if(!decrypt(recvline,key,decrypted)){
        result = NULL;
        return;
    }
    printf("ATM recieved back %s\n",decrypted);
    parse_packet(decrypted,result);
}

void parse_packet(char *packet, char *temp){
    if(packet==NULL){
        temp=NULL;
    }
    char *parse=strtok(packet,"\n");
    //char t[100];
    int i=1;
    int flag=0;
    while(parse[i]!='>' && i<strlen(parse)){
        temp[i-1]=parse[i];	
        i++;	
    }
    //printf("packet contents: %s\n",t);
    temp[i-1]='\0';

}

int encrypt(char *message,char*key,unsigned char*encrypted){
    EVP_CIPHER_CTX ctx;
    memset(encrypted,'\0',10000);
    int ret=1;
    unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    EVP_CIPHER_CTX_init(&ctx);
    EVP_EncryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key, iv);
    int len1;
    if(!EVP_EncryptUpdate(&ctx,encrypted,&len1,message,strlen(message))){
        printf("Encrypt Update Error\n");
        ret= 0;
    }
    if(!EVP_EncryptFinal(&ctx,encrypted+len1,&len1)){
        printf("Encrypt Final Error\n");
        ret= 0;
    }
    EVP_CIPHER_CTX_cleanup(&ctx);
    return ret;
}

int decrypt(unsigned char *message,char*key, unsigned char*decrypted){
    EVP_CIPHER_CTX ctx;
    memset(decrypted,'\0',10000);
    int ret = 1;
    unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    EVP_CIPHER_CTX_init(&ctx);
    EVP_DecryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key, iv);
    int len1;
    if(!EVP_DecryptUpdate(&ctx,decrypted,&len1,message,strlen(message))){
        printf("Decrypt Update Error\n");
        ret = 0;
    }
    if(!EVP_DecryptFinal(&ctx,decrypted+len1,&len1)){
        printf("Decrypt Final Error\n");
        ret = 0;

    }

    EVP_CIPHER_CTX_cleanup(&ctx);
    return ret;
}




