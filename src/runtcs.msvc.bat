@echo off

@if "%1"=="" goto :HELP1
@if "%2"=="" goto :HELP1
set FBTT=%1
set FBTN=%2

@if "%3" NEQ "" (set FIREBIRD=%3)
:: BRS 
:: Get all the file name when there are spaces
:: this can be also achieved with %* but I don't know which versions of 
:: windows allows it
:LOOP
	@shift
	@if "%3"==""  (goto DONE)
        @set FIREBIRD=%FIREBIRD% %3
	@goto loop
:DONE

@echo FIREBIRD=%FIREBIRD%
@if "%FIREBIRD%"=="" (goto :HELP2 & goto :EOF)

::********************************************************
:: Take care, the tool needs the path with forward slashes
@cd ..
@for /f "delims=" %%a in ('@cd') do (set ROOT_PATH=%%a)
@cd bin
for /f "tokens=*" %%a in ('@echo %ROOT_PATH:\=/%') do (set FBTCS=%%a)

@echo FBTCS=%FBTCS%

:: User for GF_SHUT_2
gsec -delete qa_user2
gsec -add qa_user2 -pw qa_user2
:: User for GF_SHUT_0 and GF_SHUT_1
gsec -delete shut1
gsec -add shut1 -pw shut1

echo test type %FBTT%
echo test name %FBTN%
mkdir ..\temp 2>nul
mkdir ..\output 2>nul

tcs %FBTT% %FBTN%

goto:END2

::===========
:HELP1
@echo.
@echo   To run fbtcs you need to specify a run type and a test/series/meta-series name
@echo   valid run types are 
@echo.
@echo   rt : run one test
@echo   rs : run a test serie
@echo   rms : run a test meta-serie
@echo.
@echo   The valid command syntax for runtcs is
@echo   runtcs type name [FIREBIRD dir]
@echo.
@goto :END2

::===========
:HELP2
@echo.
@echo   The fbtcs need the FIREBIRD environment variable set to work.
@echo   FIREBIRD value should be the root directory of your Firebird installation.
@echo.
@echo   The valid command syntax for runtcs is
@echo   runtcs type name [FIREBIRD dir]
@echo. 
@goto :END2

:END2
