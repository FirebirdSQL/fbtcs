@echo off

@if "%1" NEQ "" (set FIREBIRD=%1)
:: BRS 
:: Get all the file name when there are spaces
:: this can be also achieved with %* but I don't know which versions of 
:: windows allows it
:LOOP
	@shift
	@if "%1"==""  (goto DONE)
		@set FIREBIRD=%FIREBIRD% %1
	@goto loop
:DONE

@echo      FIREBIRD=%FIREBIRD%
@if "%FIREBIRD%"=="" (goto :HELP & goto :EOF)

del *.obj *.exe > nul 2>&1

@echo.
@echo building tcs.exe
cl tcs.cpp exec.cpp trns.cpp do_diffs.cpp /D "WIN_NT" /I "%FIREBIRD%/include/" /nologo > build.log
@echo.
@echo building drop_gdb.exe
cl drop_gdb.cpp /D "WIN_NT" /I "%FIREBIRD%/include/" %FIREBIRD%\lib\fbclient_ms.lib /nologo >> build.log
@echo.
@echo building diffs.exe
cl diffs.cpp do_diffs.cpp /D "WIN_NT" /I "%FIREBIRD%/include/" /nologo >> build.log
@echo.
@echo building create_file.exe
cl create_file.cpp /D "WIN_NT" /I "%FIREBIRD%/include/" /nologo >> build.log
@echo.
@echo building compare_file.exe
cl compare_file.cpp /D "WIN_NT" /I "%FIREBIRD%/include/" /nologo >> build.log
@echo.
@echo building copy_file.exe
cl copy_file.cpp /D "WIN_NT" /I "%FIREBIRD%/include/" /nologo >> build.log
@echo.
@echo building delete_file.exe
cl delete_file.cpp ws2_32.lib /D "WIN_NT" /I "%FIREBIRD%/include/" /nologo >> build.log
@echo.
@echo building sleep_sec.exe
cl sleep_sec.cpp /D "WIN_NT" /I "%FIREBIRD%/include/" /nologo >> build.log

mkdir ..\..\bin 2>nul
copy /Y ..\tcs.config.msvc ..\..\bin\tcs.config > nul
copy /Y ..\runtcs.msvc.bat ..\..\bin\runtcs.bat > nul
move /Y compare_file.exe ..\..\bin\
move /Y copy_file.exe ..\..\bin\
move /Y create_file.exe ..\..\bin\
move /Y delete_file.exe ..\..\bin\
move /Y diffs.exe ..\..\bin\
move /Y drop_gdb.exe ..\..\bin\
move /Y sleep_sec.exe ..\..\bin\
move /Y tcs.exe ..\..\bin\

del *.obj
goto:END2

::===========
:HELP
@echo.
@echo   Build process need the FIREBIRD environment variable set to work.
@echo   FIREBIRD value should be the root directory of your Firebird installation.
@echo   Example:
@echo     c:\program files\firebird
@echo. 
@goto :END2

:END2
