@echo off
rem runs inside DTALKD's child shell: Provox (C:\SPEECH is on the PATH) on the virtual COM3, then a shell
PROVOX7
PV7 LITETALK COM3
SAY DEC talk is ready. If Provox gave bad command, run GO from disk one first. Type EXIT to unload.
COMMAND
