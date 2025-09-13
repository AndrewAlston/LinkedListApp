//
//  list.c
//  LinkedListApp
//
//  Created by Andrew Alston on 05/09/2025.
//

#include "list.h"
// #include "xxHash.h"

/* llist_new creates a new linked list entry with data*/
struct llist *llist_new(void *data, size_t d_size) {
    struct llist *new = calloc(1, sizeof(struct llist));
    if(!new)
        return NULL;
    new->data = data;
    new->data_size = d_size;
    return new;
}

/* container_new creates a new linked list container and the initial first linked list entry */
struct llist_container *container_new(void) {
    struct llist_container *new = calloc(1, sizeof(struct llist_container));
    if(!new)
        return NULL;
    for(int i = 0; i < HASHMAP_SIZE; i++) {
        new->h_map[i] = NULL;
    }
#ifdef USE_LOCK
    atomic_store(&new->use_lock, true);
#else
    atomic_store(&new->use_lock, false);
#endif
    return new;
}

/* This creates a container and a complete ring - meaning that the tail entry will point to the head entry */
struct llist_container *container_new_ring(int ring_entries) {
    struct llist_container *new = calloc(1, sizeof(struct llist_container));
    struct llist *current = NULL;
    if(!new)
        return NULL;
#ifdef USE_LOCK
    atomic_store(&new->use_lock, true);
#else
    atomic_store(&new->use_lock, false);
#endif
    LOCK(new);
    for(int i = 0; i < ring_entries; i++) {
        if(i == 0) {
            current = new->head = new->list = llist_new(NULL, 0);
            if(!new->list) {
                printf("Failed adding initial ring entry - bailing\n");
                UNLOCK(new);
                exit(-1);
            }
            printf("New ring head at %p\n",new->head);
            continue;
        }
        current = new->list;
        new->list->next = llist_new(NULL, 0);
        if(!new->list->next) {
            printf("Failed creating ring entry %d, bailing\n", i);
            UNLOCK(new);
            exit(-1);
        }
        new->list->next->prev = current;
        current = new->list;
        new->list = new->list->next;
    }
    new->list->next = new->head;
    new->head->prev = new->list;
    new->list_entries = ring_entries;
    UNLOCK(new);
    return new;
}

/* container_new_list creates a container with a doubly linked list containing list_entries entries in the list */
struct llist_container *container_new_list(int list_entries) {
    struct llist_container *new = calloc(1, sizeof(struct llist_container));
    struct llist *current = NULL;
    if(!new)
        return NULL;
#ifdef USE_LOCK
    atomic_store(&new->use_lock, true);
#else
    atomic_store(&new->use_lock, false);
#endif
    
    LOCK(new);
    for(int i = 0; i < list_entries; i++) {
        if(i == 0) {
            current = new->head = new->list = llist_new(NULL, 0);
            if(!new->list) {
                printf("Failed adding initial ring entry - bailing\n");
                UNLOCK(new);
                free(new);
                return NULL;
            }
            new->list_entries++;
            continue;
        }
        current = new->list;
        new->list->next = llist_new(NULL, 0);
        if(!new->list->next) {
            printf("Failed allocating for next list entry, freeing up and bailing\n");
            new->list = new->head;
            while(new->list->next) {
                new->list = new->list->next;
                free(new->list->prev);
            }
            free(new->list);
            UNLOCK(new);
            free(new);
            return NULL;
        }
        new->list_entries++;
        new->list->next->prev = new->list;
        current = new->list;
        new->list = new->list->next;
    }
    new->tail = new->list;
    new->tail->prev = current;
    UNLOCK(new);
    printf("Completed list build with %lu entries\n",new->list_entries);
    return new;
}

int list_set_from_array(struct llist_container *cont, void *array_head, size_t entry_size, size_t n_entries) {
    if(!cont)
        return -1;
    LOCK(cont);
    if(cont->list_entries < n_entries) {
        if(cont->is_ring) {
            printf("Warning: array entries exceeds ring entries - early array entries will be overwritten on the ring\n");
        } else {
            printf("Insufficient space in list for number of fill entries\n");
            return -1;
        }
    }
    for(int i = 0; i < n_entries; i++) {
        cont->list->data = array_head;
        cont->list->data_size = entry_size;
        cont->list = cont->list->next;
        array_head += entry_size;
    }
    UNLOCK(cont);
    return 0;
}


