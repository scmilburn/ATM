# ATM/ Bank System
Project detailing a basic bank and ATM structure that send encrypted packets using AES\_256\_CBC encryption.
Max user-name length is 250 characters. Maximum balance is UINT\_MAX. Maximum withdrawal is INT\_MAX.  

####Bank commands
create-user <user-name> <pin> <balance>
balance <user-name>
deposit <user-name> <amt>

####ATM commands
begin-session <user-name>
balance
withdraw <amt>
end-session

####Usage
`make clean`
`make`

creates .atm and .bank files containing private keys
`bin/init <filename>`
`bin/bank <init_filename>.bank`
`bin/atm <init_filename>.atm`
`bin/router`
