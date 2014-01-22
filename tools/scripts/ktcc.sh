#!/bin/bash

if [ $# -ne 0 ]; then
    DEFAULT_ARGS="--keepunused -Wno-attributes"
fi

for i in $* 
do
    if expr match $i ".*\.c">/dev/null; then
	DEFAULT_ARGS="$DEFAULT_ARGS --typesfile-out=${i/.c/.ktt}"
    fi
done

CIL_BIN_DIR/bin/cilly $DEFAULT_ARGS $*
