CC = gcc
CFLAGS = -fno-stack-protector -Wall -Iutil -Iatm -Ibank -Irouter -I.

all: bin/atm bin/bank bin/router

bin/atm : atm/atm-main.c atm/atm.c
	${CC} ${CFLAGS} atm/atm.c atm/atm-main.c -o bin/atm

bin/bank : bank/bank-main.c bank/bank.c
	${CC} ${CFLAGS} bank/bank.c bank/bank-main.c -o bin/bank

bin/router : router/router-main.c router/router.c
	${CC} ${CFLAGS} router/router.c router/router-main.c -o bin/router

test : util/list.c util/list_example.c util/hash_table.c util/hash_table_example.c
	${CC} ${CFLAGS} util/list.c util/list_example.c -o bin/list-test
	${CC} ${CFLAGS} util/list.c util/hash_table.c util/hash_table_example.c -o bin/hash-table-test

clean:
	cd bin && rm -f atm bank router list-test hash-table-test
