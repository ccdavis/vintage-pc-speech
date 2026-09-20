@echo off
set PATH=A:\
set DIRCMD=/P
set BLASTER=A220 I5 D1 H5 P330 T6
rem port trapping for real-mode programs (JEMMEX QEMM emulation) and the DPMI host SBEMU and DTALKD run under
A:\JLOAD A:\QPIEMU.DLL
A:\HDPMI32I -r -x
if exist A:\VOICE.BAT goto saved
:menu
echo First boot: looking for the sound hardware. Answer the spoken questions with the number keys.
A:\SBEMU /A220 /I5 /D1 /T6 /O1 /K11025 /VOL100
A:\DTALKD COM3 /LOW /SAY Welcome to the talking disk. This is DEC talk on the speaker output. Press 1 if you can hear this. Otherwise wait ten seconds.
A:\ASK 10
if errorlevel 1 goto spk
A:\SBEMU /O0
A:\DTALKD COM3 /LOW /SAY Now trying the headphone output. Press 1 if you can hear this. Otherwise wait ten seconds.
A:\ASK 10
if errorlevel 1 goto hp
A:\DTALKD COM3 /LOW /SPK /SAY This is DEC talk on the P C speaker. Press 1 to keep it. Otherwise wait, and the menu repeats.
A:\ASK 10
if errorlevel 1 goto pcspk
goto menu
:spk
echo A:\SBEMU /A220 /I5 /D1 /T6 /O1 /K11025 /VOL100 > A:\VOICE.BAT
echo A:\DTALKD COM3 /LOW /C A:\START.BAT > A:\TALK.BAT
goto firstboot
:hp
echo A:\SBEMU /A220 /I5 /D1 /T6 /O0 /K11025 /VOL100 > A:\VOICE.BAT
echo A:\DTALKD COM3 /LOW /C A:\START.BAT > A:\TALK.BAT
goto firstboot
:pcspk
echo rem PC speaker: no Sound Blaster emulation needed > A:\VOICE.BAT
echo A:\DTALKD COM3 /LOW /SPK /C A:\START.BAT > A:\TALK.BAT
goto firstboot
:firstboot
rem SBEMU is already set up for the output that was chosen; start the talking shell directly
echo Saved in A:\VOICE.BAT. On later boots the talking shell starts without questions.
call A:\TALK.BAT
goto done
:saved
call A:\VOICE.BAT
call A:\TALK.BAT
:done
echo Speech unloaded. Run A:\VOICE.BAT then A:\TALK.BAT to start it again, or delete A:\VOICE.BAT for the setup menu.
