@echo off
setlocal EnableDelayedExpansion

REM Build DCMTK-HTJ2K for WebAssembly using Emscripten (emsdk latest) and mingw32-make.
REM Prerequisites: Git, CMake, MinGW (mingw32-make on PATH), and emsdk installed.
REM Ensure emsdk and MinGW are available on PATH.

where emsdk.bat >nul 2>nul
if errorlevel 1 (
	echo Warning: emsdk not found on PATH.
	echo Install emsdk or Add the EMSDK to PATH.
	echo Install emsdk from: https://emscripten.org/docs/getting_started/downloads.html
	exit /b 1
)

REM Ensure mingw32-make is available (MinGW bin on PATH)
where mingw32-make >nul 2>nul
if errorlevel 1 (
	echo Warning: mingw32-make not found on PATH. Add MinGW to PATH.	
	exit /b 1
)

set "PROJECT_ROOT=%CD%"
set "BUILD_TYPE=Release"
set "OTS_WASM=%CD%\ots_wasm"
set "BUILD_DIR=%CD%\build_wasm"
set "BUILD_DIR_LIB=%CD%\build_wasm\%BUILD_TYPE%"


if not exist "%OTS_WASM%" mkdir "%OTS_WASM%"
cd /d "%OTS_WASM%"

REM ----- DCMTK (WASM) -----
if not exist "dcmtk" git clone https://github.com/DCMTK/dcmtk.git
cd dcmtk
git fetch
git checkout -f DCMTK-3.6.9
REM Apply Emscripten fix for ofwhere.c (DCMTK unchanged; patch applied at build time)
call git apply "%PROJECT_ROOT%\patches\dcmtk-ofwhere-emscripten.patch"
if errorlevel 1 (
	echo ERROR: Failed to apply patches\dcmtk-ofwhere-emscripten.patch
	echo Ensure the patch matches your DCMTK version.
	exit /b 1
)
if not exist "%BUILD_TYPE%" mkdir "%BUILD_TYPE%"
cd %BUILD_TYPE%

call emcmake cmake .. -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM=mingw32-make ^
 	-DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
	-DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
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
	-DCMAKE_INSTALL_PREFIX=%OTS_WASM%/dcmtk/%BUILD_TYPE% ^
	-DCMAKE_POLICY_DEFAULT_CMP0091=NEW
REM Continue to make even if cmake returned non-zero (e.g. missing BISON/FLEX)

call emmake mingw32-make -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 (echo ERROR: DCMTK make failed. & exit /b 1)
call emmake mingw32-make install
if errorlevel 1 (echo ERROR: DCMTK install failed. & exit /b 1)

cd /d "%OTS_WASM%"

REM ----- OpenJPH (WASM) -----
if not exist "openjph" git clone https://github.com/aous72/openjph.git
cd openjph
git fetch
git checkout -f 0.26.0
if not exist "%BUILD_TYPE%" mkdir "%BUILD_TYPE%"
cd %BUILD_TYPE%

call emcmake cmake .. -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM=mingw32-make ^
	-DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
	-DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
	-DCMAKE_CXX_STANDARD=11 ^
	-DBUILD_SHARED_LIBS=OFF ^
	-DOJPH_BUILD_EXECUTABLES=OFF ^
	-DCMAKE_INSTALL_PREFIX=%OTS_WASM%/openjph/%BUILD_TYPE% ^
	-DCMAKE_POLICY_DEFAULT_CMP0091=NEW

call emmake mingw32-make -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 (echo ERROR: OpenJPH make failed. & exit /b 1)
call emmake mingw32-make install
if errorlevel 1 (echo ERROR: OpenJPH install failed. & exit /b 1)

REM ----- Main project (DCMTK-HTJ2K) WASM -----
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%BUILD_DIR_LIB%" mkdir "%BUILD_DIR_LIB%"
cd /d "%BUILD_DIR%"

call emcmake cmake .. -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM=mingw32-make ^
	-DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
	-DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
	-DCMAKE_CXX_STANDARD=11 ^
	-DBUILD_SHARED_LIBS=OFF ^
	-DBUILD_TESTING=OFF ^
	-DCMAKE_PREFIX_PATH=%OTS_WASM%/dcmtk/%BUILD_TYPE% ^
	-DDCMTK_DIR=%OTS_WASM%/dcmtk/%BUILD_TYPE%/lib/cmake/dcmtk ^
	-DDCMTK_ROOT=%OTS_WASM%/dcmtk/%BUILD_TYPE% ^
	-DOPENJPH_DIR=%OTS_WASM%/openjph/%BUILD_TYPE%/lib/cmake/openjph ^
	-DDCMTKHTJ2K_ROOT=%OTS_WASM%/dcmtkhtj2k/%BUILD_TYPE% ^
	-DCMAKE_INSTALL_PREFIX=%BUILD_DIR_LIB% ^
	-DCMAKE_POLICY_DEFAULT_CMP0091=NEW

call emmake mingw32-make -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 (echo ERROR: Main project make failed. & exit /b 1)
call emmake mingw32-make install
if errorlevel 1 (echo ERROR: Main project install failed. & exit /b 1)

cd /d "%~dp0"
echo.
echo WASM build complete. Output in build_wasm\%BUILD_TYPE%
endlocal
