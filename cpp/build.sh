#!/bin/sh

./grammar/make.sh

rm -rf obj
mkdir -p bin
mkdir -p obj

buildType=$1

cxxflags="-g -Wall -std=c++14 -Iinclude"

if [ "$buildType" == "release" ]
then
    cxxflags="-O3 -std=c++14 -Iinclude"
fi

for f in source/*.cpp
do
    g++ $cxxflags -c $f -o "${f%.*}.o"
    mv "${f%.*}.o" obj
done

g++ -o bin/sky.exe "obj/*.o"
