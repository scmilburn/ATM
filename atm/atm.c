#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>

int session_token=0;

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

void atm_process_command(ATM *atm, char *command)
{
    // TODO: Implement the ATM's side of the ATM-bank protocol

	/*
	 * The following is a toy example that simply sends the
	 * user's command to the bank, receives a message from the
	 * bank, and then prints it to stdout.
	 */
    
    //checking valid command
    
	//printf("%s\n",command);
	char *str,*str1;
	str=strtok(command,"\n");
	//printf("first token is %s\n",t);
	if(strcmp(str,"balance")==0){
		printf("asking about balance\n");
		if(session_token==0){
			printf("there is not currently a session\n");
		}
	}else if(strcmp(str,"end-session")==0){
		printf("asking to end session\n");
		if(!session_token){
			printf("No user logged in\n");
		}else if(session_token){
			printf("User logged out\n");
			session_token = 0;
		}
	}else{
		str1=strtok(str," ");
		if(strcmp(str1,"begin-session")==0){
			if(session_token==1){
				printf("A user is already logged in\n");
			}else if(session_token==0){
				str1 = strtok(NULL," ");
				printf("asking to begin a session with %s\n",str1);
				//read from card file and ask for PIN here
				session_token = 1;
			}
		}else if(strcmp(str1,"withdraw")==0){
			printf("asking to withdraw\n");
			if(session_token==0){
				printf("No user logged in\n");
			}else if(session_token==1){
				str1 = strtok(NULL," ");
				printf("wants to withdraw %s",str1);
			}
		}
	}
    	

	//printf("end of function the token is %d\n",session_token);



    	/*char recvline[10000];
    	int n;
    	printf("sending %s\n",command);
    	atm_send(atm, command, strlen(command));
    	n = atm_recv(atm,recvline,10000);
    	recvline[n]=0;
    	fputs(recvline,stdout);*/

}
