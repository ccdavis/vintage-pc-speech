@echo off
rem INT 14h call cost through ESPKD (real-mode stub vs real-mode callback); waits ~2 s first so the load message is over
c:\espk\loops 400000 > nul
c:\espk\int14b 5000 >> %1
