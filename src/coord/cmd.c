#include "cmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "command.h"
#include "utils.h"

static Command *buffer;
unsigned long buffer_size;

int cmd_init() {
  buffer = malloc(sizeof(Command));
  if (buffer == NULL) {
    return -1;
  }
  buffer_size = sizeof(Command);

  return 0;
}

Command *cmd_read(int fd) {
  ssize_t nread = read(fd, buffer, sizeof(Command));
  if (nread < 0) {
    return NULL;
  }

  if (buffer->action == UNKNOWN) {
    return NULL;
  }

  if (buffer->len == 0) {
    // No arguments given
    if (buffer->action != STATUS_ALL &&
        (buffer->action & ZERO_ARG_ACTIONS) == 0) {
      return NULL;
    }
    return buffer;
  }

  // Check current buffer size and expand if needed
  unsigned long required_space = sizeof(Command) + buffer->len + 1; // \0
  if (buffer_size < required_space) {
    // TODO: CHECK STATIC MEMBERS FOR INVALID DATA WHEN FORKING
    Command *temp = realloc(buffer, required_space);
    if (temp == NULL) {
      return NULL;
    }
    buffer_size = required_space;
    buffer = temp;
  }

  nread = read_blocking(fd, buffer->args, buffer->len + 1); // \0
  if (nread <= 0) {
    return NULL;
  }

  // Ensure null termination
  if (buffer->args[buffer->len] != '\0') {
    printf("Malformed arguments\n");
    return NULL;
  }

  return buffer;
}

void cmd_free() { free(buffer); }