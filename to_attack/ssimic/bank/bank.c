#include "bank.h"
#include "ports.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <limits.h>


Bank* bank_create()
{
    Bank *bank = (Bank*) malloc(sizeof(Bank));
    if(bank == NULL)
    {
        perror("Could not allocate Bank");
        exit(1);
    }

    // Set up the network state
    bank->sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&bank->rtr_addr,sizeof(bank->rtr_addr));
    bank->rtr_addr.sin_family = AF_INET;
    bank->rtr_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->rtr_addr.sin_port=htons(ROUTER_PORT);

    bzero(&bank->bank_addr, sizeof(bank->bank_addr));
    bank->bank_addr.sin_family = AF_INET;
    bank->bank_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    bank->bank_addr.sin_port = htons(BANK_PORT);
    bind(bank->sockfd,(struct sockaddr *)&bank->bank_addr,sizeof(bank->bank_addr));

    // Set up the protocol state
    // TODO set up more, as needed
	bank->htv = hash_table_create(100);
	bank->htp = hash_table_create(100);


    return bank;
}

void bank_free(Bank *bank)
{
    if(bank != NULL)
    {
        close(bank->sockfd);
        free(bank);
    }
}

ssize_t bank_send(Bank *bank, char *data, size_t data_len)
{
    // Returns the number of bytes sent; negative on error
    return sendto(bank->sockfd, data, data_len, 0,
                  (struct sockaddr*) &bank->rtr_addr, sizeof(bank->rtr_addr));
}

ssize_t bank_recv(Bank *bank, char *data, size_t max_data_len)
{
    // Returns the number of bytes received; negative on error
    return recvfrom(bank->sockfd, data, max_data_len, 0, NULL, NULL);
}

