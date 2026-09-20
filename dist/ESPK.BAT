@echo off
rem Second floppy: the eSpeak NG voice. Run   A:\ESPK   (or B:\ESPK B:) to copy it to C:\SPEECH and test it.
set SRC=A:
if not "%1"=="" set SRC=%1
if not exist C:\SPEECH\NUL md C:\SPEECH
copy %SRC%\ESPK.EXE C:\SPEECH > NUL
copy %SRC%\ESPK.DAT C:\SPEECH > NUL
copy %SRC%\CWSDPMI.EXE C:\SPEECH > NUL
C:
cd \SPEECH
ESPK Hello. This is the e speak voice on the three eighty six. Please open the file and read the first line. > %SRC%\ESPK.TXT
type %SRC%\ESPK.TXT
