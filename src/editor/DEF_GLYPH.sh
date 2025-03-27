set -ue

out="$(dirname "$0")/DEF_GLYPH.hpp"

function DEF_GLYPH {
  name="$2"
  code=""
  if [ "$5" == "UNICODE" ]; then
    code="$6"
    type="$8"
  else
    code="nullopt"
    type="$6"
  fi
  typestr=""
  if [ "$type" == "MARK" ]; then
    typestr="GlyphDefinitionTable::Class::Mark"
  elif [ "$type" == "BASE" ]; then
    typestr="GlyphDefinitionTable::Class::Base"
  else
    exit 1
  fi
  echo "    { $name, $code, $typestr },"
}

(
cat << EOS
struct GlyphArgs {
  std::string name;
  std::optional<uint32_t> unicode;
  GlyphDefinitionTable::Class classDef;
};
static GlyphArgs const sGlyphArgs[] = {
EOS
) > "$out"

cat EgyptianText_200.vtp | grep DEF_GLYPH | while read line; do
  # DEF_GLYPH "controlrect" ID 395 TYPE MARK END_GLYPH
  # DEF_GLYPH "G49" ID 2188 UNICODE 78201 TYPE MARK END_GLYPH
  DEF_GLYPH $line
done >> "$out"
(
cat << EOS
};
for (size_t i = 0; i < sizeof(sGlyphArgs) / sizeof(GlyphArgs); i++) {
  auto const& arg = sGlyphArgs[i];
  if (auto gid = defineGlyph(arg.name, arg.unicode, arg.classDef); !gid) {
    return EGLYF_STATUS_PUSH(gid.status());
  }
}
EOS
) >> "$out"
