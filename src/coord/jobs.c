#include "jobs.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
    }else{
        in_delim = 0;
    }
  } while (current != '\0');
  return count;
}

// Arguments given by coord are null terminated
int jobs_submit(char *cmd_args) {
  // Delimiter is Space or \n (or NULL)
  int argc = count_words(cmd_args);

  // TODO: Consider malloc
  char* argv[argc + 1]; // Account for terminating NULL

  // strtok(3)
  char *program = strtok(cmd_args, " \n");
  if (program == NULL) {
    return -1;
  }

  int i = 0;
  argv[i++] = program;
  while((argv[i++] = strtok(NULL, " \n")) != NULL) { }

  // fork(2)
  int pid = fork();
  if(pid == 0) {
    // Child process
    // exec(3)
    int ret = execvp(program, argv);
    if(ret < 0) {
        // Failed to exec
        perror("execv");
        return ret;
    }
  }
  if(pid < 0) {
    // Failed to fork
    perror("fork");
    return -1;
  }

  return 0;
}