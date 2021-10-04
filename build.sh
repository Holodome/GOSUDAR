#!/bin/sh
mkdir -p build
# get list of all .c files
engine_filenames=$(find engine/ -type f -name "*.c" -o -name "*.c" -o -name "*.m")
shared_filenames=$(find shared/ -type f -name "*.c" -o -name "*.c" -o -name "*.m")
filenames="$engine_filenames $shared_filenames"
echo Filenames: $filenames

vulkan_path="/opt/homebrew/Cellar/molten-vk/1.1.5/include"
build_options="-O0 -std=c11 -Ishared -Iengine -I$vulkan_path -Ithirdparty -D_CRT_SECURE_NO_WARNINGS"
error_policy="-Wshadow -Wextra -Wall -Werror -Wno-unused-function -Wno-missing-braces -Wformat=2"
frameworks="-framework AppKit
            -framework IOKit
            -framework Metal
            -framework Foundation
            -framework IOSurface
            -framework AudioToolbox"
defines=""
vulkan_lib="/opt/homebrew/Cellar/molten-vk/1.1.5/lib/libMoltenVK.a"
clang -g $build_options $error_policy $frameworks $defines -o build/game $vulkan_lib $filenames
#cp test.txt build/
