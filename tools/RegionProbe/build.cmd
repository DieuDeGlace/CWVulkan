@echo off
rem Builds RegionProbe.dll (CubeModLoader dev mod) with MSVC.
setlocal
set "VSDIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d "%~dp0"
cl /nologo /LD /O2 /std:c++17 /EHsc /MD /I..\..\..\SteamCubeWorldMods\vendor\cwmods RegionProbe.cpp /Fe:RegionProbe.dll /link user32.lib || exit /b 1
