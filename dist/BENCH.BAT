@echo off
rem Synthesis speed of both engines on this machine (no sound). Run from C:\SPEECH:  BENCH
rem Results go to A:\BENCH.TXT. Each line: "bench ENGINE: X ms of audio in Y ms = Z x real time".
rem Z above 1.0 means real time. SBTALK3 needs a 386 or better.
C:
cd \SPEECH
SBTALK COM3 /BENCH > A:\BENCH.TXT
SBTALK3 COM3 /BENCH >> A:\BENCH.TXT
SBTALK3 COM3 /KLATT /BENCH >> A:\BENCH.TXT
SAMSB -f A:\BENCH.TXT
