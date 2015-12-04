#include "atm.h"
#include "ports.h"
#include "hash_table.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <openssl/evp.h>

#define MAX_INT 2147483647;

static int tries = 0;
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

char * atm_process_command(ATM *atm, char *command,char *key, HashTable *tries)
{
    char *str,*str1, *str2;
    str2=strtok(command,"\n");
    str=strtok(str2," ");
    char *packet = malloc(10000);
    memset(packet,'\0',10000);

    //balance	
    if(strcmp(str,"balance")==0){
        str = strtok(NULL," ");
        if(str != NULL){
            printf("Usage: balance\n");
            free(packet);
            packet=NULL;
            return session_token;
        }
        if(!strcmp(session_token,"")){
            printf("No user logged in\n");
            free(packet);
            packet=NULL;
            return session_token;
        }else{
            sprintf(packet,"<balance|%s>",session_token);
            char recvline[10000];
            //printf("sending packet:%s\n",packet);
            char packet_contents[10000];
            memset(packet_contents,'\0',10000);
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
                printf("Invalid Packet\n");
            }else{
                comm=strtok(NULL,"|");
                if(strcmp(comm,session_token)){
                    printf("Invalid Packet\n");
                }else{
                    comm=strtok(NULL,"|");
                    if(comm==NULL){
                        printf("Invalid Packet\n");

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
            printf("Usage: end-session\n");
            free(packet);
            packet=NULL;
            return session_token;
        }
        if(!strcmp(session_token,"")){
            printf("No user logged in\n");
        }else{
            printf("User logged out\n");
            memset(session_token,'\0',250);
        }
    }else{
        str1=str;
        //begin-session <username>
        if(strcmp(str1,"begin-session")==0){
            if(strcmp(session_token,"")){
                printf("A user is already logged in\n");
            }else{
                str1 = strtok(NULL," ");
                if(str1==NULL ){
                    printf("Usage: begin-session <user-name>\n");
                    free(packet);
                    packet=NULL;
                    return session_token;
                }else if(strlen(str1)>250){
                    printf("Usage: begin-session <user-name>\n");
                    free(packet);
                    packet=NULL;
                    return session_token;
                }else{
                    char *user = malloc(251);
                    memset(user,'\0',251);
                    strncpy(user,str1,strlen(str1));
                    if(!valid_user(str1)){
                        printf("Usage: begin-session <user-name>\n");
                        free(packet);
                        packet=NULL;
                        return session_token;
                    }
                    str1 = strtok(NULL," ");

                    if(str1 !=NULL){
                        printf("Usage: begin-session <user-name>\n");
                        free(packet);
                        packet=NULL;
                        return session_token;
                    }
                    sprintf(packet,"<authentication|%s>",user);
                    //printf("sending packet:%s\n",packet);

                    if (hash_table_find(tries, user) < 3){
                        if(authenticate(user, packet,atm,key, tries)){
                            strncpy(session_token,user,strlen(user));
                            printf("Authorized\n");
                            hash_table_del(tries, user);
                            hash_table_add(tries, user, 0);
                            //printf("num of tries: %d\n", 0);
                        }else{
                            printf("Not authorized\n");

                            int num;
                            num = hash_table_find(tries, user);
                            hash_table_del(tries, user);
                            hash_table_add(tries, user, num + 1);
                            //printf("num of tries: %d\n", num);
                        }
                    }else{
                        printf("%s's account is currently locked. Please contact the bank to unlock it\n");
                    }
                    free(user);
                    user = NULL;
                }

            }
            //withdraw <amt>
        }else if(strcmp(str1,"withdraw")==0){
            if(!strcmp(session_token,"")){
                printf("No user logged in\n");
            }else{
                str1 = strtok(NULL," ");
                if(str1==NULL){
                    printf("Usage: withdraw <amt>\n");;
                }else{
                    //check if digits and greater than or equal to zero
                    if(!all_digits(str1) || atoi(str1)< 0){
                        printf("Usage: withdraw <amt>\n");
                        free(packet);
                        packet=NULL;
                        return session_token;
                    }
		    char *ptr;
		    unsigned int tmp=strtoul(str1,&ptr,10);
		    unsigned int max = 2147483647; //max size of int
		    if(tmp > max){
			printf("Usage: withdraw <amt>\n");
			free(packet);
                        packet=NULL;
                        return session_token;
                    }

                    memset(packet,'\0',10000);
                    sprintf(packet,"<withdraw|%s|%s>",session_token,str1);
                    //printf("sending packet:%s\n",packet);
                    if(atoi(str1) < 0){
                        printf("Usage: withdraw <amt>\n");
                    }else{
                        char parsed[10000];
                        memset(parsed,'\0',10000);
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

int authenticate(char *user_name, char *packet, ATM *atm,char *key, HashTable *tries){
    int ret=0;	
    FILE *card_file;
    size_t size_end;
    char *argcpy=malloc(250);
    memset(argcpy,'\0',250);
    strncpy(argcpy,user_name,strlen(user_name));
    //sending packet to see if user exists
    char packet_contents[10000];
    memset(packet_contents,'\0',10000);
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
        printf("No such user\n");
        free(argcpy);
        return 0;
    }
    //
    char *c = strcat(argcpy,".card");
    card_file=fopen(c,"r");
    if(!card_file){ //card file does not exist
        printf("Unable to access %s\'s card\n",user_name);
        ret=0;
    }else{
        //check size of card file
        fseek(card_file,0L,SEEK_END);
        size_end=ftell(card_file); //gets size of card file
        fclose(card_file);
        char buf[size_end];
        card_file=fopen(c,"r");
        memset(buf,'\0',size_end);
        size_t bytes_read;
        bytes_read=fread(buf,size_end,1,card_file);	
        //ask for pin
        char sendline[10];
        char *user_pin; 
        //if someone has tried with incorrect pin more than 3 times

        printf("PIN? ");
        fgets(sendline, 10,stdin);
        user_pin=strtok(sendline,"\n");	
        //check pin size and all digits
        if(strlen(sendline)!=4 || !all_digits(sendline)){
            free(argcpy);
            return 0;
        }


        //decrypt card file with key
        char decrypt_card[10000];
        if(!decrypt(buf,comm,decrypt_card,size_end)){
	    printf("Invalid Packet\n");
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
    memset(recvline,'\0',10000);
    int out_size=0;
    int n;	
    encrypt(packet,key,encrypted,&out_size);	
    atm_send(atm, encrypted, out_size);
    n = atm_recv(atm,recvline,10000);
    recvline[n]=0;
    if(!decrypt(recvline,key,decrypted,n)){
        result = NULL;
        return;
    }
    //printf("ATM recieved back %s\n",decrypted);
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
    temp[i-1]='\0';

}

int encrypt(char *message,char*key,unsigned char*encrypted,int *out_size){
    EVP_CIPHER_CTX ctx;
    memset(encrypted,'\0',10000);
    int ret=1;
    unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    EVP_CIPHER_CTX_init(&ctx);
    EVP_EncryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key, iv);
    int len1,tmplen;
    if(!EVP_EncryptUpdate(&ctx,encrypted,&len1,message,strlen(message))){
        printf("Encrypt Update Error\n");
        ret= 0;
    }
    if(!EVP_EncryptFinal(&ctx,encrypted+len1,&tmplen)){
        printf("Encrypt Final Error\n");
        ret= 0;
    }
    len1+=tmplen;
    *out_size=len1;
    EVP_CIPHER_CTX_cleanup(&ctx);
    return ret;
}

int decrypt(unsigned char *message,char*key, unsigned char*decrypted, int cipher_size){
    EVP_CIPHER_CTX ctx;
    int ret =1;
    memset(decrypted,'\0',10000);
    unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    EVP_CIPHER_CTX_init(&ctx);
    EVP_DecryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key, iv);
    int len1;
    if(!EVP_DecryptUpdate(&ctx,decrypted,&len1,message,cipher_size)){
        printf("Decrypt Update Error\n");
        ret=0;
    }
    if(!EVP_DecryptFinal(&ctx,decrypted+len1,&len1)){
        printf("Decrypt Final Error\n");
        ret=0;

    }
    EVP_CIPHER_CTX_cleanup(&ctx);
    return ret;
}

int valid_user(char *user_name){
    //have already check if null
    if(!user_name){
        return 0;
    }

    int i;
    for (i = 0; i < strlen(user_name); i++){
        int ascii=(int)user_name[i];
        if(ascii < 65 || ascii > 122){
            return 0;
        }else if(ascii > 90 &&  ascii < 97){
            return 0;
        }
    }
    return 1;
}




