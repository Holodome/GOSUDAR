#!/bin/sh
mkdir -p build
# get list of all .c files
engine_filenames=$(find engine/ -type f -name "*.c" -o -name "*.m")
game_filenames="game/game.c"
main_filenames="game/main.c"
echo engine_filenames: $engine_filenames
echo game_filenames: $game_filenames
echo main_filenames: $main_filenames

vulkan_path="/opt/homebrew/Cellar/molten-vk/1.1.5"
error_policy="-Wshadow -Wextra -Wall -Werror -Wno-unused-function -Wno-missing-braces -Wformat=2"
build_options="-O0 -std=c11 -Iengine -I$vulkan_path/include -Ithirdparty $error_policy"

frameworks="-framework AppKit
            -framework IOKit
            -framework Metal
            -framework Foundation
            -framework IOSurface
            -framework QuartzCore
            -framework AudioToolbox"
vulkan_lib="$vulkan_path/lib/libMoltenVK.a"
clang -g $build_options $frameworks -DCOMPILE_ENGINE -o build/engine.dylib -dynamiclib $vulkan_lib $engine_filenames
clang -g $build_options -o build/game.dylib -dynamiclib build/engine.dylib $game_filenames
clang -g $build_options -o build/game build/engine.dylib $main_filenames
