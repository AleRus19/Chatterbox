#!/bin/bash

pathfile=$1
time=$2


pa=$(locate --e $pathfile)


if  [ $1 == "--help" ]; then
  echo usa: $(basename $0) file time
  exit 1
fi

if  [ $2 == "--help" ]; then
  echo usa: $(basename $0) file time
  exit 1
fi

#se il numero degli argomenti $# è less then 2 esce
if [ $# -lt 2 ]; then 
  echo usa: $(basename $0) file time
  exit 1
fi

if ! [ -f $1 ]; then
echo "Error: File $1 not exists!"
exit 1
fi


list=$(grep DirName $pathfile | cut -d'=' -s -f2 )

IFS=' ' read -r -a array <<< $list

for el in ${array[@]}
do
  bname=basename $el
  if  [ -d $bname ]; then
     DirName=$el
 fi
done 
  
if [ $time -eq 0 ]; then
  for file in "$DirName"/*
  do
    echo "$file" 
  done
  exit 1
fi


if [ $time -gt 0 ]; then
  file=$DirName/*
  find $file -mmin +$time -print -exec tar --append --file=log.tar.gz {} + -exec rm -fr {} +
fi



