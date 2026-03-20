@echo off
setlocal EnableDelayedExpansion

REM Run from "x64 Native Tools Command Prompt for VS 2022" so that MSVC and CMake are available.
REM Or ensure MSVC 2022 and CMake are on PATH.

if "%1"=="Debug" (
	set "BUILD_TYPE=Debug"
) else (
	set "BUILD_TYPE=Release"
)

set "BUILD_DIR=%CD%\build"
set "BUILD_DIR_LIB=%CD%\build\%BUILD_TYPE%"
set "OTS_DEV_SPACE=%CD%\ots"

if not exist "%OTS_DEV_SPACE%" mkdir "%OTS_DEV_SPACE%"
cd /d "%OTS_DEV_SPACE%"

if not exist "dcmtk" git clone https://github.com/DCMTK/dcmtk.git
cd dcmtk
git fetch
git checkout -f DCMTK-3.6.9
if not exist "%BUILD_TYPE%" mkdir "%BUILD_TYPE%"
cd %BUILD_TYPE%

cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
	-DCMAKE_CXX_STANDARD=11 ^
	-DDCMTK_MODULES=ofstd;oficonv;oflog;dcmdata;dcmimgle;dcmimage;dcmjpeg;dcmjpls ^
	-DDCMTK_ENABLE_STL=OFF ^
	-DDCMTK_WIDE_CHAR_FILE_IO_FUNCTIONS=OFF ^
	-DDCMTK_DEFAULT_DICT=builtin ^
	-DDCMTK_WITH_TIFF=OFF ^
	-DDCMTK_WITH_PNG=OFF ^
	-DDCMTK_WITH_OPENSSL=OFF ^
	-DDCMTK_WITH_XML=OFF ^
	-DDCMTK_WITH_ZLIB=OFF ^
	-DDCMTK_WITH_SNDFILE=OFF ^
	-DDCMTK_WITH_ICONV=ON ^
	-DDCMTK_WITH_WRAP=OFF ^
	-DBUILD_APPS=OFF ^
	-DCMAKE_INSTALL_PREFIX=%OTS_DEV_SPACE%/dcmtk/%BUILD_TYPE% ^
	-DCMAKE_POLICY_DEFAULT_CMP0091=NEW ^
	-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$^<$^<CONFIG:Debug^>:Debug^>

cmake --build . --parallel %NUMBER_OF_PROCESSORS% --config %BUILD_TYPE%
cmake --install . --config %BUILD_TYPE%

cd /d "%OTS_DEV_SPACE%"
if not exist "openjph" git clone https://github.com/aous72/openjph.git
cd openjph
git fetch
git checkout -f 0.26.0
if not exist "%BUILD_TYPE%" mkdir "%BUILD_TYPE%"
cd %BUILD_TYPE%

cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
	-DCMAKE_CXX_STANDARD=11 ^
	-DBUILD_SHARED_LIBS=OFF ^
	-DOJPH_BUILD_EXECUTABLES=OFF ^
	-DCMAKE_INSTALL_PREFIX=%OTS_DEV_SPACE%/openjph/%BUILD_TYPE% ^
	-DCMAKE_POLICY_DEFAULT_CMP0091=NEW ^
	-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$^<$^<CONFIG:Debug^>:Debug^>

cmake --build . --parallel %NUMBER_OF_PROCESSORS% --config %BUILD_TYPE%
cmake --install . --config %BUILD_TYPE%

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%BUILD_DIR_LIB%" mkdir "%BUILD_DIR_LIB%"
cd /d "%BUILD_DIR%"

cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
	-DCMAKE_CXX_STANDARD=11 ^
	-DBUILD_SHARED_LIBS=OFF ^
	-DBUILD_TESTING=ON ^
	-DDCMTK_ROOT=%OTS_DEV_SPACE%/dcmtk/%BUILD_TYPE% ^
	-DOPENJPH_DIR=%OTS_DEV_SPACE%/openjph/%BUILD_TYPE%/lib/cmake/openjph ^
	-DDCMTKHTJ2K_ROOT=%OTS_DEV_SPACE%/dcmtkhtj2k/%BUILD_TYPE% ^
	-DCMAKE_INSTALL_PREFIX=%BUILD_DIR_LIB% ^
	-DCMAKE_POLICY_DEFAULT_CMP0091=NEW ^
	-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$^<$^<CONFIG:Debug^>:Debug^>

cmake --build . --parallel %NUMBER_OF_PROCESSORS% --config %BUILD_TYPE%
cmake --install . --config %BUILD_TYPE%

endlocal
