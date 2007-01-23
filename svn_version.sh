#!/bin/sh

# $Id$
# $URL$


OLD_VERSION=`cat svn_version.h 2>/dev/null`

if [ -d .svn ]; then
    NEW_VERSION="#define SVN_VERSION \"`svnversion -n`\""
fi

if [ "$NEW_VERSION" != "$OLD_VERSION" ]; then
    echo $NEW_VERSION >svn_version.h
fi
