#!/usr/bin/env bash
#
# Installer script
#
# We expect that Lindi has already been built successfully.

cp lindi /usr/bin/lindi
if [[ -e /usr/share/lindi ]]
then
	rm -rf /usr/share/lindi
fi
cp -R ../share /usr/share/lindi
which lindi


