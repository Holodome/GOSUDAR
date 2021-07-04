@echo off

cls
if not exist .\build (
    mkdir build 
)

if not exist .\build\simple_timer.exe (
    call tools\build_simple_timer.bat
)

pushd build 

set "build_options=-nologo -DEBUG -fp:fast -Od -Oi -Zi -FC -MTd -wd4201 -WX -I"..\src" -I"..\thirdparty" -std:c++17 -D_CRT_SECURE_NO_WARNINGS"

simple_timer -start
cl %build_options% ../src/compile.cc -link -opt:ref gdi32.lib user32.lib kernel32.lib -out:game.exe
copy game.exe ..\game\
simple_timer -end

popd 