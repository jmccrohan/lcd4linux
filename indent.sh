#! /bin/bash

# -kr    Use Kernighan & Ritchie coding style.
# -l150  Set maximum line length for non-comment lines to 150.
# -pmt   Preserve access and modification times on output files.

indent -kr -l150 -pmt *.c *.h

