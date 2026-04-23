#include "list.h"

#include <stdlib.h>

void ll_init(LinkedList *l) {
  l->front = NULL;
  l->size = 0;
}

int ll_push(LinkedList *l, void *data) {
  Node *node = malloc(sizeof(Node));
  if (node == NULL) {
    return -1;
  }

  node->data = data;
  node->next = l->front;

  l->front = node;
  l->size++;

  return 0;
}

void ll_free(LinkedList *l) {
  Node *n = l->front;
  Node *temp;
  while (n != NULL) {
    temp = n;
    n = n->next;

    free(temp);
  }
  l->size = 0;
}

int ll_remove(LinkedList *l, Node *node) {
  // Node in front
  if (l->front == node) {
    l->front = node->next;
    free(node);
    l->size--;
    return 0;
  }

  // Find previous node
  Node *current = l->front;
  while (current->next != NULL && current->next != node) {
    current = current->next;
  }

  // Node not found
  if (current->next == NULL) {
    return -1;
  }

  current->next = node->next;
  free(node);
  l->size--;
  return 0;
}