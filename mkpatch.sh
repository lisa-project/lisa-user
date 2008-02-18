#!/bin/bash

KVER="linux-2.6"

patch_path=$1
if [ -z "$patch_path" ]; then
	patch_path="$KVER-lisa.patch"
fi

cd $KVER
old_branch="`git-branch --no-color | grep '^\*' | sed 's/^..//'`"
echo "~~~ Current branch on linux-2.6 tree is '$old_branch'"
latest_kver="`git-branch --no-color -r | grep "origin/lisa-v" | sed 's|^..origin/lisa-v||' | sort -rg | head -n 1`"
echo "~~~ Latest kernel version for lisa is '$latest_kver'"
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