void bank_process_local_command(Bank *bank, char *command, size_t len)
{
    // TODO: Implement the bank's local commands
	char head[300];
	strncpy(head, command, 300);
	char *token;
	char *getting = malloc(sizeof(char) * 5);
	memcpy(getting, "cont",4);
	getting[4] = '\0';
	token = strtok(head," ");
	
	if((strcmp(getting,"cont")==0) && strncmp(token,"create-user",11)==0){
		
		//int userPin=0;
		unsigned int balance;

		//user-name
		token = strtok(NULL," ");
		char *user;
		if((token != NULL && (strcmp(getting,"cont")==0)) && (strlen(token) <= 250)){
			user = malloc(sizeof(char) * strlen(token));
			memcpy(user, token, strlen(token));
			user[strlen(token)] = '\0';

			//pin
			token = strtok(NULL," ");
			char *pin;
			if(((token != NULL) && (strcmp(getting,"cont")==0)) && (strlen(token) <= 4)){
				pin = malloc(sizeof(char) * strlen(token));
				memcpy(pin, token, strlen(token));
				pin[strlen(token)] = '\0';
				//userPin = atoi(pin);

				//balance
				token = strtok(NULL," ");
				char *balanceStr;
				if(token != NULL && (strcmp(getting,"cont")==0)){
					int numLen = strlen(token)-1;
					if(numLen < 11){
						balanceStr = malloc(sizeof(char) * strlen(token));
						memcpy(balanceStr, token, strlen(token));
						balanceStr[strlen(token)-1] = '\0';

						token = strtok(NULL," ");
						if(token == NULL && (strcmp(getting,"cont")==0)){
							if((hash_table_find(bank->htp,user) == NULL)  && (strcmp(getting,"cont")==0)){
								char *cardFile = malloc(sizeof(char) * (strlen(user)+6));
								memcpy(cardFile,user, strlen(user));
								memcpy(cardFile+strlen(user), ".card", 5);
								cardFile[strlen(user)+5] = '\0';

								FILE *fpbank;
								fpbank = fopen(cardFile, "w");
								if(fpbank == NULL){
									fclose(fpbank);
									printf("Error creating card file for user %s\n",user);
								}
								else{
									fclose(fpbank);

									char *ptr;
									balance = strtoul(balanceStr, &ptr,10);

									char *tot = malloc(sizeof(char) * numLen);
									sprintf(tot,"%u",balance);
									tot[strlen(tot)] = '\0';

									hash_table_add(bank->htp, user, pin);
									hash_table_add(bank->htv, user, tot);

									printf("Created user %s\n", user);
								}
							}
							else{
								printf("Error: user %s already exists\n", user);
							}
						}
						else{
							printf("Usage: create-user <user-name> <pin> <balance>\n");
						}
						free(balanceStr);
					}
					else{
						printf("Usage: create-user <user-name> <pin> <balance>\n");
					}
				}
				else{
					printf("Usage: create-user <user-name> <pin> <balance>\n");
				}
			}
			else{
				printf("Usage: create-user <user-name> <pin> <balance>\n");
			}	
		}
		else{
			printf("Usage: create-user <user-name> <pin> <balance>\n");
		}
	}
	else if((strcmp(getting,"cont")==0) && strncmp(token,"deposit",7)==0){

		char *user;
		char *balanceStr;
		unsigned int addThis;

		//user-name
		token = strtok(NULL," ");
		if(((token != NULL) && (strcmp(getting,"cont")==0)) && (strlen(token) <= 250)){
			user = malloc(sizeof(char) * strlen(token));
			memcpy(user, token, strlen(token));
			user[strlen(token)] = '\0';

			//balance
			token = strtok(NULL," ");
			if(token != NULL && (strcmp(getting,"cont")==0)){
				int numLen = strlen(token)-1;
				balanceStr = malloc(sizeof(char) * strlen(token));
				memcpy(balanceStr, token, strlen(token));
				balanceStr[strlen(token)-1] = '\0';
				
				if(balanceStr[0] != '-'){
					token = strtok(NULL," ");
					if(token == NULL && (strcmp(getting,"cont")==0)){

						if((strcmp(getting,"cont")==0) && (hash_table_find(bank->htv,user) != NULL)){
							char *ptr;
							addThis = strtoul(balanceStr, &ptr,10);
							unsigned int oldBal = strtoul(hash_table_find(bank->htv,user),&ptr,10);
							unsigned int newTotal;
							
							if((addThis>0) || ((balanceStr[0]=='0')&&strlen(balanceStr)==1)){
								if(oldBal <= (ULONG_MAX - addThis)){
									newTotal = addThis + oldBal;

									char *tot = malloc(sizeof(char) * numLen);
									sprintf(tot,"%u",newTotal);
									tot[strlen(tot)] = '\0';
									
									hash_table_del(bank->htv, user);
									hash_table_add(bank->htv, user, tot);
									//printf("New balance: %u\n", newTotal);
									printf("$%u added to %s's account\n", addThis, user);
								}else{
									printf("Too rich for this program\n");
								}
							}else{
								printf("Usage: deposit <user-name> <amt>\n");
							}
						}else{
							printf("No such user\n");
						}
					}else{
						printf("Usage: deposit <user-name> <amt>\n");
					}
				}else{
					printf("Usage: deposit <user-name> <amt>\n");
				}
				free(balanceStr);
			}else{
				printf("Usage: deposit <user-name> <amt>\n");
			}
		}else{
			printf("Usage: deposit <user-name> <amt>\n");
		}
	}
	else if((strcmp(getting,"cont")==0) && strncmp(token,"balance",7)==0){
		char *user;

		//user-name
		token = strtok(NULL," ");
		if(((token != NULL) && (strcmp(getting,"cont")==0)) && (strlen(token) <= 250)){
			user = malloc(sizeof(char) * strlen(token));
			memcpy(user, token, strlen(token));
			user[strlen(token)-1] = '\0';
			
			token = strtok(NULL," ");
			if(token == NULL && (strcmp(getting,"cont")==0)){

				if((hash_table_find(bank->htv,user) != NULL)  && (strcmp(getting,"cont")==0)){
					char *ptr;
					unsigned int balance = strtoul(hash_table_find(bank->htv,user),&ptr,10);
					printf("$%u\n", balance);
				}
				else{
					printf("No such user\n");
				}
			}
			else{
				printf("Usage: balance <user-name>\n");
			}
		}
		else{
			printf("Usage: balance <user-name>\n");
		}
		free(user);
	}
	else{
		printf("Invalid command\n");
	}
}

