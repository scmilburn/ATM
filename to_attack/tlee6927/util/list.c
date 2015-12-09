#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "list.h"

List* list_create()
{
    List *list = (List*) malloc(sizeof(List));
    list->head = list->tail = NULL;
    list->size = 0;
    return list;
}

void list_free(List *list)
{
    if(list != NULL)
    {
        ListElem *curr = list->head;
        ListElem *next;
        while(curr != NULL)
        {
            next = curr->next;
            free(curr);
            curr = next;
        }
        free(list);
    }
}

void* list_find(List *list, const char *key)
{
    if(list == NULL)
        return NULL;

    ListElem *curr = list->head;
    while(curr != NULL)
    {
        if(strcmp(curr->key, key) == 0)
            return curr->val;
        curr = curr->next;
    }

    return NULL;
}

void list_add(List *list, char *key, void *val)
{
    // Allow duplicates
    // assert(list_find(list, key) == NULL);

    ListElem *elem = (ListElem*) malloc(sizeof(ListElem));
    elem->key = key;
    elem->val = val;
    elem->next = NULL;

    if(list->tail == NULL)
        list->head = list->tail = elem;
    else
        list->tail->next = elem;

    list->size++;
}

void list_del(List *list, const char *key)
{
    // Remove the element with key 'key'
    ListElem *curr, *prev;

    curr = list->head;
    prev = NULL;
    while(curr != NULL)
    {
        if(strcmp(curr->key, key) == 0)
        {
            // Found it: now delete it

            if(curr == list->tail)
                list->tail = prev;

            if(prev == NULL)
                list->head = list->head->next;
            else
                prev->next = curr->next;

            list->size--;

            free(curr);
            return;
        }

        prev = curr;
        curr = curr->next;
    }
}

uint32_t list_size(const List *list)
{
    return list->size;
}
