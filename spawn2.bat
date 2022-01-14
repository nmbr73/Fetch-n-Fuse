@echo OFF

cd /D %2

python fuse --id %1

if "%errorlevel%"=="0" (
	echo OK 1
	echo Result: %ERRORLEVEL%
) else (
	echo Fehler 	
	echo Result: %ERRORLEVEL%
        pause
)