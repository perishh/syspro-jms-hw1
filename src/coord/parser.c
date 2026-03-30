#include "parser.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "command.h"
#include "jobs.h"
#include "utils.h"

void print_usage() {
  fprintf(stderr, "Usage: jms_coord -l <path> -n <jobs_pool>\n");
}

int parse_arguments(int argc, char **argv, char **path, int *jobs_pool) {
  // getopt(3)
  int opt;
  while ((opt = getopt(argc, argv, "l:n:")) != -1) {
    switch (opt) {
    case 'l':
      *path = optarg;
      break;
    case 'n':
      *jobs_pool = atoi(optarg);
      break;
    default:
      // Empty or unknown argument
      print_usage();
      return -1;
    }
  }

  if (*path == NULL || *jobs_pool <= 0) {
    // Ensure arguments are valid
    print_usage();
    return -1;
  }

  return 0;
}

int parse_commands(int in, Command** cmd_ptr, int* buffer_zize) {
  ssize_t nread = read(in, (*cmd_ptr), sizeof(Command));
  if (nread < 0) {
    return -1;
  }

  if ((*cmd_ptr)->action == UNKNOWN) {
    printf("Unknown command\n");
    return -1;
  }
  
  if ((*cmd_ptr)->len == 0) {
    // No arguments given
    if (((*cmd_ptr)->action & ZERO_ARG_ACTIONS) == 0) {
      printf("Invalid command\n");
      return -1;
    }
    return 0;
  }

  
  // Check current buffer size and expand if needed
  ssize_t required_space = sizeof(Command) + (*cmd_ptr)->len + 2; // \0 + EOF
  if ((*buffer_zize) < required_space) {
    // TODO: CHECK STATIC MEMBERS FOR INVALID DATA WHEN FORKING
    Command *temp = realloc((*cmd_ptr), required_space);
    if (temp == NULL) {
      perror("relloc fail");
      return -1;
    }
    *buffer_zize = required_space;
    *cmd_ptr = temp;
  }
  
  // Read arguments
  nread = read_blocking(in, (*cmd_ptr)->args, (*cmd_ptr)->len + 2); // \0 + EOF
  if (nread < 0) {
    return -1;
  }
  
  // Ensure null termination
  if ((*cmd_ptr)->args[(*cmd_ptr)->len] != '\0') {
    printf("Malformed arguments\n");
    return -1;
  }

  return 0;
}