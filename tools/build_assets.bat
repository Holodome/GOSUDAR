@echo off
pushd assets 
..\build\asset_builder.exe assets.info
copy assets.assets ..\game\
del assets.assets
popd