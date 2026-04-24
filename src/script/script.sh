#! /bin/bash

print_usage() {
  echo "Usage: $0 -l <path> -c <command> [argument]"
  echo "Available commands: list, size [n], purge"
  exit 1
}

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
      print_usage
      ;;
  esac
done

if [[ -z "$WORKDIR" || -z "$COMMAND" ]]; then
  print_usage
fi

if [[ ! -d "$WORKDIR" ]]; then
  echo "Directory not found"
  exit 1
fi

if [[ "$COMMAND" != "list" && "$COMMAND" != "size" && "$COMMAND" != "purge" ]]; then
  echo "Invalid command"
  print_usage
fi

if [[ "$COMMAND" != "size" && -n "$SIZE" ]]; then
  echo "Invalid argument"
  print_usage
fi

DIRS="$(find "$WORKDIR" -mindepth 1 -maxdepth 1 -type d | grep -E 'outputs_[0-9]*_[0-9]*_[0-9]*_[0-9]*$')"
case "$COMMAND" in
  "list")
    sed -E 's|(.*)/(outputs_[0-9]*_[0-9]*_[0-9]*_[0-9]*)$|Directory: \2|' <<< "$DIRS"
    ;;
  "size")
    count=1
    while IFS= read -r i; do
      du -sh "$i" | sort -h | sed -E 's|(.*)\t.*/(outputs_[0-9]*_[0-9]*_[0-9]*_[0-9]*)$|Directory: \2\tSize: \1|'
      if [[ -n "$SIZE" && $count -ge "$SIZE" ]]; then
        break
      fi
      ((count++))
    done <<< "$DIRS"
    ;;
  "purge")
    echo "$DIRS" | xargs -d '\n' rm -rf
    ;;
  *)
    ;;
esac