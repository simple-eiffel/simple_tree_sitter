@echo off
rem simple_tree_sitter: the static library for Windows.
rem
rem   build.bat          build simple_tree_sitter.lib
rem   build.bat test     build it, then build and run the C tests
rem   build.bat clean
rem
rem Run it from a Visual Studio x64 developer prompt, so that cl and lib are on the path.
rem macOS and Linux use the Makefile.

setlocal
cd /d "%~dp0"

if /i "%1"=="clean" (
	if exist build rmdir /s /q build
	if exist simple_tree_sitter.lib del simple_tree_sitter.lib
	exit /b 0
)

if not exist build mkdir build
set TS=tree-sitter\lib
set FLAGS=/nologo /c /O2 /std:c11 /utf-8 /DWIN32

cl %FLAGS% /W3 /I%TS%\include simple_tree_sitter.c /Fobuild\simple_tree_sitter.obj || exit /b 1
cl %FLAGS% sts_queries.c /Fobuild\sts_queries.obj || exit /b 1
cl %FLAGS% /I%TS%\include /I%TS%\src %TS%\src\lib.c /Fobuild\ts_lib.obj || exit /b 1
cl %FLAGS% /Igrammars\eiffel\src grammars\eiffel\src\parser.c /Fobuild\eiffel_parser.obj || exit /b 1
cl %FLAGS% /Igrammars\c\src grammars\c\src\parser.c /Fobuild\c_parser.obj || exit /b 1
cl %FLAGS% /Igrammars\json\src grammars\json\src\parser.c /Fobuild\json_parser.obj || exit /b 1
cl %FLAGS% /Igrammars\xml\src grammars\xml\src\parser.c /Fobuild\xml_parser.obj || exit /b 1
cl %FLAGS% /Igrammars\xml\src grammars\xml\src\scanner.c /Fobuild\xml_scanner.obj || exit /b 1

if exist simple_tree_sitter.lib del simple_tree_sitter.lib
lib /nologo /out:simple_tree_sitter.lib build\simple_tree_sitter.obj build\sts_queries.obj build\ts_lib.obj build\eiffel_parser.obj build\c_parser.obj build\json_parser.obj build\xml_parser.obj build\xml_scanner.obj || exit /b 1

if /i "%1"=="test" (
	cl /nologo /O2 /std:c11 /utf-8 /W3 /I. test\sts_test.c simple_tree_sitter.lib /Fobuild\sts_test.obj /Febuild\sts_test.exe || exit /b 1
	build\sts_test.exe ..\testing\fixtures ..\queries || exit /b 1
)
exit /b 0
