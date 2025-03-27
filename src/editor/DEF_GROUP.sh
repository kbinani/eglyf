set -ue

out="$(dirname "$0")/DEF_GROUP.hpp"

function process {
  line="$1"
  echo "$line" \
  | sed 's/DEF_GROUP "\(.*\)"/defineGroup("\1")/g' \
  | sed 's/END_ENUM/)->endEnum()/g' \
  | sed 's/END_GROUP/->endGroup();/g' \
  | sed 's/ENUM/->beginEnum()/g' \
  | sed 's/GROUP/->addGroup(/g' \
  | sed 's/GLYPH/->addGlyph(/g' \
  | tr -d ' ' \
  | sed 's/"->addGroup(/")->addGroup(/g' \
  | sed 's/"->addGlyph(/")->addGlyph(/g'
}

active=0
cat EgyptianText_200.vtp | while read line; do
  if [ $active = 1 ]; then
    process "$line"
    if echo "$line" | grep '^END_GROUP' >/dev/null; then
      active=0
    fi
  else
    if echo "$line" | grep '^DEF_GROUP' >/dev/null; then
      active=1
      process "$line"
    fi
  fi
done > "$out"
