#! /bin/bash

WORKDIR=""
COMMAND=""

while getopts "l:c:" opt; do
  case $opt in
    l) WORKDIR="$OPTARG" ;;
    c) COMMAND="$OPTARG" ;;
    *) echo "Unknown argument"; exit 1;;
  esac
done

if [[ -z "$WORKDIR" || -z "$COMMAND" ]]; then
  echo "Invalid arguments"
  exit 1
fi

if [[ ! -d "$WORKDIR" ]]; then
  echo "Directory not found"
  exit 1
fi

if [[ "$COMMAND" != "list" && "$COMMAND" != "size" && "$COMMAND" != "purge" ]]; then
  echo "Invalid command"
  exit 1
fi