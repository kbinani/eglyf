#!/bin/bash
set -ue

# Process arguments
eglyf="$(realpath "$1")"
source="$2"

function verify {
  rm -rf ./gpos_output
  mkdir -p ./gpos_output

  find ./gpos/ -name '*\.xml' | while read xml; do
    filename=$(basename "$xml")
    name="${filename%.*}"
    if [ ! -f "./gpos/${name}.xml" ]; then
      continue
    fi

    echo "Testing $name..."
    $eglyf --input="$source" --output=tmp.ttf --only-lookup-with-name="$name"
    ttx -q -o tmp.ttx tmp.ttf

    # Normalize XML files
    echo "Normalizing XML files..."
    expected="./gpos_output/${name}_expected.xml"
    actual="./gpos_output/${name}_actual.xml"

    # Process expected XML: Apply normalization XSL and format in one pipeline
    xmlstarlet tr normalize_gpos.xsl "$xml" 2>/dev/null | xmlstarlet fo -s 2 > "$expected" 2>/dev/null || true

    # Process actual XML: Extract GPOS Lookup, normalize, and format in one pipeline
    (
      echo '<?xml version="1.0" encoding="UTF-8"?>'
      echo '<LookupList>'
      xmlstarlet sel -t -c "//GPOS/LookupList/Lookup" tmp.ttx 2>/dev/null || true
      echo '</LookupList>'
    ) | xmlstarlet tr normalize_gpos.xsl 2>/dev/null | xmlstarlet fo -s 2 > "$actual" 2>/dev/null || true

    # Compare results
    echo "Comparing results..."
    if diff -u "$expected" "$actual" > "./gpos_output/${name}_diff.txt"; then
      echo "✅ Verification successful: $name"
    else
      echo "❌ Verification failed: $name"
      echo "Differences:"
      cat "./gpos_output/${name}_diff.txt" | head -30
      return 1
    fi
  done
}

(
  cd "$(dirname "$0")"
  verify
  rm -f tmp.ttx tmp.ttf
)
