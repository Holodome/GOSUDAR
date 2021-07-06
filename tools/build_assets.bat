@echo off

build\simple_timer -start
pushd assets 
..\build\asset_builder.exe assets.info
popd
build\simple_timer -end
del timer.timer