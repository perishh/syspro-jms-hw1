#ifndef LIST_H
#define LIST_H

typedef struct Node {
  void *data;
  struct Node *next;
} Node;

typedef struct {
  Node *front;
  int size;
} LinkedList;

/**
 * @brief Initialized linked list values
 * @param l pointer to the linked list
 */
void ll_init(LinkedList *l);

/**
 * @brief Pushes item to start of linked list
 * @param l pointer to the linked list
 * @param data pointer to store to linked list
 * @return 0 on success, -1 otherwise
 */
int ll_push(LinkedList *l, void *data);

/**
 * @brief Deallocates memory used for linked list
 * @note Doesn't deallocate the LinkedList itself
 * @param l pointer to the linked list
 */
void ll_free(LinkedList *l);

/**
 * @brief Removes node from linked list
 * @param l pointer to the linked list
 * @param node pointer to node to remove
 * @return 0 on success, -1 otherwise
 */
int ll_remove(LinkedList *l, Node *node);

#define FOR_EACH(list, node)                                                   \
  for (Node *node = list.front; node != NULL; node = node->next)

#endif