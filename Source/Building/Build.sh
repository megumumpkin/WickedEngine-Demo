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
	echo "Wicked Engine build directory not found."
	echo "You might want to run this script again!"
fi

cmake --build Build -j$(nproc)

rm -f Build/startup.lua
cp Source/Scripts/Initialisation/startup.lua Build/startup.lua
rm -r -f Build/Assets
cp -r Assets/ Build/Assets
rm -f Build/Assets/Scripts