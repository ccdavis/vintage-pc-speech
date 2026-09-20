@echo off
rem Universal-DOS test sheet: run under DR-DOS, MS-DOS or FreeDOS after GO (and ESPK from floppy 2).
rem Writes A:\DOSTEST.TXT. Run once per memory configuration (no EMM386 / EMM386 / EMM386 with DPMI).
set SRC=A:
if not "%1"=="" set SRC=%1
C:
cd \SPEECH
echo === DOS TEST > %SRC%\DOSTEST.TXT
ver >> %SRC%\DOSTEST.TXT
echo --- MEM /C >> %SRC%\DOSTEST.TXT
mem /c >> %SRC%\DOSTEST.TXT
echo --- CONFIG.SYS >> %SRC%\DOSTEST.TXT
if exist C:\CONFIG.SYS type C:\CONFIG.SYS >> %SRC%\DOSTEST.TXT
if exist C:\DCONFIG.SYS type C:\DCONFIG.SYS >> %SRC%\DOSTEST.TXT
echo --- LH SBTALK (loads high if UMBs exist) >> %SRC%\DOSTEST.TXT
LH SBTALK COM3 >> %SRC%\DOSTEST.TXT
echo --- MEM /C with SBTALK >> %SRC%\DOSTEST.TXT
mem /c >> %SRC%\DOSTEST.TXT
SAY S B talk is loaded. Now testing the resident e speak.
echo --- ESPKD banner (which DPMI host, where the stub went) >> %SRC%\DOSTEST.TXT
if exist ESPKD.EXE ESPKD COM3 /C C:\SPEECH\DOSTEST2.BAT >> %SRC%\DOSTEST.TXT
echo --- END >> %SRC%\DOSTEST.TXT
SAY The DOS test is finished. Results are on the floppy.
