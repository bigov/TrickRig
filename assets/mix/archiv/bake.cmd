::
:: ���� ��������� ��ப�:
::
:: clang - ��������� �ணࠬ��� clang
:: clang release - ��������� �ணࠬ��� clang ��� �⫠���
:: gcc release   - ��������� �ணࠬ��� gcc ��� �⫠���
::
:: clean - ������ ���⪠ � ���ᡮઠ
:: init - ᮧ����� Makefile ��� ᡮન ����୨��
::

@ECHO OFF
PUSHD
SETLOCAL
CHCP 65001

:: ��� �㤥� ᮡ����:
SET "PATH=%PATH%;w:\exe\msys\mingw64\bin"

SET "n2=dbg"
SET "n3=gcc"

:: �᫨ ��� ᡮન �ᯮ�짮���� clang
IF "%1"=="clang" (
  SET "CC=clang.exe"
  SET "CXX=clang++.exe"
  SET "n3=clang"
)

IF "%1"=="clean" (
  RD /Q /S CMakeFiles
  DEL /Q *make*.*
  GOTO _eof
)

IF "%2"=="release" (
  cmake -DCMAKE_BUILD_TYPE=Release ..\ -G "MinGW Makefiles"
  SET "n2=rel"
) ELSE (
  cmake -DCMAKE_BUILD_TYPE=Debug ..\ -G "MinGW Makefiles"
)

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

CALL app%n2%_%n3%.exe
IF ERRORLEVEL 1 pause

:_eof
ECHO.
CHCP 866
ENDLOCAL
POPD
