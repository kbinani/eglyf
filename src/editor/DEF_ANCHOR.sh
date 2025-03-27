set -ue

#DEF_ANCHOR "a1" ON 469 GLYPH QB1 COMPONENT 1 AT  POS DX 105 DY 1860 END_POS END_ANCHOR
#DEF_ANCHOR "bottom" ON None GLYPH r0v1 COMPONENT 1 AT  POS DY -310 END_POS END_ANCHOR

out="$(dirname "$0")/DEF_ANCHOR.hpp"

function process {
  name=$2
  glyph=$6
  dx="nullopt"
  dy="nullopt"
  if [ ${11} = "DX" ]; then
    dx=${12}
  elif [ ${11} = "DY" ]; then
    dy=${12}
  else
    echo "error1: 11=${11}; $@"
    exit 1
  fi
  if [ ${13} = "DY" ]; then
    dy=${14}
  elif [ ${13} = "END_POS" ]; then
    true
  else
    echo "error2: 13=${13}; $@"
    exit 2
  fi
  echo "    {$name, \"$glyph\", $dx, $dy},"
}

(
cat << EOS
struct AnchorArgs {
  std::string name;
  std::string glyph;
  std::optional<int16_t> dx;
  std::optional<int16_t> dy;
};
static AnchorArgs const sAnchorArgs[] = {
EOS
) > "$out"

cat EgyptianText_200.vtp | grep DEF_ANCHOR | while read line; do
  process $line
done >> "$out"

(
cat << EOS
};
for (size_t i = 0; i < sizeof(sAnchorArgs) / sizeof(AnchorArgs); i++) {
  auto const &args = sAnchorArgs[i];
  defineAnchor(args.name, args.glyph, args.dx, args.dy);
}
EOS
) >> "$out";
