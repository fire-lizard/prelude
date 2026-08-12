#!/bin/sh
# The game reads and writes every file relative to the working directory, and
# Finder launches an .app in "/". Globals constructed before main() already open
# files, so the move has to happen before the game binary starts, not inside it.
DIR=$(cd "$(dirname "$0")" && pwd)
cd "$DIR/../Resources" || exit 1
exec "$DIR/Prelude.bin" "$@"
