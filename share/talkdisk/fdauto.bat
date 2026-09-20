@echo off
set PATH=A:\;A:\FREEDOS\BIN
set DIRCMD=/P
call A:\BLASTER.BAT
if exist A:\VOICE.BAT goto saved
echo Checking the CPU...
A:\SBTALK /CPU > NUL
if errorlevel 3 goto menu
rem --- XT or 286 class: only the 1983 voice is fast enough; no RTC, so the speaker uses timer 0
:menuxt
A:\SBTALKXT /SPK /RETRO /BITS /SAY Welcome to the talking disk on a small machine. This is the 1983 voice.
A:\SBTALKXT /SPK /RETRO /BITS /SAY Press 1 for a Sound Blaster. Press 2 for the P C speaker.
A:\SBTALK /ASK
if errorlevel 3 goto menuxt
if errorlevel 2 goto xtspk
if errorlevel 1 goto xtsb
goto menuxt
:xtsb
echo A:\SBTALKXT COM3 /RETRO /BITS > A:\VOICE.BAT
goto saved
:xtspk
echo A:\SBTALKXT COM3 /SPK /RETRO /BITS > A:\VOICE.BAT
goto saved
:menu
echo Speaking the menu on the PC speaker...
A:\ESPK -k Welcome to the talking disk. This is the e speak voice on the P C speaker.
if errorlevel 1 goto menusam
A:\ESPK -k Press 1 for a Sound Blaster with the SAM voice. Press 2 for the Klatt voice.
A:\ESPK -k Press 3 for the 1983 voice on the Sound Blaster. Press 4 to keep the P C speaker.
goto ask
:menusam
A:\SBTALK /SPK /SAY Welcome to the talking disk. This is the P C speaker voice.
A:\SBTALK /SPK /SAY Press 1 for a Sound Blaster with the SAM voice. Press 2 for the Klatt voice.
A:\SBTALK /SPK /SAY Press 3 for the 1983 voice on the Sound Blaster. Press 4 to keep the P C speaker.
:ask
A:\SBTALK /ASK
if errorlevel 5 goto menu
if errorlevel 4 goto spk
if errorlevel 3 goto retro
if errorlevel 2 goto klatt
if errorlevel 1 goto sam
goto menu
:sam
echo A:\SBTALK COM3 > A:\VOICE.BAT
goto saved
:klatt
echo A:\SBTALK3 COM3 /KLATT > A:\VOICE.BAT
goto saved
:retro
echo A:\SBTALK3 COM3 /RETRO > A:\VOICE.BAT
goto saved
:spk
echo A:\SBTALK COM3 /SPK > A:\VOICE.BAT
goto saved
:saved
if exist A:\LATE.FLG goto late
call A:\VOICE.BAT
if errorlevel 1 goto fail
cls
A:\PROVOX7
A:\PV7 LITETALK COM3
goto ready
:late
rem timer-0 speaker: the synthesizer must hook the timer after Provox so Provox still sees 18.2 Hz
cls
A:\PROVOX7
call A:\VOICE.BAT
A:\PV7 LITETALK COM3
:ready
A:\SAY The talking disk is ready. Type help and press enter for a list of commands.
A:\SAY To choose another voice, type reset voi and reboot.
goto end
:fail
A:\SBTALK /SPK /SAY The sound card did not answer. Falling back to the P C speaker.
del A:\VOICE.BAT
A:\SBTALK COM3 /SPK
cls
A:\PROVOX7
A:\PV7 LITETALK COM3
:end
