#!/usr/bin/sh

if [ ! -d '../../Build' ]; then
	mkdir '../../Build'
fi

if [ ! -d '../../Data/Shader' ]; then
	mkdir -p '../../Data/Shader'
fi

if [ ! -d '../../Library/WickedEngine/build' ]; then
	mkdir -p '../../Library/WickedEngine/build'
	cd '../../Library/WickedEngine/build'
	cmake .. -DCMAKE_BUILD_TYPE=Release
	make -j$(nproc)
	cd '../../../'
fi

if [ -d 'Library/WickedEngine/build' ]; then
	cmake -B Build -DWickedEngine_DIR=Library/WickedEngine/build/cmake .
else
	echo "Wicked Engine build directory not found."
	echo "You might want to run this script again!"
fi

cmake --build Build -j$(nproc)

cp -f Source/Scripts/Root/startup.lua Build/startup.lua
cp -f Source/Scripts/Root/editor.lua Build/editor.lua