// PROVOX 7.x -- file pv.cpp
// Compile with Borland CPP 4.51, linking with DTC.LIB
const int sysnum = 241; // PROVOX system number
const int decnum = 208; // DecTalk system number
enum { pvx_hello, pvx_speak, pvx_quiet, pvx_init,
 pvx_i9data, pvx_i16data, pvx_i22data,
pvx_fetch, pvx_store_cx, pvx_store_cl, pvx_dump };
enum { vox_dtlt, vox_porttalk, vox_dtpc, vox_audapter, vox_pss,
 vox_bns, vox_accent, vox_dectalk, vox_litetalk, vox_brlmate,
 maxvoxnum };
#include <fstream.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <bios.h>
#include <dos.h>
#include <dir.h>
struct line { line *p, *n; char *t; };
struct config { int voxnumber, voxport;
unsigned char voxdrive, prefix, f9flag, f10flag, autoload, states0;
unsigned char macrobuf[780];
unsigned char bank0name[16], bank0sub[8];
unsigned char bank0flag[8], bank0table[60];
unsigned char bank1name[16], bank1sub[8];
unsigned char bank1flag[8], bank1table[60];
unsigned char bank2name[16], bank2sub[8];
unsigned char bank2flag[8], bank2table[60];
unsigned char bank3name[16], bank3sub[8];
unsigned char bank3flag[8], bank3table[60];
unsigned char bank4name[16], bank4sub[8];
unsigned char bank4flag[8], bank4table[60];
unsigned char bank5name[16], bank5sub[8];
unsigned char bank5flag[8], bank5table[60];
unsigned char bank6name[16], bank6sub[8];
unsigned char bank6flag[8], bank6table[60];
unsigned char bank7name[16], bank7sub[8];
unsigned char bank7flag[8], bank7table[60];
unsigned char bank8name[16], bank8sub[8];
unsigned char bank8flag[8], bank8table[60];
unsigned char bank9name[16], bank9sub[8];
unsigned char bank9flag[8], bank9table[60];
unsigned char vox1data[7], vox2data[3], vox3data[7];
unsigned char vox4data[4], vox5data[2], vox7data[4];
unsigned char vox8data[2], vox9data[7], vox10data[3];
unsigned char filler; }; // for even byte alignment
void no_provox (int, char **);
int identity (char *);
void cmd_save (char *, int);
void cmd_restore (char *, int);
void cmd_log (char *);
void cmd_help ();
void cmd_status (char *, int);
void set_device (int);
extern "C" { // DTPC library functions
unsigned short  _far DT_Init(void);
     char _far * _far DT_RomVer(void);
 unsigned short  _far DT_SerNum(void);
 unsigned short  _far DT_Sts(void);
           void  _far DT_Intgt(short _far *);
/*                               tts functions                             */
           void  _far DT_Say(char _far *, short bufctl);
     char _far * _far DT_Xlt(char _far *);
/*                               lpc functions                             */
           void  _far DT_Lpc(char _far *, short _far *,
                                   short speed, short bufctl);
           void  _far DT_LpcRpt(short speed, short bufctl);
/*                               pcm functions                             */
           void  _far DT_Pcm(char _far *, short _far *,
                                   short rate);
/*                               tgn functions                             */
           void  _far DT_Tgn(char _far *, short volume, short stopkey);
           void  _far DT_Tgn1(char _far *, short volume, short tempo); }
