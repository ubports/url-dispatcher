#!/bin/sh

sed '/^#/d
	 s/^[[].*] *//
	 /^[ 	]*$/d' \
      "`dirname ${0}`/POTFILES.in" | sed '$!s/$/ \\/' > POTFILES
