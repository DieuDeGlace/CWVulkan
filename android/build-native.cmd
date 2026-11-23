@echo off
rem Builds the Android app's native library (arm64-v8a) without Gradle, to check the port compiles.
rem Needs the Android NDK (ANDROID_NDK_HOME) and a desktop build's build\spirv_compile.exe.
rem The native build goes to %CUBEVULKAN_ANDROID_BUILD% (default C:\cva: a short path keeps the
rem fetched dependencies under Windows' 260-character path limit).
setlocal
set "VSDIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"
if not exist "%VSDIR%\Common7" set "VSDIR=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
set "CMAKE=%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA=%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
if "%ANDROID_NDK_HOME%"=="" (echo Set ANDROID_NDK_HOME to the NDK folder.& exit /b 1)
if "%CUBEVULKAN_ANDROID_BUILD%"=="" set "CUBEVULKAN_ANDROID_BUILD=C:\cva"
cd /d "%~dp0.."
if not exist "%CUBEVULKAN_ANDROID_BUILD%\build.ninja" "%CMAKE%" -S . -B "%CUBEVULKAN_ANDROID_BUILD%" -G Ninja ^
  -DCMAKE_MAKE_PROGRAM="%NINJA%" -DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake" ^
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-33 -DCMAKE_BUILD_TYPE=RelWithDebInfo || exit /b 1
"%CMAKE%" --build "%CUBEVULKAN_ANDROID_BUILD%" %* || exit /b 1
