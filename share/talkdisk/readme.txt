ACCESSIBLE DOS TALKING DISK  (accessible_os / freedos, 2026)

A bootable FreeDOS 1.4 floppy that talks on any PC with a Sound Blaster compatible card.
At the first boot the disk speaks a menu on the PC speaker (works on any AT-class PC, no sound
card needed): press 1 SAM on Sound Blaster, 2 Klatt on Sound Blaster, 3 the 1983 voice on Sound
Blaster, 4 keep the PC speaker. The choice is saved in A:\VOICE.BAT; RESETVOI clears it.
FDAUTO.BAT then loads SBTALK (a resident software speech synthesizer that emulates a serial
DoubleTalk on COM3 and plays through the Sound Blaster), then the Provox 7 screen reader
(GPL, Kansys / Chuck Hallenbeck) set to that port. Everything on screen is spoken.

  HELP            spoken list of commands           SAY words       speak
  SAY /X          stop speech                       SAY /F FILE     read a file
  ESPK words      speak with the eSpeak NG voice (best quality; Sound Blaster; not resident yet)
  ESPK -k words   the same voice on the PC speaker (PIT-paced PWM, any PC)
  BLASTER.BAT     edit to match your card (default A220 I5 D1)
  PROVOX7.DOC is not on the disk for space reasons; see the project directory.

Provox prefix key is '/'. Provox and SBTALK are unloaded by rebooting.
Sources: https://github.com/ (accessible_os), engine SAM (1982, reverse-engineered), Provox 7.03.
