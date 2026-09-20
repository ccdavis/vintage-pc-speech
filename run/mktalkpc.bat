@echo off
set DIRCMD=
rem Turn the FreeDOS boot floppy (B: in the LiveCD) into the talking disk for newer PCs. Run: C:\MKTALKPC
b:\freedos\bin\deltree /y b:\freedos
del b:\setup.bat
del b:\fdauto.bat
del b:\fdconfig.sys
copy c:\talkpc\*.* b:\ > nul
del b:\COPYING.TXT
dir b: /w
echo MKTALKPC DONE