void bank_process_remote_command(Bank *bank, char *command, size_t len)
{
    // TODO: Implement the bank side of the ATM-bank protocol

	/*
	 * The following is a toy example that simply receives a
	 * string from the ATM, prepends "Bank got: " and echoes 
	 * it back to the ATM before printing it to stdout.
	 */
	
	char head[300];
	strncpy(head, command, len);
	head[len] = '\0';
	char *getting = malloc(sizeof(char) * 5);
	memcpy(getting, "cont",4);
	getting[4] = '\0';
	char *token;

	if((strcmp(getting,"cont")==0) && (strncmp(head,"user",4)==0)){
		token = strtok(head," ");
		token = strtok(NULL,"\n");
		if((token != NULL) && (strcmp(getting,"cont")==0)){
			char *user = malloc(sizeof(char) * strlen(token));
			memcpy(user, token, strlen(token));
			user[strlen(token)] = '\0';
			if(hash_table_find(bank->htv,user) != NULL  && (strcmp(getting,"cont")==0)){
				command[len]=0;
   				bank_send(bank, "yes", 3);
			}
			else{
		    		command[len]=0;
		   		bank_send(bank, "nop", 3);
			}
			free(user);
		}
	}
	else if((strcmp(getting,"cont")==0) && (strncmp(head,"pin",3) == 0)){
		token = strtok(head,"|");
		token = strtok(NULL,"|");
		int testAgainst;
		
		char *thePin = malloc(sizeof(char) * strlen(token));
		memcpy(thePin,token,strlen(token));
		thePin[strlen(token)] = '\0';
		int pinNum = atoi(thePin);	

		token = strtok(NULL,"|");
		char *user = malloc(sizeof(char) * strlen(token));
		memcpy(user, token, strlen(token));
		user[strlen(token)] = '\0';
		
		if((hash_table_find(bank->htp,user) != NULL)  && (strcmp(getting,"cont")==0)){
			char *testing = malloc(sizeof(char)*4);
			memcpy(testing,hash_table_find(bank->htp,user),5);
			testing[5]='\0';

			testAgainst = atoi(testing);
			if(pinNum == testAgainst){
				bank_send(bank, "good", 4);
			}
			else{
				bank_send(bank, "stop", 4);
			}
			free(testing);
		}

		free(thePin);
		free(user);
	}
	else if((strcmp(getting,"cont")==0) && (strncmp(head,"getMoney",8) == 0)){
		token = strtok(head,"|");
		token = strtok(NULL,"|");
		unsigned int howMuch;

		char *ptr;
		howMuch = strtoul(token, &ptr,10);
		
		token = strtok(NULL,"|");
		char *user = malloc(sizeof(char) * strlen(token));
		memcpy(user, token, strlen(token));
		user[strlen(token)] = '\0';

		if((hash_table_find(bank->htp,user) != NULL) && (strcmp(getting,"cont")==0)){
			unsigned int testAgainst = strtoul(hash_table_find(bank->htv,user),&ptr,10);
			if(testAgainst >= howMuch){
				unsigned int newTotal;

				newTotal = testAgainst-howMuch;

				char *tot = malloc(sizeof(char) * len);
				sprintf(tot,"%u",newTotal);
				tot[strlen(tot)] = '\0';

				hash_table_del(bank->htv, user);
				hash_table_add(bank->htv, user, tot);
				bank_send(bank, "good", 4);
			}
			else{
				bank_send(bank, "stop", 4);
			}
		}

		//free(user);
	}
	else if((strcmp(getting,"cont")==0) && strncmp(head,"getBalance",10) == 0){		
		token = strtok(head,"|");
		token = strtok(NULL,"|");

		char *user = malloc(sizeof(char) * strlen(token));
		memcpy(user, token, strlen(token));
		user[strlen(token)] = '\0';
		char *ptr;
		unsigned int testAgainst = strtoul(hash_table_find(bank->htv,user),&ptr,10);
		char sendPin[16];
		sprintf(sendPin, "good|%u|", testAgainst);
		sendPin[strlen(sendPin)]='\0';

		bank_send(bank, sendPin, strlen(sendPin));

		free(user);
	}
	else{
		fprintf(stdout,"You shouldn't get here\n");
		fflush(stdout);
	}

   	
}
