@echo off
rem Optional second test: load the Provox screen reader (TSR) and record its status.
rem Run after GO.BAT, from C:\SPEECH:   PROVOX
rem You should hear three rising tones from the PC speaker when it installs.
rem It is activated on COM2 (LITETALK protocol, 9600 baud); with a null-modem cable to
rem another computer running a terminal program you can watch the text it speaks.
set SRC=A:
if not "%1"=="" set SRC=%1
C:
cd \SPEECH
PROVOX7
PV7 STATUS > %SRC%\PROVOX.TXT
PV7 LITETALK COM2 >> %SRC%\PROVOX.TXT
PV7 STATUS >> %SRC%\PROVOX.TXT
echo Provox is loaded and talking to COM2. >> %SRC%\PROVOX.TXT
SAMSB Provox is loaded and sending speech to COM2.
