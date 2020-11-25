@echo off

if not exist .\build mkdir build 
pushd build 

set "DisabledErrors=-Wall -Wno-writable-strings -Wno-unused-function -Wparentheses -Wswitch"
set "LinkedLibraries= -L./ -luser32 -lkernel32 -lgdi32"
set "CompilerSwitches=-D_CRT_SECURE_NO_WARNINGS"
set "BuildOptions= %CompilerSwitches% -gcodeview -O0 -I../src %DisabledErrors% %LinkedLibraries% -x c -std=c11 -fno-exceptions -fno-math-errno -Wno-c99-designator"

clang -g %BuildOptions% -o minecraft.exe ../src/main.c 

popd 