//
//  list.h
//  LinkedListApp
//
//  Created by Andrew Alston on 05/09/2025.
//
#include <stdint.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdatomic.h>
#include "xxHash/xxh3.h"
#define HASHMAP_SIZE 20000

/* struct llist defines our linked list */
struct llist {
    struct llist *next; // Next entry in linked list
    struct llist *prev; // Previous entry in linked list
    void *data; // Data in linked list
    size_t data_size;
};

struct llist_collision {
    struct llist_collision *next;
    struct llist *entry;
    uint64_t hash;
};

struct llist_map {
    struct llist *entry;
    uint64_t hash;
    struct llist_collision *collision;
};

/* struct llist_container contains the list, as well as tracking pointers for the head and tail of the list */
struct llist_container {
    struct llist *list; // Linked list pointer that can point anywhere on the list
    struct llist *head; // Linked list pointer that should always point to the head of the list
    struct llist *tail; // Linked list pointer that should always point to the tail of the list
    bool is_ring; // If this is set then head and tail have no meaning since the linked list forms a complete ring
    size_t list_entries;
    _Atomic(bool) use_lock; // This is set if we are using locking - though not technically necessary
    _Atomic(bool) locked; // Atomic Lock
    struct llist_map *h_map[HASHMAP_SIZE];
};

#define USE_LOCK

#ifdef USE_LOCK
#define LOCK(container) ({ \
    while(atomic_load(&container->locked)) { \
        continue; \
    } \
    printf("Taking lock\n"); \
    atomic_store(&container->locked, true); \
})
#define UNLOCK(container) ({ printf("Unlocking\n"); atomic_store(&container->locked, false); })
#else
#define LOCK(container) ({})
#define UNLOCK(container) ({})
#endif

struct llist *llist_new(void *data, size_t d_size);
struct llist_container *container_new(void);
int llist_set_data(struct llist_container *cont, void *data, size_t d_size);
int llist_add_head_data(struct llist_container *cont, void *data, size_t d_size);
int llist_add_tail_data(struct llist_container *cont, void *data, size_t d_size);
int llist_add_current_data(struct llist_container *cont, void *data, size_t d_size);
int llist_swap_entries(struct llist_container *cont, struct llist *first, struct llist *second);
struct llist_container *container_new_ring(int ring_entries);
int list_set_from_array(struct llist_container *cont, void *array_head, size_t entry_size, size_t n_entries);
struct llist_container *container_new_list(int list_entries);
int llist_insert_between(struct llist_container *cont, struct llist *first, struct llist *second, void *data, size_t d_size);
int llist_delete_node(struct llist_container *cont, struct llist *node, bool free_data);
int llist_insert_data_copy(struct llist_container *cont, struct llist *node, void *data, size_t d_size);
struct llist_map **hash_map_create(struct llist_container *cont);
static inline bool llist_compare_entries(struct llist *entry1, struct llist *entry2);


