#include <stdlib.h>
#include <string.h>

#include "file_list.h"

void file_list_init(file_list_t *list) {
    list->head = NULL;
    list->size = 0;
}

int file_list_add(file_list_t *list, const char *file_name) {
    if (list->head == NULL) {
        list->head = malloc(sizeof(node_t));
        if (list->head == NULL) {
            return 1;
        }
        strncpy(list->head->name, file_name, MAX_NAME_LEN);
        list->head->next = NULL;
        list->size = 1;
        return 0;
    }

    node_t *current = list->head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = malloc(sizeof(node_t));
    if (current->next == NULL) {
        return 1;
    }
    strncpy(current->next->name, file_name, MAX_NAME_LEN);
    current->next->next = NULL;
    list->size++;
    return 0;
}

int file_list_contains(const file_list_t *list, const char *file_name) {
    node_t *current = list->head;
    while (current != NULL) {
        if (strcmp(current->name, file_name) == 0) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

int file_list_is_subset(const file_list_t *l1, const file_list_t *l2) {
    // This approach is not particularly efficient
    node_t *current = l1->head;
    while (current != NULL) {
        if (!file_list_contains(l2, current->name)) {
            return 0;
        }
        current = current->next;
    }
    return 1;
}

void file_list_clear(file_list_t *list) {
    node_t *current = list->head;
    while (current != NULL) {
        node_t *to_free = current;
        current = current->next;
        free(to_free);
    }
    list->head = NULL;
    list->size = 0;
}
