#!/usr/bin/sh

if [ ! -d '../../Build' ]; then
	mkdir '../../Build'
fi

if [ ! -d '../../Data/Shader' ]; then
	cd '../../Data/'
	mkdir 'Shader'
fi

if [ ! -d '../../Library/WickedEngine/build' ]; then
	cd '../../Library/WickedEngine/'
	mkdir 'build'
	cd '../../Library/WickedEngine/build'
	cmake .. -DCMAKE_BUILD_TYPE=Release
	make -j$(nproc)
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
rm -f Build/editor.lua
cp Source/Scripts/Root/startup.lua Build/startup.lua
cp Source/Scripts/Root/editor.lua Build/editor.lua
rm -r -f Build/Data
cp -r Data/ Build/Data