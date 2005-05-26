real_path="`readlink -f -n "$PWD"`"
export LD_LIBRARY_PATH="$real_path"