void main (int argc, char **argv)
{ int a, auto_init, b, length, voxnumber, voxport;
char rev1, rev2, file[MAXPATH];
_AX = sysnum << 8 | pvx_hello;
geninterrupt (47); // is Provox already loaded?
a=_AX; rev1=_BH; rev2=_BL; length=_CX;
if (a!=EOF) no_provox (argc, argv);
cout << " PV7/Provox 7." << rev2 << rev1 << '.' << endl;
if (argc==1) { cout << "Type \"PV7 help\" for help."; return; }
strcpy (file, *argv); cmd_log (file);
auto_init=0;
while (--argc) { a = identity (*++argv);
b = identity ("lpt1"); if (a>=b) { voxport = a-b;
_CX=voxport; _BX=2;
_AX = sysnum << 8 | pvx_store_cx; geninterrupt (47);
auto_init++; continue; }
b=identity ("dtlt"); if (a>=b) { set_device (a-b);
auto_init++; continue; }
if (!stricmp (*argv, "help")) { cmd_help (); continue; }
if (!stricmp (*argv, "speak"))
{ _AX = sysnum << 8 | pvx_speak; geninterrupt (47); continue; }
if (!stricmp (*argv, "quiet"))
{ _AX = sysnum << 8 | pvx_quiet; geninterrupt (47); continue; }
if (!stricmp (*argv, "init"))
{ _AX = sysnum << 8 | pvx_init; geninterrupt (47);
auto_init=0; continue; }
if (!stricmp (*argv, "save")) { cmd_save (file, length); continue; }
if (!stricmp (*argv, "status")) { cmd_status (file, length); continue; }
if (!stricmp (*argv, "restore"))
{ cmd_restore (file, length); auto_init++; continue; }
cout << "Unknown command \"" << *argv << "\"" << endl; }
if (auto_init) { _AX = sysnum << 8 | pvx_init; geninterrupt (47); } }
/***/int identity (char *s)
{ char *name[] = { "help", "quiet", "speak", "init",
"save", "status", "restore",
"dtlt", "porttalk", "dtpc", "audapter", "pss",
"bns", "accent", "dectalk", "litetalk", "brlmate",
"lpt1", "lpt2", "lpt3", "lpt4", "com1", "com2", "com3", "com4" };
int a, b = sizeof (name)/sizeof(char *);
for (a=0; a<b; a++) if (!stricmp (s, name[a])) return a+1;
return 0; }
/***/void no_provox (int argc, char *argv[])
{
while (--argc) cout << *++argv << endl;
exit (0); }
/***/void cmd_help ()
{ cout << endl << "Syntax: (DOS prompt) PV7 command command command ..."
 << endl << endl << "The available commands are:" << endl << endl;
cout << "HELP -- Displays this information summary." << endl
 << "INIT -- Initializes the selected speech device." << endl
 << "QUIET -- PROVOX7 becomes inactive until a SPEAK command." << endl
 << "RESTORE -- Old configuration data copied into PROVOX7." << endl
 << "SAVE -- Current configuration preserved for a later RESTORE." << endl
 << "SPEAK -- PROVOX7 becomes active after an earlier QUIET command." << endl
 << "STATUS -- Reports current configuration data." << endl
 << endl << "<synthesizer name> -- Name of your speech device." << endl
 << "<Output port> -- Name of the LPT or COM port used." << endl
 << "Be sure the output port follows the synthesizer name." << endl
 << "See the manual for supported device names and output ports." << endl
 << endl << "The file PROVOX7.DOC contains the complete manual." << endl
 << "Use any text reader to read this file."
 << endl << "Press a key to continue...";
bioskey (0); }
/***/void cmd_status (char *file, int l)
{ static union { config c; unsigned char b[sizeof(config)]; } u1;
static union { config c; unsigned char b[sizeof(config)]; } u2;
int a; unsigned char a1, a2, ch;
streampos c; ifstream fl;
char *syn[] = { "dtlt", "porttalk", "dtpc", "audapter", "pss",
"bns", "accent", "dectalk", "litetalk", "brlmate" };
for (a=0; a<l; a+=2)
{ _BX=a; _AX = sysnum<<8 | pvx_fetch; geninterrupt (47);
a1=_AL; a2=_AH; u1.b[a]=u2.b[a]=a1; u1.b[a+1]=u2.b[a+1]=a2; }
fl.open (file, ios::binary);
fl.seekg (2); fl.get (ch); a=ch; fl.get (ch); a+=(ch<<8);
fl.get (ch); c=ch; fl.get (ch); c+=(ch<<8)-1; c<<=9; c+=a;
fl.seekg (c);
fl.get(a1); fl.get(a2); a=a1+(a2<<8);
if (a==l) fl.read (u2.b, l);
cout << "Provox is installed as a ";
if (u1.c.states0&2) cout << "TSR"; else cout << "Device Driver";
cout << " and is using ";
if (u1.c.states0&1) cout << "Enhanced"; else cout << "Dual";
cout << " Mode." << endl;
cout << "Speech device " << syn[u1.c.voxnumber>>1] << " is ";
if (u1.c.voxdrive<4) cout << "on port LPT" << u1.c.voxdrive+1;
else if (u1.c.voxdrive<8) cout << "on port COM" << u1.c.voxdrive-3;
else cout << "at decimal address " << (u1.c.voxport/2)*2;
cout << ", speed " << int (u1.c.f9flag);
cout << ", volume " << int (u1.c.f10flag) << "." << endl;
cout << "The Prefix Key is '";
if (u1.c.prefix==27) cout << "ESC"; else cout << u1.c.prefix;
cout << "' and PrintScreen files are written to '";
cout << u1.c.voxdrive  << ":\\PV*.*'." << endl;
if (isdigit (u1.c.autoload)) cout << "ScreenBank " << u1.c.autoload;
else cout << "No ScreenBank"; cout << " is AutoLoaded." << endl;
 }
