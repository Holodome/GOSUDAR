@echo off

cls
if not exist .\build mkdir build 
pushd build 

set "build_options=-nologo -DINTERNAL_BUILD=0 -fp:fast -O2 -Oi -Zi -FC -MTd -wd4201 -WX -I"..\src" -I"..\thirdparty" -std:c++17 -D_CRT_SECURE_NO_WARNINGS"
cl %build_options% ../src/compile.cc -link -opt:ref gdi32.lib user32.lib kernel32.lib -subsystem:windows -entry:mainCRTStartup -out:game_release.exe

popd 
copy build\game_release.exe game\