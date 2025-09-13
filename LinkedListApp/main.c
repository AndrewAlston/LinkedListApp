//
//  main.c
//  LinkedListApp
//
//  Created by Andrew Alston on 05/09/2025.
//

#include <stdio.h>
#include "list.h"
// #include "xxHash.h"

int main(int argc, const char * argv[]) {
#ifdef USE_LOCK
    printf("Locking enabled\n");
#endif
    uint64_t test[20] = {};
    for(int i = 0; i < 20; i++) {
        test[i] = rand();
    }
    struct llist_container *container = container_new_list(20);
    container->list = container->head;
    list_set_from_array(container, test, sizeof(uint64_t), 20);
    printf("Completed list fill\n");
    LOCK(container);
    container->list = container->head;
    for(int i = 0; i < 20; i++) {
        printf("Linked list entry contained %d\n", *(int *)container->list->data);
        container->list = container->list->next;
    }
    printf("Completed list dump\n");
    container->list = container->tail;
    for(int i = 0; i < 20; i++) {
        printf("Linked list entry contained %d\n", *(int*)container->list->data);
        container->list = container->list->prev;
    }
    container->list = container->head;
    UNLOCK(container);
    llist_swap_entries(container, container->head, container->tail);
    LOCK(container);
    container->list = container->head;
    for(int i = 0; i < 20; i++) {
        printf("Linked list entry contained %d\n", *(int *)container->list->data);
        container->list = container->list->next;
    }
    UNLOCK(container);
    llist_insert_between(container, container->head, container->head->next, &test[15], sizeof(int));
    LOCK(container);
    container->list = container->head;
    for(int i = 0; i < 5; i++) {
        printf("Linked list entry contained %d\n", *(int*)container->list->data);
        container->list = container->list->next;
    }
    UNLOCK(container);
    struct llist_map **h_map = hash_map_create(container);
    for(int i = 0; i < HASHMAP_SIZE; i++) {
        if(h_map[i]) {
            printf("Hash entry %llu had data %llu\n", h_map[i]->hash, *(uint64_t*)h_map[i]->entry->data);
        }
    }
    
    return 0;
}
