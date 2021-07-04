@echo off
pushd assets 
..\build\asset_builder.exe assets.assets
copy assets.assets ..\game\
del assets.assets
popd