#!/bin/bash

TEMPFILE=`mktemp`
SQLSTATUS=`sqlite3 -bail -echo -init $1 $TEMPFILE .quit 3>&1 1>&2- 2>&3- | grep ^Error`

rm $TEMPFILE

if [ "x$SQLSTATUS" == "x" ] ; then
	exit 0
else
	echo "SQL Parsing Error: $1"
	echo $SQLSTATUS
	exit 1
fi
