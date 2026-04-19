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

void ll_init(LinkedList *l);
int ll_push(LinkedList *l, const void *data);
void ll_free(LinkedList *l);

#define FOR_EACH(list, node)                                                   \
  for (Node *node = list.front; node != NULL; node = node->next)

#endif