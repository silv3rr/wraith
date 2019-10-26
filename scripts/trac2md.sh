#!/bin/sh
script="$(basename "$0")"
if [ -n "$1" ]; then
  if echo "$1" | grep -q "^https\?://"; then
    page="$( echo "$1" | awk -F'/' '{ print $(NF)".txt" }')"
    curl -s "${1}?format=txt" -o "${page}"
  else
    page="$1" 
  fi
  if [ -s "$page" ]; then
    output="$( echo "$page" | sed 's/.txt$/.md/' )"
    if [ ! -s "$output" ]; then
      sed -E \
        -e 's/^ *= (.*) =/# \1/' \
        -e 's/^ *== (.*) ==/## \1/' \
        -e 's/^ *=== (.*) ===/### \1/' \
        -e 's/^ *==== (.*) ====/#### \1/' \
        -e 's/^   \* ([^\*])/  * \1/' \
        -e 's/^ \* ([^\*])/* \1/' \
        -e 's/(\{\{\{|\}\}\})/```/g' \
        -e "s/'''/**/g" \
        -e "s/''/_/g" \
        -e 's/^ 1./1./g' \
        -e 's/\[\[PageOutline\(.*\]\]//' \
        -e 's/\[wiki:(\w+)\s((\w+).*)\]/[\2](\1)/g' \
        -e 's/\[wiki:(.*)\s(\w+)\]/[\2]\(\1)/g' \
        -e 's/\[wiki:(.*)\]/[[\1]]/g' \
        -e 's/\[(https?:\/\/.*[^\s])\s(([0-9\w]+).*)\]/[\2](\1)/g' \
        -e 's/\[(https?:\/\/.*[^\s])\s(.*)\]/[\2](\1)/g' \
        -e 's/\[(https?:\/\/.*[^\s\][0-9\w\/])\]/[[\1]]/g' \
        "$page" > "$output"
    else
      echo "ERROR: output page already exists"
      exit 1 
    fi
  else
    echo "ERROR: input page not found" 
    exit 1 
  fi
else
  echo "Convert trac wiki pages to markdown"
  echo "USAGE: ./$script <Page.txt>" 
  echo "       ./$script \"http://example.com/wiki/<Page>\""
fi
