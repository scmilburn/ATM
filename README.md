# ATM/ Bank System
Project detailing a basic bank and ATM structure that send encrypted packets using AES\_256\_CBC encryption.
Maximum user-name length is 250 characters. Maximum balance is UINT\_MAX. Maximum withdrawal is INT\_MAX.  

####Bank commands
`create-user <user-name> <pin> <balance>`<br/>
`balance <user-name>`<br/>
`deposit <user-name> <amt>`<br/>

####ATM commands
`begin-session <user-name>`<br/>
`balance`<br/>
`withdraw <amt>`<br/>
`end-session`<br/>

####Usage
`make clean`<br/>
`make`<br/>

creates .atm and .bank files containing private keys<br/>
`bin/init <init_filename>`<br/>
`bin/bank <init_filename>.bank`<br/>
`bin/atm <init_filename>.atm`<br/>
`bin/router`<br/>
