@echo off
rem Builds ZoneExtractor.dll (CubeModLoader mod) with MSVC.
setlocal
set "VSDIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d "%~dp0"
cl /nologo /LD /O2 /std:c++17 /EHsc /MT /I..\..\..\SteamCubeWorldMods\vendor\cwmods ZoneExtractor.cpp /Fe:ZoneExtractor.dll /link user32.lib d3d11.lib dxgi.lib || exit /b 1
rem /MT: the game ships an old MSVCP140 (2015) whose std::mutex is incompatible with newer headers.
