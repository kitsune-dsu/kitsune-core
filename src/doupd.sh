#!/bin/sh

# Using readlink to resolve relative paths (also resolves symbolic links)
# This will send driver a absolute path based on to doupd's cwd instead 
# having driver resolve the path relitive to its cwd.
echo -n `readlink -f $2` > /tmp/$1.upd
kill -USR2 $1