/* llist_set_data sets the data pointer in the last entry of the list defined inside the container */
int llist_set_data(struct llist_container *cont, void *data, size_t d_size) {
    // Container doesnt exist - bail
    if(!cont)
        return -1;
    // Lock the container if we are using locks
    LOCK(cont);
    // If there is no tail entry - and we are using locks, unlock the container and bail
    if(!cont->tail) {
        UNLOCK(cont);
        return -1;
    }
    cont->tail->data = data;
    cont->tail->data_size = d_size;
    UNLOCK(cont);
    return 0;
}

/* llist_add_head adds a new head entry to the linked list in the container with relevant data */
int llist_add_head_data(struct llist_container *cont, void *data, size_t d_size) {
    if(!cont)
        return -1;
    struct llist *new_entry = llist_new(data, d_size);
    if(!new_entry)
        return -1;
    LOCK(cont);
    if(!cont->head) {
        if(cont->tail) {
            printf("Something broke, no head entry with existing tail entry\n");
            free(new_entry);
            UNLOCK(cont);
            return -1;
        }
        cont->head = cont->tail = cont->list = new_entry;
        UNLOCK(cont);
        return 0;
    }
    new_entry->next = cont->head;
    cont->head->prev = new_entry;
    cont->head = new_entry;
    UNLOCK(cont);
    return 0;
}

/* llist_add_tail adds a new tail entry to the linked list in the container with relevant data */
int llist_add_tail_data(struct llist_container *cont, void *data, size_t d_size) {
    if(!cont)
        return -1;
    struct llist *new_entry = llist_new(data, d_size);
    if(!new_entry)
        return -1;
    LOCK(cont);
    if(!cont->tail) {
        if(cont->head) {
            printf("Something broke, no tail entry with existing head entry\n");
            free(new_entry);
            UNLOCK(cont);
            return -1;
        }
        cont->head = cont->tail = cont->list = new_entry;
        UNLOCK(cont);
        return 0;
    }
    cont->tail->next = new_entry;
    new_entry->prev = cont->tail;
    cont->tail = new_entry;
    UNLOCK(cont);
    return 0;
}

/* llist_add_current_data inserts a new linked list entry at the current list pointer */
int llist_add_current_data(struct llist_container *cont, void *data, size_t d_size) {
    if(!cont)
        return -1;
    // Deal with the case of there being no current list entry in the container
    struct llist *new_entry = llist_new(data, d_size);
    LOCK(cont);
    if(!cont->list) {
        if(!cont->is_ring) {
            if(cont->tail || cont->head) {
                // Something broke - we have a list entry but no head or tail and this isn't a ring
                printf("Something broke, current list entry with no list head or tail and not a ring structure\n");
                free(new_entry);
                UNLOCK(cont);
                return -1;
            }
        }
        // This is a ring with zero entries, head and tail don't matter so just set the new entry as the list and get out
        cont->list = new_entry;
        UNLOCK(cont);
        return 0;
    }
    if(cont->list->prev) {
        new_entry->prev = cont->list->prev;
        cont->list->prev->next = new_entry;
    }
    if(cont->list->next) {
        new_entry->next = cont->list->next;
        cont->list->next->prev = new_entry;
    }
    cont->list = new_entry;
    UNLOCK(cont);
    return 0;
}

/* llist_isadjacent returns true if two nodes are adjacent or returns false */
static inline bool llist_isadjacent(struct llist *first, struct llist *second) {
    if(!first || !second)
        return false;
    return (first->next == second && second->prev == first) || (first->prev == second && second->next == first);
}

