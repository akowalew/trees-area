@echo off

mkdir out >nul 2>nul
pushd out
rem cl ..\trees_area.c /MT /EHa- /EHs- /EHc- /Zf /Zi /FC /W4 /wd4100 /wd4189 /nologo user32.lib gdi32.lib /link /INCREMENTAL:no /OPT:REF
rem cl ..\getfoi.c /MT /EHa- /EHs- /EHc- /Zf /Zi /FC /W4 /wd4100 /wd4189 /nologo ws2_32.lib user32.lib gdi32.lib /link /INCREMENTAL:no /OPT:REF
rem cl ..\get_cemetery_by_id.c /MT /EHa- /EHs- /EHc- /Zf /Zi /FC /W4 /wd4100 /wd4189 /nologo ws2_32.lib user32.lib gdi32.lib /link /INCREMENTAL:no /OPT:REF
rem cl ..\parse_cemeteries.c /MT /EHa- /EHs- /EHc- /Zf /Zi /FC /W4 /wd4100 /wd4189 /nologo ws2_32.lib user32.lib gdi32.lib /link /INCREMENTAL:no /OPT:REF
cl ..\fetch_cemetery.c /MT /EHa- /EHs- /EHc- /Zf /Zi /FC /W4 /Wall /wd4820 /wd5045 /wd4100 /wd4189 /nologo ws2_32.lib user32.lib gdi32.lib /link /INCREMENTAL:no /OPT:REF
popd