/***/void set_device (int device)
{ int a, voxport=4; // com1;
switch (device) {
case vox_dtlt: a = DT_Init();
if (a==1016) voxport=4; // com1
else if (a==760) voxport=5; // com2
else if (a==1000) voxport=6; // com3
else if (a==744) voxport=7; // com4
else { cout << "DTLT not found" << endl; return; } break;
case vox_porttalk: case vox_accent: voxport=2; break; // lpt3
case vox_dtpc: a = DT_Init();
if (a==606 || a==670 || a==734 || a==798 || a==862 || a==926)
{ voxport=a|1; break; }
cout << "DTPC not found" << endl; return;
case vox_dectalk: _AX = decnum << 8 | 0; geninterrupt (47);
a=_AL; if (a==255) { voxport=7; break; }
cout << "DecTalk not found" << endl; return;
case vox_litetalk: voxport=0; break; } // lpt1
_CX = device<<1; _BX = 0; 
_AX = sysnum<<8 | pvx_store_cx; geninterrupt (47);
_CX = voxport; _BX = 2;
_AX = sysnum<<8 | pvx_store_cx; geninterrupt (47); }
/***/void cmd_save (char *file, int l)
{ int a; unsigned char a1, a2, ch;
streampos c; fstream fl;
fl.open (file, ios::in | ios::out | ios::ate | ios::binary);
fl.seekg (2); fl.get (ch); a=ch; fl.get (ch); a+=(ch<<8);
fl.get (ch); c=ch; fl.get (ch); c+=(ch<<8)-1; c<<=9; c+=a;
fl.seekp (c);
cout << "Saving." << endl;
a1=l%256; a2=l>>8; fl.put (a1); fl.put (a2);
for (a=0; a<l; a+=2)
{ _BX=a; _AX = sysnum<<8 | pvx_fetch; geninterrupt (47);
a1=_AL; a2=_AH; fl.put (a1); fl.put (a2); }
fl.close(); }
/***/void cmd_restore (char *file, int l)
{ int a; unsigned char a1, a2, ch;
streampos c; ifstream fl;
fl.open (file, ios::binary);
fl.seekg (2); fl.get (ch); a=ch; fl.get (ch); a+=(ch<<8);
fl.get (ch); c=ch; fl.get (ch); c+=(ch<<8)-1; c<<=9; c+=a;
fl.seekg (c);
fl.get(a1); fl.get(a2); a=a1+(a2<<8);
if (fl.fail()) { cout << "Nothing to restore." << endl; return; }
if (a!=l) { cout << "Mismatched lengths." << endl; return; }
cout << "Restoring." << endl;
for (a=0; a<l; a+=2)
{ fl.get(a1); fl.get(a2);
if (fl.fail()) { cout << "Insufficient data." << endl; break; }
_CL=a1; _CH=a2; _BX=a;
_AX = sysnum<<8 | pvx_store_cx; geninterrupt (47); } }
/***/void cmd_log (char *fn1)
{ int a, b;
char *c, fn2[MAXPATH];
_AX = sysnum<<8 | pvx_fetch; _BX=0; geninterrupt (47);
a=_AX; if (a!=maxvoxnum<<1) return;
_AX = sysnum<<8 | pvx_fetch; _BX=8; geninterrupt (47);
a=_AX; if (a&(1<<9)) return; // tsr, not device driver
strcpy (fn2, fn1); c=strrchr (fn2, '.');
strcpy (c+1, "LOG");
ofstream fout (fn2); if (fout.fail())
{ cout << "Cannot create " << fn2 << endl; return; }
cout << "Creating " << fn2 << endl;
a=b=32; while (a!=EOF)
{ _AX = sysnum<<8 | pvx_dump; geninterrupt (47);
a=_AX; if (a!=EOF) { if (a==13) a=10; else if (a<32) a=32;
if (a==10 || a!=b || b!=32) { b=a; fout << char (a); } } } }


