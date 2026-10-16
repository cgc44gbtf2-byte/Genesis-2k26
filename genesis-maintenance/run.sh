#!/bin/sh
set -eu

port=${PORT:-4000}
case "$port" in
    ''|*[!0-9]*)
        echo "PORT must be a numeric TCP port." >&2
        exit 2
        ;;
esac
if [ "$port" -le 1024 ] || [ "$port" -gt 65535 ]; then
    echo "PORT must be between 1025 and 65535." >&2
    exit 2
fi

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p "$script_dir/player" "$script_dir/backup" "$script_dir/log"
cd "$script_dir/area"
exec ../src/merc "$port"
