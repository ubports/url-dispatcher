#!/bin/bash

if [ "x${URL_DISPATCHER_TEST_CLICK_DIR}" == "x" ]; then
	exit 1
fi

cat "${URL_DISPATCHER_TEST_CLICK_DIR}/.click/info/${2}.manifest"
