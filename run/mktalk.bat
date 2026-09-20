@echo off
set DIRCMD=
rem Turn the FreeDOS boot floppy (drive B: in the LiveCD) into the talking disk. Run: C:\MKTALK
b:\freedos\bin\deltree /y b:\freedos\setup
b:\freedos\bin\deltree /y b:\freedos\nls
del b:\freedos\bin\v*.com
del b:\freedos\bin\fd*.*
del b:\freedos\bin\*.bat
del b:\freedos\bin\*.sys
del b:\freedos\bin\*.dll
del b:\freedos\bin\*.ini
del b:\freedos\bin\*.cfg
del b:\freedos\bin\*.oem
del b:\freedos\bin\shsucdx.com
del b:\freedos\bin\shsurdrv.exe
del b:\freedos\bin\srdisk.exe
del b:\freedos\bin\lbacache.com
del b:\freedos\bin\doslfn.com
del b:\freedos\bin\devload.com
del b:\freedos\bin\tickle.com
del b:\freedos\bin\vcpi.exe
del b:\freedos\bin\jemmex.exe
del b:\freedos\bin\zip.exe
del b:\freedos\bin\grep.exe
del b:\freedos\bin\fc.exe
del b:\setup.bat
del b:\freedos\bin\format.exe
del b:\freedos\bin\sys.com
del b:\freedos\bin\xcopy.exe
del b:\freedos\bin\attrib.com
del b:\freedos\bin\deltree.com
del b:\freedos\bin\cpuid.exe
copy c:\sbtalk.exe b:\ > nul
copy c:\sbtalk3.exe b:\ > nul
copy c:\sbtalkxt.exe b:\ > nul
copy c:\say.exe b:\ > nul
copy c:\provox7.exe b:\ > nul
copy c:\pv7.exe b:\ > nul
copy c:\espk\espk.exe b:\ > nul
copy c:\espk\espk.dat b:\ > nul
copy c:\cwsdpmi.exe b:\ > nul
copy c:\talkdisk\fdconfig.sys b:\ > nul
copy c:\talkdisk\kernl86.sys b:\kernel.sys > nul
copy c:\talkdisk\fdauto.bat b:\ > nul
copy c:\talkdisk\readme.txt b:\ > nul
copy c:\talkdisk\help.bat b:\ > nul
copy c:\talkdisk\blaster.bat b:\ > nul
copy c:\talkdisk\resetvoi.bat b:\ > nul
dir b: /w
dir b:\freedos\bin /w
echo MKTALK DONE
