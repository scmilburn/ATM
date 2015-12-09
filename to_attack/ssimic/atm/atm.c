#include "atm.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

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
    atm->Session=0;
    atm->lockout=0;
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
	
	char head[300];
	strncpy(head, command, 300);
	char *getting = malloc(sizeof(char) * 5);
	memcpy(getting, "cont",4);
	getting[4] = '\0';
	char *token;
	
	if((strcmp(getting,"cont")==0) && strncmp(head,"begin-session", 13) == 0){
		if(atm->Session == 0){
			token = strtok(head, " ");
			token = strtok(NULL," ");
			if(strlen(token) <= 250 && (strcmp(getting,"cont")==0)){
				char *theName = malloc(sizeof(char) * strlen(token));
				memcpy(theName,token,strlen(token));
				theName[strlen(token)-1] = '\0';
				
				char *user = malloc(sizeof(char) * strlen(token) + 5);
				memcpy(user,"user ", 5);
				user[5]='\0';
				int len = strlen(token)-1;
				memcpy(user+5,token, len);
				user[len+5]='\0';
			
				token = strtok(NULL," ");
				if(token == NULL && (strcmp(getting,"cont")==0)){
					char recvline[5];
		    			int n;
			
		    			atm_send(atm, user, strlen(user));
		    			n = atm_recv(atm,recvline,5);
		    			recvline[n]=0;

					char com[257];
					sprintf(com,"./%s.card",theName);

					
					if(strncmp(recvline,"yes", 3) == 0 && (strcmp(getting,"cont")==0)){
						FILE *fp;
						if((fp = fopen(com,"r")) != NULL) {
							int myPin;
							printf("PIN? ");
							scanf("%d", &myPin);
						
							char sendPin[300];
							sprintf(sendPin, "pin|%d|%s|", myPin, theName);
							sendPin[strlen(sendPin)]='\0';
							char recievePin[5];
				    			int x;
			
				    			atm_send(atm, sendPin, strlen(sendPin));
				    			x = atm_recv(atm,recievePin,4);
				    			recievePin[x]=0;

							if(strncmp(recievePin,"good",4) == 0 && (strcmp(getting,"cont")==0)){
								printf("Authorized\n");
								atm->Session=1;
								atm->name = malloc(sizeof(char) * strlen(theName));
								memcpy(atm->name,theName,strlen(theName));
								atm->name[strlen(theName)] = '\0';
							}
							else{
								printf("Not authorized\n");
						
							}
							fclose(fp);
							sendPin[0] = '\0';
							recievePin[0] = '\0';
						}
						else{
							printf("Unable to access %s's card\n", user);
						}
					}
					else{
						printf("No such user\n");
					}
					recvline[0] = '\0';
					com[0] = '\0';
				}
				else{
					printf("Usage: begin-session <user-name>\n");
				}
				free(user);
				free(theName);
			}else{
				printf("Usage: begin-session <user-name>\n");
			}
		}
		else{
			printf("A user is already logged in\n");
		}
		
	}
	else if((strcmp(getting,"cont")==0) && strncmp(head,"withdraw", 8) == 0){
		if(atm->Session == 1){
			token = strtok(head, " ");
			token = strtok(NULL," ");

			if(token != NULL && (strcmp(getting,"cont")==0)){
				char *balanceStr = malloc(sizeof(char) * strlen(token));
				unsigned int howMuch;

				memcpy(balanceStr, token, strlen(token));
				balanceStr[strlen(token)-1] = '\0';
				token = strtok(NULL," ");
				if(((token == NULL) && (balanceStr[0] != '-')) && (strcmp(getting,"cont")==0)){
					char *ptr;
					howMuch = strtoul(balanceStr, &ptr,10);
				
					char sendPin[300];
					sprintf(sendPin, "getMoney|%u|%s|", howMuch, atm->name);
					sendPin[strlen(sendPin)]='\0';
					char recievePin[5];
		    			int x;
		
		    			atm_send(atm, sendPin, strlen(sendPin));
		    			x = atm_recv(atm,recievePin,4);
		    			recievePin[x]=0;

					if((strncmp(recievePin,"good",4) == 0)  && (strcmp(getting,"cont")==0)){
						printf("$%u dispensed\n",howMuch);
					
					}
					else{
						printf("Insufficient funds\n");
					}
					sendPin[0] = '\0';
					recievePin[0] = '\0';
				}else{
					printf("Usage: withdraw <amt>\n");
				}
				free(balanceStr);
			}
			else{
				printf("Usage: withdraw <amt>\n");
			}
		}
		else{
			printf("No user logged in\n");
		}
	}
	else if((strcmp(getting,"cont")==0) && (strncmp(head,"balance", 7) == 0)){
		if(atm->Session == 1){
			if(strlen(head) == 8 && (strcmp(getting,"cont")==0)){
				char sendPin[300];
				sprintf(sendPin, "getBalance|%s|", atm->name);
				sendPin[strlen(sendPin)]='\0';
				char recievePin[15];
	    			int x;

				atm_send(atm, sendPin, strlen(sendPin));
	    			x = atm_recv(atm,recievePin,17);
	    			recievePin[x]=0;

				if((strncmp(recievePin,"good",4) == 0) && (strcmp(getting,"cont")==0)){
					token = strtok(recievePin, "|");
					token = strtok(NULL,"|");
					printf("$%s\n",token);
					
				}

				sendPin[0] = '\0';
				recievePin[0] = '\0';

			}
			else{
				printf("Usage: balance\n");
			}
		}
		else{
			printf("No user logged in\n");
		}


	}
	else if((strcmp(getting,"cont")==0) && ((strncmp(head,"end-session", 11) == 0) && (strlen(head) == 12))){
		if(atm->Session == 1 && (strcmp(getting,"cont")==0)){
			atm->Session = 0;
			free(atm->name);
			printf("User logged out\n");
		}
		else{
			printf("No user logged in\n");
		}

	}
	else{
		printf("Invalid command\n");
	}
	head[0] = '\0';
}
