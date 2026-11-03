#!/bin/bash
# Usage: gcc_units.sh <tree> <outdir>  - compiles the translation units the patch touches
tree="$1"; out="$2"; mkdir -p "$out"; rc=0
for f in src/common/strlib.c src/common/sql_async.c src/map/charselect.c src/map/chat.c src/map/clif.c src/map/guild.c src/map/homunculus.c src/map/map.c src/map/party.c src/map/pet.c src/map/vending.c src/api/api.c; do
  gcc -std=gnu11 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-clobbered -Wno-missing-field-initializers \
      -DPACKETVER=20220406 -I"$tree/src" -I"$tree/3rdparty" -I"$tree/3rdparty/mariadb/include" -I"$tree/3rdparty/pcre/include" \
      -c "$tree/$f" -o "$out/$(basename "$f" .c).o" 2> "$out/$(basename "$f" .c).log" || { echo "FAIL $f"; rc=1; }
done
exit $rc