/* llist_refresh_outer refreshes the outer pointers on a node */
static inline void llist_refresh_outer(struct llist *node) {
    if(node->prev != NULL)
        node->prev->next = node;
    if(node->next != NULL)
        node->next->prev = node;
}
/* llist_swap_entries swaps two entries in a linked list */
int llist_swap_entries(struct llist_container *cont, struct llist *first, struct llist *second) {
    LOCK(cont);
    if(!first || !second) {
        printf("One of the entries in the attempted swap was NULL, bailing\n");
        UNLOCK(cont);
        return -1;
    }
    if(cont->head == first && cont->tail == second) {
        cont->head = second;
        cont->tail = first;
    }
    
    struct llist *vector[4];
    struct llist *temp;
    
    if(second->next == first) {
        temp = first;
        first = second;
        second = temp;
    }
    
    vector[0] = first->prev;
    vector[1] = second->prev;
    vector[2] = first->next;
    vector[3] = second->next;
    
    if(llist_isadjacent(first, second)) {
        first->prev = vector[2];
        second->prev = vector[0];
        first->next = vector[3];
        second->next = vector[1];
    } else {
        first->prev = vector[1];
        second->prev = vector[0];
        first->next = vector[3];
        second->next = vector[2];
    }
    llist_refresh_outer(first);
    llist_refresh_outer(second);
    
    
    UNLOCK(cont);
    return 0;
 }

/* llist_insert_between inserts an entry between two adjacenct nodes */
int llist_insert_between(struct llist_container *cont, struct llist *first, struct llist *second, void *data, size_t d_size) {
    if(!cont)
        return -1;
    if(!llist_isadjacent(first, second)) {
        printf("Specified nodes are non-adjacent\n");
        return -1;
    }
    struct llist *new = llist_new(data, d_size);
    if(!new)
        return -1;
    new->prev = first;
    new->next = second;
    LOCK(cont);
    first->next = new;
    second->prev = new;
    cont->list_entries++;
    UNLOCK(cont);
    return 0;
}

/* llist_free_data will free up the data pointer if do_free is true */
static inline void llist_free_data(bool do_free, struct llist *node) {
    if(do_free && node->data)
        free(node->data);
}

