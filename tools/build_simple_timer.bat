@echo off

cls
if not exist .\build mkdir build 
pushd build 

set "build_options=-nologo -DEBUG -fp:fast -O2 -Oi -Zi -FC -MTd -wd4201 -WX -I"..\src" -I"..\thirdparty" -std:c++17 -D_CRT_SECURE_NO_WARNINGS"

cl %build_options% ../tools/simple_timer.cc -link -opt:ref -out:simple_timer.exe

popd 