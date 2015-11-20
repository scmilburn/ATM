#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <openssl/evp.h>

static const char session_token[250];
unsigned char encrypted[1000];
unsigned char decrypted[1000];

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
		printf("asking about balance\n");
		if(!strcmp(session_token,"")){
			printf("there is not currently a session\n");
		}else{
			sprintf(packet,"<balance|%s>",session_token);
			printf("sending packet:%s\n",packet);
			//unsigned char encrypted[1000];
			//memset(encrypted,'\0',1000);
			char recvline[1000];
			encrypt(packet,key,encrypted);	
			atm_send(atm, encrypted, strlen(encrypted));
    			int n = atm_recv(atm,recvline,1000);
    			recvline[n]=0;
			decrypt(recvline,key,decrypted);
			printf("ATM recieved back %s\n",decrypted);
			char *packet_contents[strlen(decrypted)];
			parse_packet(decrypted,packet_contents);
			//printf("message contents is now %s\n",packet_contents);
			char *comm=strtok(packet_contents,"|");
			if(strcmp(comm,"balance")){
				printf("Invalid packet\n");
			}else{
				comm=strtok(NULL,"|");
				if(strcmp(comm,session_token)){
					printf("Invalid packet\n");
				}else{
					comm=strtok(NULL,"|");
					if(comm==NULL){
						printf("Invalid packet\n");
					}else{
					printf("$%s\n",comm);
					}
				}
			}

			
			
		}

	//end-session
	}else if(strcmp(str,"end-session")==0){
		printf("asking to end session\n");
		if(!strcmp(session_token,"")){
			printf("No user logged in\n");
		}else{
			printf("User logged out\n");
			memset(session_token,'\0',250);
		}
	}else{
				puts("HERE");
		str1=strtok(str," ");

		//begin-session <username>
		if(strcmp(str1,"begin-session")==0){
			if(strcmp(session_token,"")){
				printf("A user is already logged in\n");
			}else{
				str1 = strtok(NULL," ");
				if(str1==NULL){
					printf("Invalid command\n");
				}else{
					char sendline[5];
    					int n;
					printf("PIN?");
					fgets(sendline, 5,stdin);
//check size of pin
					char *pin=strtok(sendline,"\n");	
					sprintf(packet,"<authentication|%s>",str1);
					printf("sending packet:%s\n",packet);
					if(authenticate(str1, packet,atm,key,pin)){
						strcpy(session_token,str1);
						printf("Authenticated\n");
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
//CHECK IF AMOUNT IF VALID
					sprintf(packet,"<withdraw|%s|%s>",session_token,str1);
					printf("sending packet:%s\n",packet);
					//withdraw(packet,key,atm,str1);
					if(atoi(str1) < 0){
						printf("Invalid command\n");
					}else{
					char parsed[1000];
					send_and_recv(atm,packet,key,parsed);
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

	//printf("end of function the token is %d\n",session_token);



/*    	char recvline[10000];
    	int n;
    	printf("sending %s\n",command);
    	atm_send(atm, command, strlen(command));
    	n = atm_recv(atm,recvline,10000);
    	recvline[n]=0;
    	fputs(recvline,stdout);
*/
	
}


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
		char packet_contents[1000];
		send_and_recv(atm,packet,key,packet_contents);
		char *comm=strtok(packet_contents,"|");
		//printf("command is %s\n",comm);
		if(comm == NULL || strcmp(comm,"authentication")){
			printf("invalid packet\n");
			return 0;
		}
		comm = strtok(NULL,"|");
		//printf("The key is %s\n",comm);
		//decrypt card file with key
		char decrypt_card[10000];
		decrypt(buf,comm,decrypt_card);
		//printf("The decrypted card is %s\n",decrypt_card);
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
			//printf("pins match!\n");
			//session_token=user_name;
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
	char recvline[1000];
	encrypt(packet,key,encrypted);	
	atm_send(atm, encrypted, strlen(encrypted));
	memset(recvline,'\0',1000);
    	int n = atm_recv(atm,recvline,1000);
    	recvline[n]=0;
	decrypt(recvline,key,decrypted);
	printf("ATM recieved back %s\n",decrypted);
	//char *packet_contents[strlen(decrypted)];
	parse_packet(decrypted,result);
}

void parse_packet(char *packet, char *temp){
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

void encrypt(char *message,char*key,unsigned char*encrypted){
	EVP_CIPHER_CTX ctx;
	memset(encrypted,'\0',1000);
	unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	EVP_CIPHER_CTX_init(&ctx);
	EVP_EncryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key, iv);
	int len1;
	if(!EVP_EncryptUpdate(&ctx,encrypted,&len1,message,strlen(message))){
		printf("Encrypt Update Error\n");
	}
	if(!EVP_EncryptFinal(&ctx,encrypted+len1,&len1)){
		printf("Encrypt Final Error\n");
	}
	EVP_CIPHER_CTX_cleanup(&ctx);

}

void decrypt(unsigned char *message,char*key, unsigned char*decrypted){
	EVP_CIPHER_CTX ctx;
	memset(decrypted,'\0',1000);
	unsigned char iv[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	EVP_CIPHER_CTX_init(&ctx);
	EVP_DecryptInit_ex(&ctx,EVP_aes_256_cbc(),NULL,key, iv);
	int len1;
	if(!EVP_DecryptUpdate(&ctx,decrypted,&len1,message,strlen(message))){
		printf("Decrypt Update Error\n");
	}
	if(!EVP_DecryptFinal(&ctx,decrypted+len1,&len1)){
		printf("Decrypt Final Error\n");
						
	}
	//char * mess=strtok(decrypted,"\n");
	//decrypted=mess;
	EVP_CIPHER_CTX_cleanup(&ctx);
}




