#! /bin/bash

# $Id$
# $URL$


# -kr    Use Kernighan & Ritchie coding style.
# -l120  Set maximum line length for non-comment lines to 150.
# -npro  Do not read ‘.indent.pro’ files.

rm *.c~ *.h~ 2>/dev/null  # trash "no such file or directory" warning messages 
indent -kr -l120 -npro *.c *.h

for i in *.c *.h; do
  if !(diff -q $i $i~); then
    rm $i~
  else
    mv $i~ $i
  fi
done
