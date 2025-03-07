set -ue

resave="$1"
input="$2"
dir=$(dirname "$(realpath "$input")")
filename=$(basename "$input")
extension="${filename##*.}"
name="${filename%.*}"
output="$dir/$name-out.$extension"

"$resave" "$input" "$output"

input_ttx="$dir/$name.ttx"
output_ttx="$dir/$name-out.ttx"

ttx -q -o "$input_ttx" "$input"
ttx -q -o "$output_ttx" "$output"

tables=$(xmlstarlet sel -t -m "ttFont/*" -n  -v "name()" "$input_ttx" | grep -v DSIG)

for table in $tables; do
  echo "table='$table'"
  xmlstarlet sel -t -c "ttFont/$table" "$input_ttx" > "${table}-expected.xml"
  xmlstarlet sel -t -c "ttFont/$table" "$output_ttx" > "${table}-actual.xml"
  diff -u "${table}-expected.xml" "${table}-actual.xml"
  rm -f "${table}-expected.xml" "${table}-actual.xml"
done
