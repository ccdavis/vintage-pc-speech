@echo off
rem Phase 2 test: resident software synthesizer + Provox screen reader = a talking DOS machine.
rem Run from C:\SPEECH after GO.BAT has copied the files:   TALK
rem It first runs a 6 s diagnostic (interrupt and buffer counters) into A:\SBTALK.TXT.
rem SBTALK emulates a serial DoubleTalk/LiteTalk on COM3 and plays through the Sound Blaster.
rem You should hear "S B talk ready", then Provox's three rising tones, then Provox reading
rem the screen. Any key press stops the current speech. Reboot to unload both.
C:
cd \SPEECH
SBTALK COM3 /TEST > A:\SBTALK.TXT
SBTALK COM3
if errorlevel 1 goto fail
PROVOX7
PV7 LITETALK COM3
echo Provox is now talking through S B talk on the sound blaster.
goto end
:fail
echo SBTALK could not start. Check the BLASTER variable.
:end
