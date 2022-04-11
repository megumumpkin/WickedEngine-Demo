#!/usr/bin/sh

if [ ! -d '../../Build' ]; then
	mkdir '../../Build'
fi

cd ../../

cmake -B Build -DWickedEngine_DIR=Library/WickedEngine/build/cmake .
cmake --build Build -j8
