@echo off
rem CPU cost of the resident ESPKD: LOOPS idle, then LOOPS twice while a long text is spoken.
rem usage (from ESPKD's child shell, or ESPKD COM3 /C C:\ESPK\BENCH.BAT C:\OUT\EB5.TXT)
echo idle >> %1
c:\espk\loops 1500000 >> %1
c:\say /F c:\espk\long.txt
echo speaking >> %1
c:\espk\loops 1500000 >> %1
c:\espk\loops 1500000 >> %1
c:\say /X
echo flushed >> %1
c:\espk\loops 300000 >> %1
mem /c | find "free" >> %1
