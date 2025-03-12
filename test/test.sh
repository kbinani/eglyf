set -ue

resave="$1"
input="$2"
dir=$(dirname "$(realpath "$input")")
filename=$(basename "$input")
extension="${filename##*.}"
name="${filename%.*}"
output="$dir/$name-out.$extension"

input_ttx="$dir/$name.ttx"
output_ttx="$dir/$name-out.ttx"

ttx -q -o "$input_ttx" "$input" &

"$resave" "$input" "$output" &

wait

if [ ! -f "$output" ]; then
  exit 255
fi

ttx -q -o "$output_ttx" "$output"

tables=$(xmlstarlet sel -t -m "ttFont/*" -n  -v "name()" "$input_ttx" | grep -v DSIG)

function test {
  table="$1"
  tmp=$(mktemp)
  xmlstarlet sel -t -c "ttFont/$table" "$input_ttx" | grep -v numberOfHMetrics > "${table}-expected.xml"
  xmlstarlet sel -t -c "ttFont/$table" "$output_ttx" | grep -v numberOfHMetrics > "${table}-actual.xml"

  if diff -u "${table}-expected.xml" "${table}-actual.xml" > "$tmp"; then
    echo -n
  else
    cat "$tmp" | head -30
    exit 255
  fi
  rm -f "${table}-expected.xml" "${table}-actual.xml" "$tmp"
}

pids=()
for table in $tables; do
  test "$table" &
  pids+=($!)
done

for pid in ${pids[@]}; do
  wait $pid
  if [ $? -ne 0 ]; then
    wait
    exit 3
  fi
done
