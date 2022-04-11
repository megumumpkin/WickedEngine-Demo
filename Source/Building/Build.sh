#!/usr/bin/sh

if [ ! -d '../../Build' ]; then
	mkdir '../../Build'
fi

if [ ! -d '../../Library/WickedEngine/build' ]; then
	cd '../../Library/WickedEngine/'
	mkdir 'build'
	cd '../../Library/WickedEngine/build'
	cmake .. -DCMAKE_BUILD_TYPE=Release
	make
fi

if [ -d '../../Library/WickedEngine/build' ]; then
	cd ../../
	cmake -B Build -DWickedEngine_DIR=Library/WickedEngine/build/cmake .
else
	echo "Wicked Engine build directory not found!"
fi

cmake --build Build -j$(nproc)