::
:: ���� ��������� ��ப�:
::
:: clean - ������ ���⪠ � ���ᡮઠ
:: init - ᮧ����� Makefile ��� ᡮન ����୨��
::

@ECHO OFF
PUSHD
SETLOCAL
SET "WD=_dbg_"

:: �᫨ ��� ᡮન �ᯮ�짮���� clang
IF "%1"=="clang" (
	SET "CC=C:/usr/bin/clang"
	SET "CXX=C:/usr/bin/clang++"
)

IF EXIST CMakeLists.txt (
	MKDIR %WD%
	COPY /Y /L bake.cmd %WD%\
	CD %WD%
	ECHO.
	ECHO ��������! ��� ᡮન �ᯮ���� ����� '%WD%'.
	ECHO.
	CALL bake.cmd
	GOTO _eof
)

IF "%1"=="clean" (
	RD /Q /S CMakeFiles
	DEL /Q *make*.*
	DEL /Q app*.exe
)

cmake -DCMAKE_BUILD_TYPE=Debug ..\ -G "MinGW Makefiles"

IF "%1"=="init" GOTO _eof
IF "%2"=="init" GOTO _eof

cmake --build .

IF ERRORLEVEL 1 (
	ECHO ----------------
	ECHO ..�訡�� ᡮન
	ECHO.
	pause
	GOTO _eof
)

CALL app_dbg.exe
IF ERRORLEVEL 1 pause

:_eof
ECHO.
ENDLOCAL
POPD
