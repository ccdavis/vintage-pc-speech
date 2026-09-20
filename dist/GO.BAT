@echo off
rem accessible_os / FreeDOS 386 speech test.  Run blind from the DOS prompt as:   A:\GO
rem If the floppy is another drive, pass it:   B:\GO B:
rem Beeps (PC speaker): 1 = started, 2 = files copied, 3 = finished.
rem Sound Blaster: 440 Hz tone, then a pre-recorded sentence, then live SAM speech.
rem Results: %SRC%\RESULTS.TXT (also spoken at the end). Files go to C:\SPEECH.
rem Keep every line under 127 characters: DR-DOS and MS-DOS COMMAND.COM drop longer ones.
set SRC=A:
if not "%1"=="" set SRC=%1
echo 
if not exist C:\SPEECH\NUL md C:\SPEECH
copy %SRC%\*.EXE C:\SPEECH > NUL
copy %SRC%\*.WAV C:\SPEECH > NUL
copy %SRC%\*.TXT C:\SPEECH > NUL
copy %SRC%\*.BAT C:\SPEECH > NUL
copy %SRC%\*.DOC C:\SPEECH > NUL
C:
cd \SPEECH
echo 
echo accessible_os FreeDOS 386 speech test > %SRC%\RESULTS.TXT
echo Test complete. Results are on the floppy. Here are the timings. > %SRC%\SUMMARY.TXT
ver >> %SRC%\RESULTS.TXT
echo BLASTER=%BLASTER% >> %SRC%\RESULTS.TXT
echo. >> %SRC%\RESULTS.TXT
echo --- SBPLAY -tone 440 >> %SRC%\RESULTS.TXT
SBPLAY -tone 440 >> %SRC%\RESULTS.TXT
if errorlevel 1 echo SBPLAY TONE FAILED >> %SRC%\RESULTS.TXT
if errorlevel 1 echo sbplay tone failed >> %SRC%\SUMMARY.TXT
echo --- SBPLAY HELLO.WAV >> %SRC%\RESULTS.TXT
SBPLAY HELLO.WAV >> %SRC%\RESULTS.TXT
if errorlevel 1 echo SBPLAY WAV FAILED >> %SRC%\RESULTS.TXT
if errorlevel 1 echo sbplay wav failed >> %SRC%\SUMMARY.TXT
echo --- SAMSB short >> %SRC%\RESULTS.TXT
SAMSB -log %SRC%\SUMMARY.TXT Hello. This is the speech test running on the three eighty six. >> %SRC%\RESULTS.TXT
if errorlevel 1 echo SAMSB 1 FAILED >> %SRC%\RESULTS.TXT
if errorlevel 1 echo samsb 1 failed >> %SRC%\SUMMARY.TXT
echo --- SAMSB long >> %SRC%\RESULTS.TXT
SAMSB -log %SRC%\SUMMARY.TXT The quick brown fox jumps over the lazy dog and runs faster than real time. >> %SRC%\RESULTS.TXT
if errorlevel 1 echo SAMSB 2 FAILED >> %SRC%\RESULTS.TXT
if errorlevel 1 echo samsb 2 failed >> %SRC%\SUMMARY.TXT
echo --- SAMSB fast >> %SRC%\RESULTS.TXT
SAMSB -log %SRC%\SUMMARY.TXT -speed 40 Now speaking at the faster rate a screen reader user would set. >> %SRC%\RESULTS.TXT
if errorlevel 1 echo SAMSB 3 FAILED >> %SRC%\RESULTS.TXT
if errorlevel 1 echo samsb 3 failed >> %SRC%\SUMMARY.TXT
echo --- MEM /C >> %SRC%\RESULTS.TXT
mem /c >> %SRC%\RESULTS.TXT
echo --- SET >> %SRC%\RESULTS.TXT
set >> %SRC%\RESULTS.TXT
echo --- END >> %SRC%\RESULTS.TXT
SAMSB -f %SRC%\SUMMARY.TXT
echo 
