#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

int numWords(char* string);
char **splitCommand(char *command);
int isValidPin(char* string);
int isValidBalance(char* string);
int isValidUsername(char* username);
char *encrypt_msg(int* length, char *msg, char* key, char* iv);
char *decrypt_msg(int length, char *cipher, char* key, char* iv);
#endif

