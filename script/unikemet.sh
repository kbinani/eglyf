(

echo "// clang-format off"
cat script/Unikemet.txt \
  | grep -v '^#' \
  | grep kEH_AltSeq \
  | sed 's/U+//g' \
  | cut -f1,3- \
  | awk '{for (i=1; i<=NF; i++) { if (i == 1) { printf "r[0x%s] = {", $i } else { printf "0x%s, ", $i } }; print "};" }' \
  | sed 's/, }/}/g'
echo "// clang-format on"

) > src/res/UnikemetAltSeq.hpp
