@echo off
rem RESETVOI: forget the saved voice choice; the menu is asked again at the next boot.
if exist A:\VOICE.BAT del A:\VOICE.BAT
if exist A:\LATE.FLG del A:\LATE.FLG
A:\SAY The voice choice is cleared. Reboot to choose again.
