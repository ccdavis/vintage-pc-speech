@echo off
rem Provox reading a long TYPE through ESPKD (INT 14h traffic with status polls): LOOPS while it speaks.
rem usage: ESPKD COM3 [/PMALL] /C C:\ESPK\PVBENCH.BAT C:\OUT\file
c:\provox7
c:\pv7 LITETALK COM3
c:\espk\loops 300000 >> %1
type c:\espk\long.txt
c:\espk\loops 1500000 >> %1
c:\say /X
