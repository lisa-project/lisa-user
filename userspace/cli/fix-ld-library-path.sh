#!/bin/bash
# Interpreter line above is just for propper vim syntax highlighting.
# You should always run this as ". fix-ld-library-path.sh" to get the
# variables set in the current shell.

real_path="$(readlink -f -n "$PWD")"
switch_path="$(readlink -f -n "$real_path/../switch")"
export LD_LIBRARY_PATH="$real_path:$switch_path"
