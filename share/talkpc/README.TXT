TALKPC.IMG - a bootable FreeDOS floppy that talks, for 2005-2010 PCs

Write it to a 1.44 MB floppy (or dd it to a USB stick; most BIOSes boot it as a floppy) and
boot.  On the first boot a spoken menu tries the speaker output, then the headphone output,
then the PC speaker; press 1 when you hear the voice.  The answer is saved in A:\VOICE.BAT.
Then Provox (the screen reader) and DEC talk are resident and the prompt talks.  Type HELP.

What is on the disk: FreeDOS 1.4 kernel + COMMAND.COM, JEMMEX + QPIEMU + HDPMI32i (memory, port trapping, DPMI),
SBEMU (Sound Blaster emulation on Intel HD Audio / AC97 / PCI sound cards, github.com/crazii/SBEMU),
DTALKD (DECtalk built for DOS, a DoubleTalk-compatible synthesizer on COM3 via INT 14h),
PROVOX7 + PV7 (Provox 7.05 screen reader, GPL), SAY (speak from the command line), ASK.
