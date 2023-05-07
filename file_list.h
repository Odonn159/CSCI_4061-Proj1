#ifndef _FILE_LIST_H
#define _FILE_LIST_H

#define MAX_NAME_LEN 32

//  Definition of each node in the linked list
typedef struct node {
    char name[MAX_NAME_LEN];
    struct node *next;
} node_t;

// Linked list definition
typedef struct {
    node_t *head;
    int size;
} file_list_t;

// Initialize a new, empty list
void file_list_init(file_list_t *list);

// Add a new file name to the tail of the linked list
int file_list_add(file_list_t *list, const char *file_name);

// Remove all entries from the list and free any memory associated with them
void file_list_clear(file_list_t *list);

// Determine if a file name is contained in a list
// Returns 1 if the name is present as an element in the list, 0 otherwise
int file_list_contains(const file_list_t *list, const char *file_name);

// Determine if the elements of l1 are a subset of the elements of l2
// That is, all elements of l1 are contained in l2
// Returns 1 if l1 is a subset of l2, 0 otherwise
int file_list_is_subset(const file_list_t *l1, const file_list_t *l2);

#endif