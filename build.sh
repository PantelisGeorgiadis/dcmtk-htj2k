#!/bin/bash
set -xe

if [ "$1" == "Debug" ]; then
	BUILD_TYPE=Debug
else
	BUILD_TYPE=Release
fi

BUILD_DIR="$(pwd)/build"
BUILD_DIR_LIB="$(pwd)/build/$BUILD_TYPE"
OTS_DEV_SPACE="$(pwd)/ots"

mkdir -p $OTS_DEV_SPACE
cd $OTS_DEV_SPACE
[[ -d dcmtk ]] || git clone https://github.com/DCMTK/dcmtk.git
cd dcmtk
git fetch
git checkout -f DCMTK-3.6.9
mkdir -p $BUILD_TYPE
cd $BUILD_TYPE
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
	-DCMAKE_CXX_STANDARD=11 \
	-DDCMTK_MODULES='ofstd;oficonv;oflog;dcmdata;dcmimgle;dcmimage;dcmjpeg;dcmjpls' \
	-DDCMTK_ENABLE_STL=OFF \
	-DDCMTK_WIDE_CHAR_FILE_IO_FUNCTIONS=OFF \
	-DDCMTK_DEFAULT_DICT='builtin' \
	-DDCMTK_WITH_TIFF=OFF \
	-DDCMTK_WITH_PNG=OFF \
	-DDCMTK_WITH_OPENSSL=OFF \
	-DDCMTK_WITH_XML=OFF \
	-DDCMTK_WITH_ZLIB=OFF \
	-DDCMTK_WITH_SNDFILE=OFF \
	-DDCMTK_WITH_ICONV=ON \
	-DDCMTK_WITH_WRAP=OFF \
	-DBUILD_APPS=OFF \
	-DCMAKE_INSTALL_PREFIX=$OTS_DEV_SPACE/dcmtk/$BUILD_TYPE \
	-DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
	-DCMAKE_MSVC_RUNTIME_LIBRARY='MultiThreaded$<$<CONFIG:Debug>:Debug>'
cmake --build . --parallel $(nproc) --config $BUILD_TYPE
cmake --install . --config $BUILD_TYPE

cd $OTS_DEV_SPACE
[[ -d openjph ]] || git clone https://github.com/aous72/openjph.git
cd openjph
git fetch
git checkout -f 0.26.0
mkdir -p $BUILD_TYPE
cd $BUILD_TYPE
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
	-DCMAKE_CXX_STANDARD=11 \
	-DBUILD_SHARED_LIBS=OFF \
	-DOJPH_BUILD_EXECUTABLES=OFF \
	-DCMAKE_INSTALL_PREFIX=$OTS_DEV_SPACE/openjph/$BUILD_TYPE \
	-DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
	-DCMAKE_MSVC_RUNTIME_LIBRARY='MultiThreaded$<$<CONFIG:Debug>:Debug>'
cmake --build . --parallel $(nproc) --config $BUILD_TYPE
cmake --install . --config $BUILD_TYPE

mkdir -p $BUILD_DIR
mkdir -p $BUILD_DIR_LIB
cd $BUILD_DIR
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
	-DCMAKE_CXX_STANDARD=11 \
	-DBUILD_SHARED_LIBS=OFF \
	-DBUILD_TESTING=ON \
	-DDCMTK_ROOT=$OTS_DEV_SPACE/dcmtk/$BUILD_TYPE \
	-DOPENJPH_DIR=$OTS_DEV_SPACE/openjph/$BUILD_TYPE/lib/cmake/openjph \
	-DDCMTKHTJ2K_ROOT=$OTS_DEV_SPACE/dcmtkhtj2k/$BUILD_TYPE \
	-DCMAKE_INSTALL_PREFIX=$BUILD_DIR_LIB \
	-DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
	-DCMAKE_MSVC_RUNTIME_LIBRARY='MultiThreaded$<$<CONFIG:Debug>:Debug>'
cmake --build . --parallel $(nproc) --config $BUILD_TYPE
cmake --install . --config $BUILD_TYPE
