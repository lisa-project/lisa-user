#!/bin/bash

show_help() {
cat << EOT
Usage: $0 [options]
-h, --help		This help line
-o <path>		Output patch to <path>
-v <version>		Generate patch against version <version>
EOT
}

KVER="linux-2.6"

temp=`getopt -o 'o:v:h' --long help -n "$0" -- "$@"`

if [ $? != 0 ]; then
    exit 3
fi

eval set -- "$temp"

patch_path="$KVER-lisa.patch"

while true; do
	case $1 in
	-h|--help)
		show_help
		exit 1
		shift
		;;
	-o)
		patch_path="$2"
		shift 2
		;;
	-v)
		latest_kver="$2"
		shift 2
		;;
	--)
		shift
		break
		;;
	*)
		echo "Internal error!"
		exit 3
		;;
	esac
done

cd $KVER
old_branch="`git-branch --no-color | grep '^\*' | sed 's/^..//'`"
echo "~~~ Current branch on linux-2.6 tree is '$old_branch'"

if [ -z "$latest_kver" ]; then
	latest_kver="`git-branch --no-color -r | grep "origin/lisa-v" | sed 's|^..origin/lisa-v||' | sort -rg | head -n 1`"
	echo "~~~ Latest kernel version for lisa is '$latest_kver'"
else
	check_kver="`git-branch --no-color -r | grep "origin/lisa-v" | sed 's|^..origin/lisa-v||' | grep "$latest_kver"`"
	if [ -z "$check_kver" ]; then
		echo "~~~ Bad externally configured version '$latest_kver'"
		exit 1
	fi
	echo "~~~ Using externally configured version '$latest_kver'"
fi

if [ -z "`git-branch --no-color | grep "^..lisa-v$latest_kver"`" ]; then
	echo "~~~ Automatically creating local branch lisa-v$latest_kver"
	git-branch lisa-v$latest_kver origin/lisa-v$latest_kver
fi
tmp_branch="tmp-`uuidgen`"
echo "~~~ Creating temporary branch '$tmp_branch'"
git-checkout -b $tmp_branch v$latest_kver || exit 1
echo "~~~ Merging lisa branch"
git-merge --squash lisa-v$latest_kver || exit 1
echo "~~~ Disabling debugging"
tmp=`mktemp`
sed 's/^.*CFGFLAGS.*DDEBUG.*$/CFGFLAGS += -g/' < net/switch/Makefile > $tmp
cat $tmp > net/switch/Makefile
rm -f $tmp
echo "~~~ Committing changes"
git-commit -a -s -F - << EOF
Lisa for linux $latest_kver

Applied lisa patch on v$latest_kver
EOF
[ $? ] || exit 1
echo "~~~ Writing patch to $patch_path"
git-show HEAD > $patch_path
echo "~~~ Getting back to '$old_branch'"
git-checkout $old_branch
echo "~~~ Deleting temporary branch"
git-branch -D $tmp_branch
