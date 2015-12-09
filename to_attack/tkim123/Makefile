CC = gcc
CFLAGS = -fno-stack-protector -Wall -Iutil -Iatm -Ibank -Irouter -I. 
LIBS = -lcrypto

all: function bin/atm bin/bank bin/router bin/init 

function: util/functions.c
	${CC} -DLINUX -I/usr/local/ssl/include -L/usr/local/ssl/lib ${LIBS} util/functions.c -c

bin/atm : atm/atm-main.c atm/atm.c util/hash_table.c util/functions.c
	${CC} ${CFLAGS} ${LIBS} atm/atm.c atm/atm-main.c functions.o util/hash_table.c util/list.c -o bin/atm 

bin/bank : bank/bank-main.c bank/bank.c util/functions.c
	${CC} ${CFLAGS} ${LIBS} bank/bank.c bank/bank-main.c functions.o util/list.c util/hash_table.c -o bin/bank 

bin/router : router/router-main.c router/router.c util/functions.c
	${CC} ${CFLAGS} ${LIBS} router/router.c router/router-main.c functions.o -o bin/router

bin/init : init/init-main.c util/functions.c
	${CC} ${CFLAGS} ${LIBS} init/init-main.c functions.o -o bin/init 

test : util/list.c util/list_example.c util/hash_table.c util/hash_table_example.c
	${CC} ${CFLAGS} util/list.c util/list_example.c -o bin/list-test
	${CC} ${CFLAGS} util/list.c util/hash_table.c util/hash_table_example.c -o bin/hash-table-test

clean:
	rm -f *.card
	rm -f test.*
	rm -f *functions.o
	cd bin && rm -f atm bank router init list-test hash-table-test 
