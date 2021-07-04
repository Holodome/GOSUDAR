@echo off

cls
if not exist .\build mkdir build 
pushd build 

where cl /q
if %ERRORLEVEL% neq 0 call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

set "build_options=-nologo -DEBUG -fp:fast -Od -Oi -Zi -FC -MTd -wd4201 -WX -I"..\src" -I"..\thirdparty" -std:c++17 -D_CRT_SECURE_NO_WARNINGS"

cl %build_options% ../tools/asset_file_builder.cc -link -opt:ref kernel32.lib -out:asset_builder.exe

popd 