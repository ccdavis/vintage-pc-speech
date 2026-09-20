@echo off
rem DECtalk, resident, with Provox, straight from this disk (needs a 386 + 2 MB XMS; CWSDPMI is here):
rem   DTALK          through a Sound Blaster (BLASTER variable, default A220 I5 D1)
rem   DTALK SBEMU    no Sound Blaster: load HDPMI32i + SBEMU first (Pentium, Intel HD Audio / AC97 / PCI sound)
rem   DTALK SPK      on the PC speaker (386 or better, no sound card)
rem Run it from the disk's own drive (A:\DTALK). Provox comes from disk 1 (GO copies it to C:\SPEECH).
rem EXIT in the talking shell unloads everything.
set PATH=C:\SPEECH;%PATH%
if "%1"=="SPK" goto spk
if "%1"=="SBEMU" goto sbemu
DTALKD COM3 /LOW /C DTALK2.BAT
if errorlevel 1 goto nosb
goto end
:nosb
echo No Sound Blaster answered at the BLASTER address. Trying SBEMU (HDPMI32i + SBEMU, Pentium or better).
:sbemu
JLOAD QPIEMU.DLL > NUL
HDPMI32I -r -x
SBEMU /A220 /I5 /D1 /T6 /O1 /K11025 /VOL100
set BLASTER=A220 I5 D1 H5 P330 T6
DTALKD COM3 /LOW /C DTALK2.BAT
goto end
:spk
DTALKD COM3 /LOW /SPK /C DTALK2.BAT
:end
