@echo off
rem Resident eSpeak voice: ESPKD stays running, hosts a DOS shell, and drives Provox through the
rem Sound Blaster (TALKD) or the PC speaker (TALKD SPK). Type EXIT in that shell to unload.
C:
cd \SPEECH
if "%1"=="SPK" ESPKD COM3 /SPK /C C:\SPEECH\TALKD2.BAT
if not "%1"=="SPK" ESPKD COM3 /C C:\SPEECH\TALKD2.BAT
