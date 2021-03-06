#!/bin/bash

#
# file   runScriptET.sh
# author Tri Kurniawan Wijaya
# date   Tue 26 Apr 2011 01:00:03 PM CEST 
#
# run benchmark example for a topology
#
# require 2 params
# 1st param: target directory, example: ModuleCore/ModuleCore-line
# 2nd param: result directory, example: StatsCore/StatsCore-line
#

mainDir=$2
resultDir=$3

go=1
if [ -a $resultDir ]; then
  go=0
  read -p "$resultDir is already exist. Do you want to delete it anyway? [y]es / [c]ancel execution: " inp
  if [ "$inp" = "y" ]; then
    rm -rf $resultDir
    go=1
  fi 
fi

if [ $go -eq 1 ]; then

mkdir $resultDir
#DLVHEX="dlvhex --mlp --forget --num=100 --verbose=128"
DLVHEX=$1
for dir in $mainDir/*; do 
	if [ -d $dir ]; then
		#execute 10 instances here
		shortDir=${dir#$mainDir/}
		mkdir $resultDir/$shortDir
		echo "process $shortDir"
		for i in {1..10}
		do
			(ulimit -v 1048576 ; /usr/bin/time --verbose -o $resultDir/$shortDir/time-$shortDir-i$i.log $DLVHEX $mainDir/$shortDir/$shortDir-i$i-*.mlp) 2>$resultDir/$shortDir/stats-$shortDir-i$i.log 1>/dev/null
			echo "$i instances(s) processed"
		done
		echo $curdir
		
	fi
done

fi
