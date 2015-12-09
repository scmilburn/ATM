#include "list.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
    List *ls = list_create();
    printf("Charlie -> %s\n", (list_find(ls, "Charlie") == NULL ? "Not Found" : "FAIL"));
    list_add(ls, "Alice", "123");
    list_add(ls, "Bob", "345");

    printf("Alice -> '%s'\n", (char*) list_find(ls, "Alice"));
    printf("Bob -> '%s'\n", (char*) list_find(ls, "Bob"));

    printf("Charlie -> %s\n", (list_find(ls, "Charlie") == NULL ? "Not Found" : "FAIL"));

    printf("Size = %d\n", ls->size);
    list_add(ls, "Alice", "456");
    printf("Size = %d\n", ls->size);
    list_free(ls);

	return EXIT_SUCCESS;
}

