@echo off
cls
pushd .build

set output=light.exe
set entry=../src/WinMain.cpp
set c_defines=/D_DEBUG /DWIN32_LEAN_AND_MEAN
set c_flags=/I../src/ /I../src/vendor/ /permissive /std:c++17 /Zi %c_defines%
set libs=kernel32.lib shell32.lib user32.lib dxgi.lib dxguid.lib d3d11.lib d3dcompiler.lib assimp-vc143-mt.lib
set link_flags=/nologo /incremental:no /out:%output% /libpath:../src/vendor/ %libs%

:BUILD
cl.exe %entry% %c_flags% /link %link_flags%
copy %output% ..

popd
