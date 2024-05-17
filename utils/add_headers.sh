#!/bin/bash

# Path to the configure script
SCRIPT=$(readlink -f "$0")
# Directory containing configure
SCRIPTPATH=$(dirname "$SCRIPT")

CHEADER="${SCRIPTPATH}/headers/header_cpp"
SHHEADER="${SCRIPTPATH}/headers/header_sh"

# Get comment separators from file extension
# Args:
# - $1 : filename
#
set_comm_del()
{
    (echo "$1" | grep "\\." > /dev/null) || (echo "$1 has no extension"; exit 1)
    EXT=$(echo "$1" | rev | cut -d "." -f 1 | rev)

    case $EXT in
        sh)
            SDL="# "
            ENDL=" #"
        ;;
        c|h)
            SDL="/* #"
            ENDL="# */"
        ;;
        *)
            echo "Extension $EXT is not supported in $1"
            exit 1
        ;;
    esac
}

# Remove MPC header from FILE
#
# Args:
# -$1: file name
#
remove_header()
{
    TMP=$(mktemp)
    grep -Ev "#.{70}#" > "$TMP" < "$1"
    cp "$TMP" "$1"
    rm -f "${TMP}"
}

get_date()
{
    LANG=us date
}

# Get the author list for a file
#
# Args:
#   - $1: file
#
get_authors()
{
    FILE_DIR=$(dirname "$1")
    FILE_NAME=$(basename "$1")
    git -C "$FILE_DIR" log --follow --pretty="%an <%aE>" -- "$FILE_NAME" | sort -u | uniq -i | git -C "$FILE_DIR" check-mailmap --stdin | sort -u
}

wrap()
{
    while read -r l
    do
        if test -n "$l"; then
            printf "%s - %-66s %s~~" "$SDL" "$l" "$ENDL"
        fi
    done <<< "$1"
}

wrap2()
{
    while read -r l
    do
        if test -n "$l"; then
            printf "%s %-68s %s" "$SDL" "$l" "$ENDL"
        fi
    done <<< "$1"
}

for f in "$@"
do
    echo "Processing $f"
    set_comm_del "$f"

    AUTH="$(wrap "$(get_authors "$f")")"
    DATE="$(wrap2 "$(get_date)")"

    SFILE=$(mktemp)

    remove_header "$f"

    #GEN Header
    HEADER=$(mktemp)

    cat "$CHEADER" | sed "s;@AUTHORS@;$AUTH;g" | sed "s/~~/\n/g" | sed "s;@DATE@;$DATE;g" | grep -v ^$ > "$HEADER"

    cp "$f" "$SFILE"
    cat "$HEADER" "$SFILE" > $f

done
