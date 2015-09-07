#!/usr/bin/env bash
if [ ./HELP -nt src/app/help.cpp ] ; then
	echo "updating src/app/help.cpp"
	xxd -i HELP src/app/help.cpp
fi

