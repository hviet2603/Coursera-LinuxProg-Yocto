#!/bin/sh
filedir=$1
searchstr=$2

if [ -z "$filedir" ] || [ -z "$searchstr" ]; then
    echo "File directory or search string are not specified"
    return 1
fi

if [ ! -d "$filedir" ]; then
    echo "$filedir is not a valid directory"
fi

files=$(ls $filedir)
numfiles=$(ls $filedir | wc -l)
totalmatches=0
for file in $files; do
    filepath="$filedir/$file"
    matches=$(cat $filepath | grep $searchstr | wc -l)
    totalmatches=$((totalmatches+matches)) 
done

echo "The number of files are ${numfiles} and the number of matching lines are ${totalmatches}"