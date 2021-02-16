#!/bin/bash

TMPDIR=tmpsai

rm -rf "$TMPDIR"

set -e
git clone ../ "$TMPDIR" 2>/dev/null

OBJECTS=""

TAGS=`git tag | tail -n 200`   # TODO remove tail

echo all tags: $TAGS

for tag in $TAGS
do 
    COMMIT=`git rev-list -n 1 "$tag"`; 

    cd "$TMPDIR"

    set +e
    git cat-file -e $COMMIT:experimental/saiextensions.h 2>/dev/null # TODO saiversion.h
    EXIT=$?
    set -e

    if [ $EXIT == 0 ];
    then

        echo processing tag $tag \($COMMIT\)

        git co $COMMIT 2>/dev/null

        cat inc/*.h experimental/*.h | perl -ne 'print "$1\n" if /^\s+(SAI_\w+)/' | grep -vP "_(START|END|RANGE_BASE)$" | head -n 5000 | \
            perl ../ver.pl "$tag" "$COMMIT" > ../ver_meta_$COMMIT.c

        gcc -ansi -Wall -O2 -c ../ver_meta_$COMMIT.c -I inc -I experimental -I .. -o ../ver_meta_$COMMIT.o

        OBJECTS+=" ver_meta_$COMMIT.o"

        #echo "no version/extensions"
    fi

    cd ..

done

echo "obj: $OBJECTS"

perl vermeta.pl $OBJECTS > saimetaver.c

gcc -ansi -Wall -O2 saimetaver.c -o saimetaver -I . $OBJECTS



#rm -f ver_meta*.[co]

rm -rf "$TMPDIR"
