#! /bin/bash

WORKDIR=""
COMMAND=""
SIZE=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    -l)
      shift
      WORKDIR="$1"
      shift
      ;;
    -c)
      shift
      COMMAND="$1"
      shift

      # Read command argument if exists
      if [[ $# -gt 0 && "$1" != -* ]]; then
        SIZE="$1"
        shift
      fi
      ;;
    *)
      # TODO: Print usage
      echo "Invalid argument"
      exit 1
      ;;
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

if [[ "$COMMAND" != "size" && -n "$SIZE" ]]; then
  echo "Invalid argument"
  exit 1
fi

DIRS="$(find "$WORKDIR" -mindepth 1 -maxdepth 1 -type d)"
case "$COMMAND" in
  "list")
    echo "$DIRS"
    ;;
  "size")
    while IFS= read -r i; do
      du -sh "$i" | sort -h | sed -E 's/(.*)\t(.*)/Directory: \2\tSize: \1/'
    done <<< "$DIRS"
    ;;
  "purge")
    echo "$DIRS" | xargs -d '\n' rm -rf
    ;;
  *)
    ;;
esac