/* llist_delete_node deletes a node in the linked list - if free_data is true it will also free up the data entry */
/* This function has been modified to also delete any entries in an existent hash map*/
int llist_delete_node(struct llist_container *cont, struct llist *node, bool do_free) {
    if(!cont)
        return -1;
    if(!node)
        return -1;
    LOCK(cont);
    if(node->data && node->data_size != 0) {
        // Hash the data
        uint64_t hash = XXH3_64bits(node->data, node->data_size);
        // if there is a hash map entry for this - we need to get rid of it
        if(cont->h_map[hash] && cont->h_map[hash]->hash == hash) {
            // There is no collision on the hash map entry, so we can just free that point in the map
            if(!cont->h_map[hash]->collision) {
                free(cont->h_map[hash]);
                cont->h_map[hash] = NULL;
            } else {
                // Data matches the hash map entry
                if(llist_compare_entries(cont->h_map[hash]->entry, node)) {
                    // Move the first collision list entry to the main hash map
                    cont->h_map[hash]->entry = cont->h_map[hash]->collision->entry;
                    // If there is a second collision free the first collision entry and make the second
                    // entry the first collision entry
                    if(cont->h_map[hash]->collision->next) {
                        struct llist_collision *temp = cont->h_map[hash]->collision->next;
                        free(cont->h_map[hash]->collision);
                        cont->h_map[hash]->collision = temp;
                    } else {
                        free(cont->h_map[hash]->collision);
                        cont->h_map[hash]->collision = NULL;
                    }
                } else {
                    struct llist_collision *current, *previous = NULL;
                    current = cont->h_map[hash]->collision;
                    do {
                        if(llist_compare_entries(current->entry, node)) {
                            // There is a previous entry in the collision list
                            if(previous) {
                                previous->next = current->next;
                                free(current);
                                break;
                            } else {
                                previous = cont->h_map[hash]->collision;
                                cont->h_map[hash]->collision = current->next;
                                free(previous);
                                break;
                            }
                        }
                        previous = current;
                        current = current->next;
                    } while(current);
                }
            }
            
        }
    }
    if(cont->head == node) {
        if(cont->head->next) {
            cont->head = cont->head->next;
            llist_free_data(do_free, node);
            free(node);
            UNLOCK(cont);
            return 0;
        }
        cont->head = NULL;
        cont->tail = NULL;
        llist_free_data(do_free, node);
        free(node);
        UNLOCK(cont);
        return 0;
    }
    if(cont->tail == node) {
        if(cont->tail->prev) {
            cont->tail = cont->tail->prev;
            llist_free_data(do_free, node);
            free(node);
            UNLOCK(cont);
            return 0;
        }
        cont->head = NULL;
        cont->tail = NULL;
        llist_free_data(do_free, node);
        free(node);
        UNLOCK(cont);
        return 0;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    llist_free_data(do_free, node);
    free(node);
    UNLOCK(cont);
    return 0;
}

/* llist_insert_data_copy inserts data at a specified node by copying it from source, node->data MUST be NULL when calling this */
int llist_insert_data_copy(struct llist_container *cont, struct llist *node, void *data, size_t d_size) {
    if(!cont || !node || node->data)
        return -1;
    node->data = calloc(1, d_size);
    if(!node->data)
        return -1;
    memcpy(node->data, data, d_size);
    return 0;
}

static inline bool llist_compare_entries(struct llist *entry1, struct llist *entry2) {
    return (entry1 && entry2 && (entry1->data_size == entry2->data_size) &&
            !memcmp(entry1->data, entry2->data, entry1->data_size));
}

/* llist_create_hash_map creates a hash map of every entry in the list, utilising collision avoidance where necessary
   This function returns a double pointer containing HASHMAP_SIZE entries, where unused entries in the array are set
   to NULL */

struct llist_map **hash_map_create(struct llist_container *cont) {
    struct llist_map *h_map;
    bool found = false;
    if(!cont) {
        printf("Container not intialized\n");
        return NULL;
    }
    LOCK(cont);
    // We don't create hash maps of rings
    if(cont->is_ring)
        goto end_error;
    if(!cont->head) {
        printf("No linked list head\n");
        goto end_error;
    }
    // reset the list pointer to the head of the list
    cont->list = cont->head;
    int entries = 0;
    while(cont->list) {
        found = false;
        if(cont->list->data_size != 0 && cont->list->data) {
            entries++;
            uint64_t hash = XXH3_64bits(cont->list->data, cont->list->data_size)%HASHMAP_SIZE;
            printf("Hashing linked list entry %d, hash was %llu\n", entries, hash);
            h_map = cont->h_map[hash];
            printf("h_map base at %p and h_map[%llu] at %p\n", cont->h_map, hash, h_map);
            if(h_map) {
                printf("We have a hash collision... deal with this, for now, not adding hash entry for this\n");
                printf("Hash was %llu\n",hash);
                if(llist_compare_entries(h_map->entry, cont->list)) {
                    cont->list = cont->list->next;
                    continue;
                    // Duplicate data do nothing
                } else {
                    if(!h_map->collision) {
                        h_map->collision = calloc(1, sizeof(struct llist_collision));
                        if(!h_map->collision) {
                            printf("Failed allocating for collision\n");
                            return NULL;  // Probably need better error handling here
                        }
                        h_map->collision->entry = cont->list;
                        h_map->collision->hash = hash;
                        cont->list = cont->list->next;
                        continue;
                    } else {
                        struct llist_collision *col_entry = h_map->collision;
                        // Duplicate data found in the collision entry
                        if(llist_compare_entries(col_entry->entry, cont->list)) {
                            cont->list = cont->list->next;
                            continue;
                        }
                        while(col_entry->next) {
                            if(llist_compare_entries(col_entry->entry, cont->list)) {
                                cont->list = cont->list->next;
                                found = true;
                                break;
                            }
                            col_entry = col_entry->next;
                        }
                        if(found) {
                            cont->list = cont->list->next;
                            continue;
                        }
                        col_entry->next = calloc(1, sizeof(struct llist_collision));
                        if(!col_entry->next) {
                            printf("Failed to allocate for next collision entry\n");
                            goto end_error;
                        }
                        col_entry = col_entry->next;
                        col_entry->entry = cont->list;
                        cont->list = cont->list->next;
                        continue;
                    }
                }
            } else {
                cont->h_map[hash] = calloc(1, sizeof(struct llist_map));
                if(!cont->h_map[hash])
                    goto end_error;
                cont->h_map[hash]->entry = cont->list;
                cont->h_map[hash]->hash = hash;
                cont->list = cont->list->next;
                continue;
            }
        }
        cont->list = cont->list->next;
    }
    UNLOCK(cont);
    return cont->h_map;
end_error:
    UNLOCK(cont);
    return NULL;
}


    
    
