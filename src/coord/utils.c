#include "utils.h"

#include <string.h>
#include <sys/poll.h>
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <unistd.h>

ssize_t read_blocking(int _fd, void *_buf, size_t _nbytes) {
  // poll(2)
  struct pollfd pfd;
  pfd.fd = _fd;
  pfd.events = POLLIN;

  int ret = poll(&pfd, 1, -1);
  if (ret <= 0) {
    return ret;
  }

  return read(_fd, _buf, _nbytes);
}

int count_words(const char *args) {
  int in_delim = 1;
  int count = 0;
  int i = 0;
  char current;
  do {
    current = args[i++];
    if (current == ' ' || current == '\n' || current == '\0') {
      if (!in_delim) {
        in_delim = 1;
        count++;
      }
    } else {
      in_delim = 0;
    }
  } while (current != '\0');
  return count;
}

int decode_args(char *raw, char **argv) {
  // strtok(3)
  char *program = strtok(raw, " \n");
  if (program == NULL) {
    return -1;
  }

  int i = 0;
  argv[i++] = program;
  while ((argv[i++] = strtok(NULL, " \n")) != NULL) {
  }

  return 0;
}
