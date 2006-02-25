#! /bin/bash

# -kr    Use Kernighan & Ritchie coding style.
# -l120  Set maximum line length for non-comment lines to 150.
# -pmt   Preserve access and modification times on output files.

indent -kr -l120 *.c *.h

for i in *.c *.h; do
  if !(diff -q $i $i~); then
    rm $i~
  else
    mv $i~ $i
  fi
done
