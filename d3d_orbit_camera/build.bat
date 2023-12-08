@echo off
cls
pushd .build

set libs=kernel32.lib shell32.lib user32.lib dxgi.lib dxguid.lib d3d11.lib d3dcompiler.lib
set c_defines=/DWIN32_LEAN_AND_MEAN /D_DEBUG
set i_am_speed=/GS- /Gs999999 /Gm- /GR- /EHa- /Oi

cl ../src/main.cpp /I../src/ /std:c++20 /permissive /Zi %i_am_speed% %c_defines% /link /out:cam.exe /nologo /incremental:no %libs% 
copy cam.exe ..

popd