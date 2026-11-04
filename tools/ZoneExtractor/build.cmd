@echo off
rem Build the maintained CMake target; DLL and intermediates stay under build.
rem Optional arguments are forwarded to the build backend (for example, -j4).
setlocal
pushd "%~dp0..\.." || exit /b 1
call build.cmd --target ZoneExtractor -- %*
set "EXTRACTOR_BUILD_RESULT=%errorlevel%"
popd
exit /b %EXTRACTOR_BUILD_RESULT%
