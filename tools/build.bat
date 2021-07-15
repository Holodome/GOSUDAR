@echo off

cls
if not exist .\build (
    mkdir build 
)

if not exist .\build\simple_timer.exe (
    call tools\build_simple_timer.bat
)

where cl /q
if %ERRORLEVEL% neq 0 call clin.bat

pushd build 

set "build_options=-nologo -DEBUG -fp:fast -Od -Oi -Zi -FC -MTd -GS- -Gs9999999 -wd4201 -WX -I"..\src" -I"..\thirdparty" -std:c++17 -D_CRT_SECURE_NO_WARNINGS"
rem set "link_options=-stack:0x100000,0x100000 -NODEFAULTLIB -ENTRY:main -ignore:4210 -opt:ref gdi32.lib user32.lib kernel32.lib "
set "link_options=-opt:ref gdi32.lib user32.lib kernel32.lib "

simple_timer -start
cl %build_options% ../src/compile.cc -link %link_options% -out:game.exe
copy game.exe ..\game\
simple_timer -end

popd 
