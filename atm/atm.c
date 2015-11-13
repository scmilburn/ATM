#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>

char *session_token="";

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

char * atm_process_command(ATM *atm, char *command)
{
	char *str,*str1;
	str=strtok(command,"\n");

	//balance	
	if(strcmp(str,"balance")==0){
		printf("asking about balance\n");
		if(!strcmp(session_token,"")){
			printf("there is not currently a session\n");
		}else{
		}
	//end-session
	}else if(strcmp(str,"end-session")==0){
		printf("asking to end session\n");
		if(!strcmp(session_token,"")){
			printf("No user logged in\n");
		}else{
			printf("User logged out\n");
			strncpy(session_token,"",1);
		}
	}else{
		str1=strtok(str," ");
		//begin-session <username>
		if(strcmp(str1,"begin-session")==0){
			if(strcmp(session_token,"")){
				printf("A user is already logged in\n");
			}else{
				str1 = strtok(NULL," ");
				int i = authenticate(str1);
				if(i){printf("Authenticated\n");}
				
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
					printf("wants to withdraw %s",str1);
				}			
			}
		}else{
			printf("Invalid command\n");
		}
	}
    	return session_token;

	//printf("end of function the token is %d\n",session_token);



    	/*char recvline[10000];
    	int n;
    	printf("sending %s\n",command);
    	atm_send(atm, command, strlen(command));
    	n = atm_recv(atm,recvline,10000);
    	recvline[n]=0;
    	fputs(recvline,stdout);
*/
	
}

int authenticate(char *user_name){
	int ret=0;
	if(user_name==NULL){
		printf("Invalid command\n");	
	}else{
		//gets card file
		FILE *card_file;
		char *packet = malloc(256);
		char *argcpy=malloc(strlen(user_name));
    		strcpy(argcpy,user_name); //////FIX 
		char *c = strcat(argcpy,".card");
		card_file=fopen(c,"r");
		if(!card_file){ //card file does not exist
			printf("No such user\n");
		}else{
			char recvline[10000];
			char sendline[10];
    			int n;
			printf("PIN?");
			fgets(sendline, 10,stdin);
			char *temp=strtok(sendline,"\n");	
			sprintf(packet,"<authentication|%s>",user_name);
			printf("sending packet:%s\n",packet);
			/*atm_send(atm, packet, strlen(packet));
    			n = atm_recv(atm,recvline,10000);
    			recvline[n]=0;*/
			//strncpy(session_token,str1,sizeof(str1));
			
			fclose(card_file);
			ret=1;
		}
		free(argcpy);
		free(packet);
	}

	return ret;
}

