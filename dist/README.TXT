ACCESSIBLE_OS / FREEDOS  -  PHASE 1 HARDWARE TEST PACKAGE FOR THE 386 DX-25

Unzip everything onto a floppy. On the FreeDOS machine, at the DOS prompt, type:

    A:\GO

and press Enter. Nothing needs to be read from the screen:
  1 beep  (PC speaker)     the batch started
  2 beeps                  files copied to C:\SPEECH
  Sound Blaster:           2 s of 440 Hz, a pre-recorded sentence, then three live SAM
                           sentences (the third at a fast rate)
  spoken summary           SAM reads the timing lines and any FAILED lines
  3 beeps                  finished
Results are written to A:\RESULTS.TXT and A:\SUMMARY.TXT. Bring the floppy back.

The line that matters in RESULTS.TXT:  "synthesized N samples (X s of audio) in Y s"
Y must be well under X for real-time speech.

TALKING DISK: dist/talkdisk.img is a bootable 1.44 MB FreeDOS floppy image that loads
SBTALK and Provox from its autoexec and talks from the first prompt. Write it to a floppy
with Rufus ("DD image" mode), Win32DiskImager, or  dd if=talkdisk.img of=/dev/fd0 . Boot from
it on any PC with a Sound Blaster compatible card (edit A:\BLASTER.BAT for other settings).
Type HELP at the prompt for a spoken command list. At the first boot it speaks a menu on the
PC speaker and asks which sound hardware and voice to use (no sound card needed for that).
SBTALK /SPK uses the PC speaker (RTC-timed PWM, independent of CPU speed);  SBTALK /SPK /SAY text
speaks once without staying resident;  SBTALK /ASK waits for a key and returns it as errorlevel.

PHASE 2: type   TALK   and press Enter (after GO). It loads SBTALK, a resident
software synthesizer that pretends to be a serial DoubleTalk on COM3 and speaks
through the Sound Blaster, then Provox on top of it. The machine should start
talking: "S B talk ready", three rising tones, then the screen contents. Any key
stops the current speech. A 6 s diagnostic goes to A:\SBTALK.TXT first.
JAWS users: configure JAWS for a DoubleTalk LT / LiteTalk on COM3 instead of Provox.
SAY.EXE speaks from the command line once SBTALK is loaded:  SAY hello there
(SAY /X stops speech, SAY /F FILE.TXT reads a file).

BENCH: type   BENCH   (after GO) to time both engines without sound; results in A:\BENCH.TXT
and spoken. SBTALK3.EXE is the 386 build with the second engine:  SBTALK3 COM3 /KLATT
loads the Klatt formant synthesizer (8 kHz) instead of SAM, and  SBTALK3 COM3 /RETRO
loads the 1983 SPEECH.COM one-bit voice (from horndrv's tables) through the Sound Blaster.

ESPK: the eSpeak NG voice (best quality so far, GPL) is on a SECOND floppy (espk386.zip):
insert it and type   A:\ESPK   to copy it to C:\SPEECH, speak a sentence and write its speed
to A:\ESPK.TXT. Afterwards   ESPK any words   speaks from C:\SPEECH, and   ESPK -k any words   plays the same
voice on the PC speaker (no sound card needed).
RESIDENT: type   TALKD   (Sound Blaster) or   TALKD SPK   (PC speaker). ESPKD stays running,
opens a DOS shell with Provox reading the screen through the eSpeak voice; EXIT unloads it.
The CWSDPMI.EXE on this disk has paging disabled, which ESPKD needs; keep it beside ESPKD. It is not resident yet,
so it cannot drive the screen reader until the next phase.

UNIVERSAL DOS TEST (DR-DOS, MS-DOS, FreeDOS): after GO and ESPK, type   DOSTEST
It records VER, MEM /C, your CONFIG.SYS, loads SBTALK high (LH), then starts the resident
eSpeak in a child shell and records MEM there, into A:\DOSTEST.TXT. Run it once per memory
setup you use. On DR-DOS 7.02 try both   DEVICE=EMM386.EXE   with and without   /DPMI=ON
(with it, ESPKD uses DR-DOS's own DPMI host instead of CWSDPMI, saving 43 KB); under
MS-DOS EMM386 or FreeDOS JEMM386 CWSDPMI is used. ESPKD needs about 2 MB of free extended
memory and refuses to start otherwise. The ESPKD child shell shows an [E speak] prompt; EXIT
unloads it. TASKMGR (DR-DOS): load Provox and SBTALK before TASKMGR (they are global TSRs);
do not run ESPKD under TASKMGR.

Optional second test (Provox screen reader, GPL):  after GO finishes, type   PROVOX
and press Enter (C:\SPEECH is the current directory). Three rising tones from the PC
speaker mean the TSR installed. Status goes to A:\PROVOX.TXT. Provox sends its speech
as text out COM2 at 9600 baud; a laptop on a null-modem cable can show it.
Provox's prefix key is '/'. Reboot to unload it.

Files: GO.BAT TALK.BAT PROVOX.BAT SBTALK.EXE SBTALKXT.EXE (XT build, 1983 voice only, 62 KB resident) SAY.EXE SBPLAY.EXE SAMSB.EXE CWSDPMI.EXE HELLO.WAV PROVOX7.EXE
(shipped 7.05) PROVOX7T.EXE (rebuilt from GPL source) PV7.EXE PROVOX7.DOC (manual).
SBPLAY/SAMSB are 32-bit DJGPP programs and need CWSDPMI.EXE next to them.
They honour the BLASTER variable (default A220 I5 D1).
