# Modern DOS audio on a 2010-vintage Panasonic Toughbook: HDA, Sound Blaster emulation, networking, SSH

Status: web research notes, 2026-09-20. Question from the user: he owns a 2009-2011 Toughbook (4 GB,
Intel CPU, "has a floppy drive") and wants FreeDOS on it as an accessible terminal (screen reader +
software speech, network, ssh). He believes the built-in audio is "Creative Labs compatible with SB16
CSP capabilities and reserved wavetable memory". Conventions: **[verified: URL]** = read on that page
(or fetched with curl) for this report; **[recall]** = memory, not re-checked; **[unverified]** =
plausible but no source found; **[inference]** = my reasoning.

## 0. Short answer

The Sound Blaster belief is wrong. Every Toughbook of that generation has an Intel High Definition
Audio controller (PCI 00:1b.0) with a SigmaTel/IDT, Analog Devices or Realtek HDA codec, nothing at
port 220h, no DSP, no CSP/ASP, no wavetable RAM. Sound Blaster compatibility in DOS on this machine
comes from a software TSR: **SBEMU** (crazii, GPL-2.0, active in 2026) or its fork **VSBHDA**
(Japheth, GPL-2.0, v2.0 of 2026-09-04). Both drive the ICH8M/ICH9M/5-Series HDA controllers, both
emulate SB16 DSP incl. 8-bit auto-init DMA (what SBTALK/ESPKD would use), both need
JEMMEX+QPIEMU+HDPMI32i (or SBEMU's new VDPMI build, which needs neither). Laptop results are
"hit-and-miss" because of codec pin/amp configuration; ALC269-based Toughbooks (CF-31, CF-19 mk5,
CF-53) are the best bet, IDT/AD-codec ones (CF-30, CF-52 mk2) the riskiest. Networking: the
PCH-integrated Intel NIC is NOT covered by the FreeDOS E1000PKT packet driver; use Intel's NDIS2
E1000.DOS + DIS_PKT9 shim. SSH: SSH2DOS 0.2.1 needs legacy sshd options; the AnttiTakala fork
(dh-group14-sha256 / aes128-ctr / hmac-sha2-256) talks to a default modern server.

## 1. Which Toughbook is it? (4 GB, Intel, "floppy drive", 2009-2011)

Marks and platforms (Panasonic reuses a model number across several yearly "marks"):

| Model / mark | Year | CPU | Chipset (south bridge) | Max RAM | Notes |
|---|---|---|---|---|---|
| CF-30 mk1 (CF-30C) | 2007 | Core Duo L2400 | 945GM / ICH7-M | 4 GB | HDA ctrl "NM10/ICH7 Family", codec STAC9200 (8384:7690), NIC Marvell 88E8055 [verified: https://forums.linuxmint.com/viewtopic.php?t=274519, model CF-30CTQAZBM] |
| CF-30 mk2 | 2008 | Core 2 Duo L7500 | GM965 / ICH8-M | 4 GB DDR2 [verified: https://www.ruggedpcreview.com/3_notebooks_panasonic_cf30_012008.html] | codec STAC9200 [inference from mk1 and CF-74 STAC9200: https://bugs.launchpad.net/ubuntu/+source/alsa-driver/+bug/1225793] |
| CF-30 mk3 (CF-30K/L/P/Q) | Jan 2009 | Core 2 Duo SL9300 | GS45 (GMA 4500MHD) / ICH9-M | 4 GB [verified: https://help.toughoutlet.com/article/97-panasonic-toughbook-cf-30-cpu-processor and https://www.manua.ls/panasonic/toughbook-cf-30/specifications] | ICH9-M = HDA 8086:293e [recall]; codec [unverified], probably STAC/IDT 92HDxx |
| CF-19 mk3 | 2009 | Core 2 Duo SU9300 | GS45 / ICH9-M | 4 GB [recall] | mk1 used STAC9751 [verified: https://notebooktalk.net/topic/433-cf-19-model-guide/]; mk3 codec [unverified] |
| CF-19 mk4 | 2010 | Core i5-540UM | QM57 / HM55 (5 Series) [recall] | 4-8 GB [recall] | codec [unverified] |
| CF-19 mk5 (CF-19A) | 2011 | Core i5-2520M | QM67 (6 Series) | 8 GB [recall] | HDA "6 Series/C200" (8086:1c20), codec Realtek ALC269VB 10ec:0269 subsys 10f7:0400, NIC 82579LM, AMI BIOS V5.00L12 2011-06-16 [verified: https://forums.linuxmint.com/viewtopic.php?t=298815, model CF-19ADUC01M] |
| CF-31 mk1 | 2010 | Core i3-350M / i5-520M / i5-540M | Mobile Intel QM57 ("Calpella") [verified: https://en.wikipedia.org/wiki/Toughbook and https://help.toughoutlet.com/article/120-panasonic-toughbook-cf-31-cpu-processor] | 4-16 GB DDR3 (wiki; mk1 practically 8 GB) | HDA 8086:3b56 [recall]; codec Realtek ALC269 family [inference from mk3]; NIC 82577LM (8086:10ea) [recall] |
| CF-31 mk3 (CF-31SBL741M) | 2013 | Ivy Bridge | 7 Series (8086:1e20) | | ALC269VC 10ec:0269 subsys 10f7:0600, NIC 82579LM [verified: https://forums.linuxmint.com/viewtopic.php?t=317253] |
| CF-52 mk2 (CF-52G) | 2008-09 | Core 2 Duo | GM45 / ICH9-M | 4 GB [recall] | HDA 82801I (8086:293e), codec Analog Devices AD1883, NIC 82567LM (8086:10f5) [verified: https://forums.linuxmint.com/viewtopic.php?t=281740, model CF-52GCNBXAM] |
| CF-52 mk3 | 2010 | Core i5-540M | QM57 [inference] | 8 GB [recall] | [verified CPU: https://help.toughoutlet.com/article/165-panasonic-toughbook-cf-52-cpu-processor] |
| CF-52 mk4 | 2011 | Core i5-2540M | QM67 [inference] | | |
| CF-53 mk1 | 2011 | Core i3/i5-2520M | QM67 | 8 GB [verified: Wikipedia Toughbook] | later mk2 has ALC269 on Panther Point [verified: https://www.notebookcheck.com/Panasonic-Toughbook-CF-53JSWZGFG.86373.0.html] |
| CF-F9 | 2010 | Core i5-520M | QM57 | 4-8 GB | [recall] |
| CF-C1 | 2010 | Core i5-520M/460M | QM57/HM55 | 4-8 GB | [recall] |

Floppy drive: **no 2009-2011 Toughbook had an internal floppy option.** The media bay ("Multimedia
Pocket") on CF-30/CF-31 takes a DVD/combo drive or a second battery [verified: ruggedpcreview CF-30;
https://help.toughoutlet.com/article/132-panasonic-toughbook-cf-31-optical-drive]. Internal/bay
floppies existed only on older lines: CF-27/28/29 (bay FDD) and CF-51 (internal CF-K51FD002)
[verified: https://www.ebay.com/itm/283555915096 and https://esaitech.com/products/panasonic-cf-k51fd002-3-5-inch-internal-floppy-disk-drive-fdd-for-cf-51-toughbook].
For the 2009-2011 machines Panasonic's floppy is the **external USB CF-VFDU03U**, listed as a CF-30
accessory [verified: https://www.ocruggedlaptops.com/refurbished-toughbook-30 and
https://www.getthatpart.com/Panasonic-CF-VFDU03U-External-USB-Laptop-Floppy-Disk-Drive.html]. So
"4 GB + floppy + 2010" most likely means a **CF-30 mk3, CF-19 mk3/mk4, CF-52 mk2/mk3 or CF-31 mk1
with the USB floppy**; if the floppy is genuinely inside the bay, it is a CF-29 (max 1.5 GB) or CF-51
and the "4 GB" is wrong [inference]. All of these are legacy-BIOS machines (AMI, F2 for setup), so
FreeDOS boots without any CSM/UEFI fuss [verified BIOS type for CF-19 mk5; rest recall].

## 2. What the audio hardware actually is, and the "SB16 CSP" claim

- Every model above: Intel HDA controller in the south bridge at PCI 00:1b.0, one external codec on
  the HDA link. Controller PCI IDs, all in Intel vendor 8086 [verified by fetching
  https://admin.pci-ids.ucw.cz/read/PC/8086/<id>]:
  - 284b "82801H (ICH8 Family) HD Audio Controller" (GM965 era: CF-30 mk2, CF-52 mk1)
  - 293e "82801I (ICH9 Family) HD Audio Controller" (GS45/GM45 era: CF-30 mk3, CF-19 mk3, CF-52 mk2)
  - 3b56 / 3b57 "5 Series/3400 Series Chipset High Definition Audio" (QM57/HM55: CF-31 mk1, CF-19 mk4, CF-52 mk3, CF-F9, CF-C1)
  - 1c20 "6 Series/C200 Series Chipset Family High Definition Audio Controller" (QM67: CF-19 mk5, CF-52 mk4, CF-53 mk1)
  - 1e20 "7 Series/C216 Chipset Family High Definition Audio Controller" (later CF-31/CF-53 marks)
- Codecs seen on Toughbooks: SigmaTel STAC9200 (CF-30 mk1, CF-74), STAC9751 (CF-19 mk1), Analog
  Devices AD1883 (CF-52 mk2), Realtek ALC269VB/VC (CF-19 mk5, CF-31, CF-53, CF-C2) [verified per the
  Linux Mint threads cited in the table]. Panasonic's subsystem vendor is 10f7 (Matsushita).
- **No Sound Blaster register compatibility.** HDA is memory-mapped (BAR0, 16 KB) with a
  CORB/RIRB command ring and DMA stream engines; nothing decodes ISA I/O 220h-22Fh, 388h, DMA
  channel 1 or IRQ 5 on these laptops. Reads of 220h return the floating-bus value 0xFF [recall;
  inference]. A CSP/ASP existed only on ISA SB16 cards CT1740/CT1750/CT1770/CT1790/CT2230/CT2740/
  CT2950/CT2290 (a SGS-Thomson ST18932 DSP with 16 KB program / 8 KB data RAM) and "wavetable" meant
  the Wave Blaster daughterboard header [verified: https://en.wikipedia.org/wiki/Sound_Blaster_16].
  Neither concept exists in HDA.
- Where the belief probably came from: a DOS hardware-probe utility (NSSI/ASTRA/CheckIt-style
  "Sound Blaster 16 with CSP" detection) misreading 0xFF bus reads, or a probe run inside an emulator
  (DOSBox/86Box) that emulates an SB16 with ASP/CSP [unverified; inference]. Nothing else fits.
- PC speaker: the PIT channel 2 / port 61h beep still exists in the chipset. On Realtek codecs the
  PCBEEP input pin is routed to speaker and headphone by the hidden register (coeff 0x36 on NID 20h,
  reset value 0x3717 = beep on 1Ah amplified to HP and speaker) so beeps work before any OS driver
  loads [verified: https://www.kernel.org/doc/html/latest/sound/hd-audio/realtek-pc-beep.html]. On
  STAC9205/AD codecs the beep also passes through until an OS driver mutes it [verified for STAC9205:
  https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=460410]. Panasonic POST uses beep codes
  [verified for CF-18 service manual: https://www.manualslib.com/manual/467007/Panasonic-Toughbook-Cf-18nhhzxbm.html?page=9].
  So the SPEECH.COM/SAM one-bit voice will very likely work on the Toughbook as a fallback; a fixed
  BIOS routing to the codec is [unverified] per model.

## 3. Sound Blaster-compatible audio under FreeDOS on an HDA laptop (2026)

### 3.1 SBEMU (crazii) - https://github.com/crazii/SBEMU

- What it is: protected-mode TSR (DJGPP) that traps the SB I/O ports (220h, 388h, DMA 00h-0Fh/C0h-DFh
  and PIC) via a V86 monitor and a DPMI host with port trapping, and plays the result on a PCI/HDA
  device using MPXPlay's au_cards drivers [verified: https://raw.githubusercontent.com/crazii/SBEMU/main/README.txt].
- License GPL-2.0; repo pushed 2026-09-20; releases: 1.0.0-beta.5 (2024-08-18), beta.6rc
  (2026-05-16, integrates TinySoundFont from VSBHDASF, adds /VMPU /VMSF), beta.6rc2 (2026-06-04),
  **1.0.0-beta.6 (2026-06-16)**, and an experimental **vdpmi_pre_release (2026-06-16): "No HDPMI/JEMM
  dependency. Better compatibility. Support 16 bit DPMI clients"** - VDPMI is crazii's own DPMI server
  that now includes a V86 monitor [verified: GitHub API releases list, fetched with curl]. FreeDOS
  announced beta.6rc on 2026-05-24 [verified: https://sourceforge.net/p/freedos/news/2026/05/updated-sbemu/].
- Emulates: SB 1.0/2.0, SB Pro, SB Pro2 (with OPL3), SB16 (/T1-/T6); OPL3 FM (DOSBox code);
  MPU-401 UART with serial MIDI out, and a virtual MPU with SoundFont2 synth (/VMPU); 16-bit PCM via
  SB16 high DMA. Options: /A220 /I7 (5,7,9) /D1 (0,1,3) /H5 (5,6,7) /K22050 /VOL /FIXTC /O0|/O1
  /PM /RM /SC /SCL /SCFM /SCMPU /R (reset card) [verified: README.txt].
- **8-bit auto-init DMA: yes.** sbemu/sbemu.c implements SBEMU_CMD_8BIT_OUT_AUTO (1Ch),
  SBEMU_CMD_8BIT_OUT_AUTO_HS (90h), SBEMU_CMD_EXIT_8BIT_AUTO (DAh), SET_SIZE (48h), 2/3/4-bit ADPCM
  auto-init, SB16 C0h-CFh modes with PCM8/16 mono/stereo, SET_TIMECONST 40h, SET_SAMPLERATE 41h,
  HALT/CONTINUE DMA, TRIGGER_IRQ F2h, SPEAKER_ON/OFF D1h/D3h, GETVER E1h (DSP version depends on
  /T; internal default 0x0302) [verified: https://raw.githubusercontent.com/crazii/SBEMU/main/sbemu/sbemu.c].
  That is exactly the command set SBTALK/ESPKD's SB DMA output uses, so a real-mode synth TSR should
  run unmodified provided it gets its IRQ (see next point).
- Requirements [verified: README.txt]: DOS 6.22+/FreeDOS; a V86 monitor with port trapping: QEMM or
  **JEMM386/JEMMEX + QPIEMU.DLL** (EMM386 cannot trap ports below 100h, so it cannot trap the DMA
  controller); a DPMI host with port trapping: **HDPMI32i** (crazii's modified HX build). Canonical
  setup: `DEVICE=JEMMEX.EXE X2MAX=8192 NOEMS` in CONFIG.SYS; `JLOAD QPIEMU.DLL`, `HDPMI32i -r -x`,
  `SBEMU` in AUTOEXEC. README: "If you don't load JEMM+QPIEMU (or QEMM), only protected mode
  applications will be supported" - i.e. a real-mode TSR such as SBTALK needs JEMMEX+QPIEMU. VCPI
  clients (DOS32A etc.) bypass the trapping; `JEMMEX NOVCPI` after loading works around it.
- Memory: "SBEMU is a DPMI client and most memory used by SBEMU are DPMI memories located above 1M.
  There're some tiny pieces of DOS resident memory (below 1M) that SBEMU will try to load it to UMB
  automatically, Currently the PSP's still resident in low memory" [verified: README.txt Q2]. A user
  measured 580 KB conventional free after JEMMEX+QPIEMU+HDPMI32i+SBEMU, i.e. roughly 60 KB total
  cost incl. DOS [verified: https://github.com/crazii/SBEMU/issues/129, open, no maintainer reply].
  Per-component figures for HDPMI32i -r: [unverified] (japheth.de unreachable during this research).
- Hardware driven (from the repo's list) [verified: https://github.com/crazii/SBEMU]: **Intel HDA**,
  Intel ICH AC97 / nForce / SiS 7012, VIA VT82C686/VT8233/37 (VT8235 untested), SB Live!/Audigy,
  Audigy LS (CA0106), Ensoniq ES1371/1373, C-Media CMI8338/8738, AMD CS5535/36, ESS Maestro 3i
  (ES1983, tested) / Maestro 2/2E (untested); ported-from-Linux and less tested: X-Fi EMU20K1/2,
  Yamaha YMF7x4, ALS4000, CMI8788 Oxygen, ESS Allegro-1, Trident 4D Wave.
- ICH9M / 5-Series support: the HDA driver's PCI table contains 0x284b "Intel ICH8", 0x293e "Intel
  ICH9", 0x3b56 "Intel SCH (5 Series/3400)", 0x1c20 "Intel CPT6", 0x1e20 "Intel PCH (Panther
  Point)" plus many later ones [verified by grep of https://raw.githubusercontent.com/crazii/SBEMU/main/mpxplay/au_cards/sc_inthd.c].
  So the controller side of every Toughbook in the table is covered.
- Laptop/codec issues (the real risk): codec init is generic - find the AFG, walk output pins,
  prefer Line-Out, then HP, then Speaker, unmute everything at max [verified: sc_inthd.c summary].
  There is no per-model quirk table like ALSA's, no EAPD/GPIO amp-enable logic and no pin retasking.
  Consequences reported: Dell Latitude E6500 (**ICH9M 82801I + IDT 111D76B2**): card detected, "The
  volume is muted", only a click, no sound with /T1 /V9, unresolved [verified: https://github.com/crazii/SBEMU/issues/108];
  ICH7-M + AD1981HD laptop: click only, /O made no difference; AMD RS880M laptop: no sound
  [verified: https://www.vogons.org/viewtopic.php?t=93006&start=1140 and start=1060]; Vogons
  summarizes AC97/HDA laptop support as "very hit-and-miss" [verified: https://www.vogons.org/viewtopic.php?p=1255997].
  Working laptop reports are mostly AC97-era ThinkPads/Latitudes plus a Broadwell Dell with HDA
  [verified: https://www.vogons.org/viewtopic.php?t=96273]. README advice: "Try changing /O if SBEMU
  doesn't have any sounds for your HDA, or use /O0 if you're using a headphone" [verified: README.txt].
  Toughbook-specific worry: on Linux the CF-52 (AD1883) needed hdajackretask to declare pin 0x13 as
  the internal speaker, and the CF-19 mk5 (ALC269VB) needed pin 0x17 retasked to "Internal Speaker";
  the CF-31 (ALC269VC) speaker only plays when the *Headphone* mixer level is raised [verified: the
  three Linux Mint threads above]. SBEMU's "unmute everything" may get lucky on the ALC269 machines
  (headphone jack output is the safest test), and is likely to fail on the AD1883/STAC machines the
  same way issue #108 fails [inference]. VSBHDA adds /DEV (start index for HDA device scan) for
  multi-codec cases [verified: vsbhda.txt].
- SBEMU-X: the sbemu-x/sbemu-x repo (fork of thp/sbemu, cross-compile support) is an archived public
  repo whose README says its changes were merged upstream, "just use SBEMU instead" [verified:
  https://github.com/sbemu-x/sbemu-x]. SNDEMU (andersrodrig) is a mirror-style fork, no new hardware
  [verified: https://github.com/andersrodrig/SNDEMU]. Ready-made bootable FreeDOS 1.3 USB images with
  SBEMU preconfigured are attached to the UserBuild releases [verified: GitHub API].

### 3.2 VSBHDA (Japheth / Baron-von-Riedesel) - https://github.com/Baron-von-Riedesel/VSBHDA

- Fork of SBEMU rewritten for Open Watcom/JWasm, compatible with the *standard* HX runtime (HDPMI32i
  v3.21+, also a 16-bit HDPMI16i/VSBHDA16 variant), Jemm v5.84+ with QPIEMU.DLL for real-mode
  programs, optional JHDPMI.DLL, UNINST.EXE to unload; GPL-2.0 [verified: https://github.com/Baron-von-Riedesel/VSBHDA and https://raw.githubusercontent.com/Baron-von-Riedesel/VSBHDA/master/vsbhda.txt].
- Emulates SB 1.0/2.0/Pro/Pro2/16, OPL3, ADPCM, TinySoundFont MIDI; hardware: HDA, ICH/nForce/SiS
  AC97, VIA, SB Live/Audigy, ES1371/1373, CA0106; the VSBCMI fork adds CMI8338/8738 [verified: repo
  README]. Releases: v1.8 (2025-10-13: HDA fix for VMware, SB IRQ handler no longer shares badly
  with PCI IRQs, low-level tools pciirq/sblive/ichac97), v1.9 (2026-06-06: DSP cmd 0xFD, direct-DAC
  rework), **v2.0 (2026-09-04: hardware buffer default cut from 16/32 KB to 4 KB (/B), DSP play cmds
  now honour the DMA mask, better resampling, /BP buffer protection, pcisnd.exe)** [verified: GitHub
  API]. Options include /O (output connector, HDA & SB Live only), /DEV, /B, /BS, /SD, /CF, /LF log
  file [verified: vsbhda.txt]. FreeDOS's news calls it "Virtual SoundBlaster for HDA"
  [verified: https://sourceforge.net/p/freedos/news/2024/02/virtual-soundblaster-for-hda/].
- Notable for a synth TSR: vsbhda.txt section 4.3.2 warns that real-mode programs allocating an XMS
  block as a DMA buffer must land below 16 MB physical; XMSRes /H can push VSBHDA/HDPMI up to free
  low addresses [verified: vsbhda.txt]. SBTALK/ESPKD use conventional memory for their DMA buffer
  [recall from this project], which is always below 1 MB, so this does not bite.

### 3.3 Driving HDA directly from your own DJGPP program (MPXPlay au_cards / sc_inthd.c)

- The Intel HDA driver both SBEMU and MPXPlay use is `mpxplay/au_cards/sc_inthd.c` ("(C) 1998-2014
  PDSoft (Attila Padar)... based on ALSA and WSS libs") [verified: file header via curl].
  **License caveat:** the file header is *not* a GPL notice; it says "Please contact with the author
  (with me) if you want to use or modify this source." The SBEMU repository as a whole is tagged
  GPL-2.0 [verified: GitHub API], and MPXPlay's site only says "opensource" (v1.68 DOS, 2025-08-24,
  sources for DOS/Win32) without naming a license [verified: https://mpxplay.sourceforge.net/].
  Treat reuse as "ask Attila Padar" rather than "GPL and done" [inference].
- Mechanism in SBEMU mode (`#ifdef SBEMU`): AZX_PERIOD_SIZE = 512 bytes (4096 in MPXPlay mode);
  BDL entries are written with the IOC bit (`PDS_PUTB_LE32(&bdl[off+3],0x01)`), the stream's
  SD_CTL gets SD_INT_COMPLETE, INTCTL gets ICH6_INT_CTRL_EN|ICH6_INT_GLOBAL_EN, and
  `INTELHD_IRQRoutine()` acknowledges SD_STS/RIRBSTS; card_samples_per_int = 128. Position is also
  readable from SD_LPIB (`INTELHD_getbufpos`). In MPXPlay mode it polls LPIB instead [verified: grep
  of sc_inthd.c lines 37-43, 1146-1150, 1483-1486, 1562-1566, 1634-1698]. So the code already does
  interrupt-driven, per-period double buffering - exactly what a resident synth wants.
- Codec bring-up is generic (AFG search, DAC->mixer->pin paths, unmute at max, Line-Out > HP >
  Speaker priority); there is no EAPD/GPIO/vendor-coefficient handling, which is why some laptops stay
  silent. For a Toughbook-specific driver you could hard-code the pin(s) from the Linux threads:
  CF-52 AD1883 speaker on pin 0x13; CF-19 mk5 ALC269VB speaker on pin 0x17 (not 0x14); CF-31 ALC269VC
  speaker gated by the headphone amp path [verified: Linux Mint threads]. ALC269 also needs the
  Realtek "PC beep"/coef quirks only for beep, not for PCM [inference].
- Practicalities for DJGPP: BAR0 is a 16 KB MMIO region (map with DPMI 0800h), BDL and PCM buffers
  must be physically contiguous 128-byte-aligned memory (DOS conventional memory via DPMI 0100h is
  the easy way), and PCI bus-mastering must be enabled in the command register [recall]. For a TSR
  the HX/HDPMI resident model that SBEMU/VSBHDA use is the proven path; a plain CWSDPMI program cannot
  stay resident [recall].
- Other DOS HDA code: I found no separate "hdainit"/"PDSound"/"Judas" HDA driver in FreeDOS-related
  projects; the Judas sound system is AC97/SB-era [recall]. MPXPlay's is the only maintained DOS HDA
  driver and it keeps gaining PCI IDs (Intel, ATI, Zhaoxin, Loongson) [verified: https://mpxplay.sourceforge.net/whatsnew.txt via search summary].

### 3.4 Recommended stack for the speech use case

1. FreeDOS 1.3/1.4, `DEVICE=JEMMEX.EXE NOEMS X2MAX=8192` (or plain JEMMEX if EMS needed).
2. `JLOAD QPIEMU.DLL`, `HDPMI32i -r -x`, `SBEMU /T4 /A220 /I7 /D1 /K22050` (SB16 type; use /I5 if
   the synth TSR insists on IRQ 5; SBEMU sets BLASTER for you) - or VSBHDA v2.0 with the same
   BLASTER. Test with the headphone jack first (/O0), then /O1. Try the VDPMI build if JEMMEX
   conflicts with the screen reader's TSR hooks.
3. Then SBTALK/ESPKD (SB DMA mode, 8-bit auto-init) + Provox/ASAP/JAWS as in this project.
4. Fallback if the codec stays silent: the PC-speaker voices, or a USB/serial hardware synth.

## 4. DOS TTS / screen readers on HDA laptops - any reports?

- I found **no third-party report** of a DOS screen reader + software synthesizer running through
  SBEMU/VSBHDA on an HDA laptop. The only page pairing these terms is this project's own
  `ccdavis/vintage-pc-speech` (SBTALK as a virtual DoubleTalk on a COM port, SB DMA or PC speaker,
  SAM/Klatt/SPEECH.COM voices, ESPKD) [verified: https://github.com/ccdavis/vintage-pc-speech].
- The FreeDOS community's accessibility thread (Oct 2024) discusses Orca/espeakup under QEMU,
  DOSEMU2, aSAP, DECtalk, and Mateusz Viste's talking SvarDOS build, which "outputs speech commands to
  the computer's serial port" to a Braille 'n Speak - i.e. external hardware synth, not SBEMU
  [verified: https://www.mail-archive.com/freedos-user@lists.sourceforge.net/msg26440.html].
- Eric Auer's freedos-user post proposing SBEMU/SBEMU-X/VSBHDA for the distribution mentions games
  only [verified: https://sourceforge.net/p/freedos/mailman/message/57986443/].
- So the Toughbook would be a first-of-its-kind report; expect to debug codec pins yourself.

## 5. Networking and SSH (brief)

- NICs: CF-52 mk2 has Intel 82567LM (8086:10f5), the QM57-generation machines (CF-31 mk1, CF-52 mk3,
  CF-19 mk4, CF-F9, CF-C1) have 82577LM (8086:10ea), the QM67 generation has 82579LM (8086:1502)
  [verified IDs: https://admin.pci-ids.ucw.cz/read/PC/8086; CF-52 mk2/CF-19 mk5/CF-31 mk3 NICs from the
  Linux Mint threads; 82577LM on CF-31 mk1 is recall]. CF-30 mk1 has a Marvell 88E8055 [verified].
- **E1000PKT (FreeDOS package) does not cover them.** It is Intel's GPLv2 "Gigabit Packet Driver"
  GIGPKTDRVR.EXE, "specifically the Intel 82544, 82540, 82545, 82541, and 82547 based Ethernet
  controllers" [verified: https://raw.githubusercontent.com/ulrich-hansen/E1000PKT/master/README.md;
  https://gitlab.com/FreeDOS/net/e1000pkt]. The PCH-integrated 8256x/8257x/82579 family is a
  different (e1000e) register model.
- What does work: Intel's NDIS2 driver **E1000.DOS** from "Intel Ethernet Adapter Drivers for MS-DOS
  - FINAL RELEASE" v23.5.2 (2019-02-06), whose support list includes 82566DC/DM/MC/MM, 82567V/LM/LF,
  82577LC/LM, 82578DC/DM, 82579LM/V, I217/I218/I219 [verified: https://www.intel.com/content/www/us/en/download/2595/28919/intel-ethernet-adapter-drivers-for-ms-dos-final-release.html;
  a v24.3 copy exists on https://archive.org/details/PRODOS]. Load it under MS Client's
  PROTMAN.DOS/PROTMAN.EXE + NETBIND with the **DIS_PKT9.DOS** NDIS-to-packet-driver shim, then any
  packet-driver stack (mTCP, WATT-32) [verified: https://help.fdos.org/en/hhstndrd/network/ndis_ins.htm;
  https://ftp.icm.edu.pl/packages/novell/drivers/wfw.txt]. Intel forum: 82579LM DOS NDIS2 works for
  most, one "loads but no ping" case [verified: https://community.intel.com/t5/Ethernet-Products/82579LM-DOS-NDIS2-driver-does-not-work-no-ping-response/td-p/247017].
  A Marvell CF-30 mk1 needs Marvell's/SysKonnect's DOS driver instead [recall]. Wi-Fi: no WPA2 in
  DOS; use a bridge/router [verified: https://www.vogons.org/viewtopic.php?p=1306510].
- SSH client: **SSH2DOS v0.2.1** (SSH-2; SSHDOS v0.95 is SSH-1.5), based on PuTTY + WATT-32 + zlib +
  CVT100, needs a packet driver, includes SCP/SFTP, xterm/VT100 emulation [verified: https://sshdos.sourceforge.net/].
  Its algorithms are 2005-era: kex diffie-hellman-group1-sha1, ciphers 3des-cbc/blowfish-cbc/
  aes128-cbc, MAC hmac-sha1, host key ssh-dss. OpenSSH >= 6.7 disabled the kex/cipher, >= 6.9
  disabled ssh-dss, so a modern sshd needs
  `KexAlgorithms +diffie-hellman-group1-sha1` (users found the full list was needed),
  `Ciphers +3des-cbc,blowfish-cbc,aes128-cbc`, `HostKeyAlgorithms +ssh-dss`, `MACs +hmac-sha1`,
  then `ssh-keygen -A`, and the client flag `-g` before the username [verified: https://sourceforge.net/p/sshdos/discussion/45522/thread/8443ed77/;
  https://www.openssh.org/legacy.html]. OpenSSH 9.x/10.x also dropped the DSA host key code
  entirely (2025), so the ssh-dss route may be impossible on a current server [recall].
- Better: the **AnttiTakala/SSH2DOS fork, v0.2.1+SHA256.1 (2021-04-14)**, which swaps in
  diffie-hellman-group14-sha256, aes128-ctr and hmac-sha2-256 from PuTTY 0.70 code; all three are in
  a default OpenSSH server today. Caveats from its README: RNG "not cryptographically secure at all",
  use at your own risk [verified: https://github.com/AnttiTakala/SSH2DOS]. Host-key verification
  is still ssh-rsa (SHA-1 signature) or ssh-dss [inference]; OpenSSH 8.8+ disabled ssh-rsa signatures
  by default, so the server may still need `HostKeyAlgorithms +ssh-rsa` [recall, unverified against
  the fork's code]. Community writeups: https://justinmiller.io/posts/2020/07/15/legacy-dos-ssh-keys/
  and https://darrengoossens.wordpress.com/2020/12/31/ssh-from-freedos/ [verified exist via search].
- Alternative to SSH on the DOS side: a serial console (all these Toughbooks have a real RS-232 port
  [verified for CF-30: ruggedpcreview]) to a Linux box that does the SSH - which also sidesteps
  every cipher problem and pairs naturally with a serial hardware synth [inference].

## 6. Sources (primary)

- SBEMU README: https://raw.githubusercontent.com/crazii/SBEMU/main/README.txt
- SBEMU DSP emulation: https://raw.githubusercontent.com/crazii/SBEMU/main/sbemu/sbemu.c
- SBEMU HDA driver: https://raw.githubusercontent.com/crazii/SBEMU/main/mpxplay/au_cards/sc_inthd.c
- SBEMU releases (GitHub API): https://api.github.com/repos/crazii/SBEMU/releases
- SBEMU issues #108 (ICH9M laptop, no sound), #129 (conventional memory)
- VSBHDA: https://github.com/Baron-von-Riedesel/VSBHDA and vsbhda.txt
- Vogons compatibility list: https://www.vogons.org/viewtopic.php?t=96273
- Toughbook Linux threads: linuxmint.com t=274519 (CF-30), t=281740 (CF-52), t=298815 (CF-19), t=317253 and t=457569 (CF-31)
- pci-ids: https://admin.pci-ids.ucw.cz/read/PC/8086
- E1000PKT README; Intel DOS drivers final release page; sshdos.sourceforge.net; AnttiTakala/SSH2DOS

## Addendum: libau and the SBEMU load recipe

Added 2026-09-20 after a second research pass. Same tags as above. Sources fetched with curl unless
noted; line numbers refer to the `main` branch of crazii/SBEMU on 2026-09-20 and to maraakate/q2dos
`master` (pushed 2026-09-14).

### A1. What "libau" actually is

- **LIBAU is Ruslan Starodubov's standalone extraction of MPXPlay's `au_cards` drivers**, not a
  Japheth/HX component and not SBEMU's directory. Its readme: "This is a library of sound cards based
  on the modified mpxplay_source_code me (Ruslan Starodubov). The library can be compiled for
  different memory model (for WATCOMC and DJGPP). Use compile directive ZDM to disable
  __djgpp_nearptr_enable() (only for DJGPP). Maybe used in any DOS projects for sound in programs.
  All changes its code let me know. (starus2009@mail.ru)" [verified:
  https://raw.githubusercontent.com/maraakate/q2dos/master/dos/3rdparty/libau.src/src/readme].
  His site (sound-dos.ucoz.ru, entries 2014-2016) also offers a patched HX 2.17 whose dsound/winmm
  route to "Intel-HDA, ICH/AC97, VIA82xx, ENS1371/1373, CMI 8338/8738" [verified: WebFetch of
  http://sound-dos.ucoz.ru/]. Michael Kostylev's MPlayer-for-DOS uses it as `-ao au` and a user
  reported it drove HDA "internal speakers and the output jack" where WSS failed [verified:
  https://www.bttr-software.de/forum/mix_entry.php?id=16603].
- Where to get it: bundled in **Q2DOS** (maraakate/q2dos; neozeed's bitbucket is the older mirror)
  as `dos/3rdparty/libau.src/src/` (26 files: au.c, libau.h, libaudef.h, dpmi_c.c, mdma.c, pcibios.c,
  tim.c, ac97_def.c, sc_inthd.c/h, sc_ich.c, sc_via82.c, sc_e1371.c, sc_cmi.c, sc_sbliv.c/h,
  sc_sbl24.c/h, sc_sbxfi.c, emu10k1.h, Makefile.dj, Makefile.wat, cross_build.sh) plus prebuilt
  `dos/3rdparty/lib/libau.a` (75850 bytes) and `dos/3rdparty/lib_dxe/sndpci.dxe` (47428 bytes)
  [verified: GitHub trees API for maraakate/q2dos]. Q2DOS's readme credits "Mpxplay PCI Audio
  library" and "Mpxplay code importing by Ruslan Starodubov (http://sound-dos.ucoz.ru/)" and lists
  "Several PCI sound cards like AC'97 or HDA (run with -sndpci)" [verified: q2dos readme.txt].
- Cards (au.c `all_sndcard_info[]`): SB Live!/Audigy (SBLIVE), X-Fi EMU20Kx, ES1371/1373, CMI8338/
  8738, VIA82xx, Intel ICH AC97, **Intel HDA** (IHD) - seven drivers, no CS5535/YMF/Maestro/ALS
  [verified: au.c]. Its `sc_inthd.c` header is "(C) copyright 1998-2025 by PDSoft (Attila Padar)"
  (so it tracks a 2025 MPXPlay drop, newer than SBEMU's 1998-2015 copy) with the same "Please
  contact with the author (with me) if you want to use or modify this source" clause; libau itself
  ships **no license file** [verified: file headers, tree listing]. The only written permission on
  record is Padar's reply to crazii, kept as `mpxplay/au_cards/LICENSE` in the SBEMU repo:
  2023-02-26 "Keeping the header in the Mpxplay related files and keeping the project opensource, you
  can use that (audio driver) part of my sources. But I don't support (would like to see) Mpxplay
  clones", and 2023-12-17 "It's OK for the AU_CARDS directory of Mpxplay" to be GPL inside SBEMU
  [verified: https://raw.githubusercontent.com/crazii/SBEMU/main/mpxplay/au_cards/LICENSE]. Reading:
  reuse in an open-source FreeDOS synth that keeps the headers is within what Padar has already
  allowed; a closed binary would not be [inference].
- Build: `Makefile.dj` = `gcc -O2 -Wall -DHAVE_STDINT_H`, `ar rs libau.a`, and
  `dxe3gen -o sndpci.dxe -E _AU_ -U` (a DJGPP DXE3 plug-in exporting the `AU_*` symbols);
  `Makefile.wat` = `wcc386 -bt=DOS -zp=1 ... -dSDR`. So **yes, plain DJGPP links it** (Q2DOS is a
  DJGPP program) [verified: both makefiles].
- API (`libau.h`, complete): `au_context *AU_search(unsigned config)` (0 = line-out, 1 = "STEREO
  SPEAKER OUT (only Intel HDA chips)" - the same switch as SBEMU's /O); `const struct auinfo_s
  *AU_getinfo(ctx)` returning `{char infostr[96]; char *card_DMABUFF; unsigned long card_dmasize;
  unsigned bytespersample_card, freq_card, bits_set, chan_set;}`; `AU_setrate(ctx,&fr,&bt,&ch)`
  (in/out, the card may change them); `AU_setmixer_all(ctx, vol%)`; `AU_start/AU_stop/AU_close`;
  `unsigned AU_cardbuf_space(ctx)` (free bytes); `AU_writedata(ctx,pcm,len)` [verified: libau.h,
  au.c]. `AU_writedata` **busy-waits** (`do { space=AU_cardbuf_space(); ... } while(len)`), and
  `AU_cardbuf_space` gets its position from `cardbuf_pos` = `SD_LPIB` (sc_inthd.c line 1282).
  **There is no IRQ path at all in libau**: no `irq_routine`, IOC never set, period size 4096, ring =
  `AUCARDS_DMABUFSIZE_NORMAL 32768 * bytes-per-sample` (max 131072) [verified: au.c, mdma.c,
  libaudef.h lines 113-115, sc_inthd.c grep]. Q2DOS therefore hands `aui->card_DMABUFF` to Quake's
  mixer as `dma.buffer` and polls `AU_cardbuf_space` from the frame loop (`PCI_GetDMAPos`)
  [verified: https://raw.githubusercontent.com/maraakate/q2dos/master/dos/snd_pci.c].
- DJGPP glue (`dpmi_c.c`): DMA ring via `int 31h/0100h` (DOS memory, so physical = linear = seg*16),
  MMIO via `int 31h/0800h` + `__djgpp_conventional_base` with `__djgpp_nearptr_enable()`, or with
  `-DZDM` a separate LDT selector and `dosmemput/_farnspokeb` copies [verified: dpmi_c.c, mdma.c].
  Watcom's `-dSDR` drops `AU_cardbuf_space/AU_writedata` so the host does its own ring bookkeeping
  [verified: au.c `#ifndef SDR`].
- **Coexistence:** `snd_pci.c` has `sbemu_detect()` (scans INT 2Dh AMIS multiplex for the "SBEMU"
  signature) and refuses to start: "PCI Audio: refusing to work with SBEMU present" - two drivers
  cannot own one HDA controller [verified: snd_pci.c]. The same applies to our synth: libau path
  **or** SBEMU/VSBHDA, never both.

### A2. SBEMU's copy: `mpxplay/au_cards/`, its API, and how main.c drives it

- Directory (36 files): LICENSE, ac97_def.c/h, au_base.c/h (crazii's merged newfunc/DPMI helpers,
  per the LICENSE thread), au_cards.c/h, au_linux.c/h, dmairq.c/h, emu10k1.h, fw_ymf.h, ioport.c,
  pcibios.c/h, sc_allegro, sc_als4000, sc_cmi, sc_cs5535, sc_ctxfi, sc_e1371, sc_emu10k1x, sc_ich,
  sc_inthd.c/h, sc_null, sc_oxygen, sc_sbl24.c/h, sc_sbliv.c/h, sc_sbxfi, sc_trident, sc_via82,
  sc_ymf [verified: GitHub contents API]. Compiler: `CC := i586-pc-msdosdjgpp-gcc`, `-march=i386
  -O2 -flto -D__DOS__ -DSBEMU`; CI uses andrewwutw/build-djgpp v3.4 (GCC 12.2.0) [verified: makefile,
  .github/workflows/02-pr-checks.yml]. So the DJGPP build of au_cards is the production one.
- Driver vtable `one_sndcard_info` (au_cards.h 268-295): `card_config, card_init, card_detect,
  card_info, card_start, card_stop, card_close, card_setrate, cardbuf_writedata, cardbuf_pos,
  cardbuf_clear, cardbuf_int_monitor, irq_routine, card_writemixer, card_readmixer, card_mixerchans,
  card_fm_write/read, card_mpu401_write/read`. `IHD_sndcard_info` (sc_inthd.c 1680-1698) fills
  `INTELHD_adetect/card_info/start/stop/close/setrate, MDma_writedata, INTELHD_getbufpos,
  MDma_clearbuf, MDma_interrupt_monitor, INTELHD_IRQRoutine (#ifdef SBEMU, else NULL)` [verified].
- main.c sequence (line numbers): `AU_init(&aui,&fm_aui,&mpu401_aui)` 912 (autodetect; `/SC`,
  `/O` -> `aui.card_select_config` 911); `aui.card_irq` comes from PCI config INTLINE
  (sc_inthd.c 1483 `pcibios_ReadConfig_Byte(card->pci_dev, PCIR_INTR_LN)`), and if >15 (APIC)
  `pcibios_AssignIRQ` 922; `pcibios_enable_interrupt` 938; `PIC_MaskIRQ` 1130;
  `AU_ini_interrupts` 1131 (in SBEMU mode selects `aucards_writedata_nowait`, au_cards.c 588-590);
  `AU_setrate(&aui,&adi)` 1134 (`/K` rate; `aui.freq_card` valid afterwards); `AU_setmixer_init`,
  `AU_setmixer_outs(...,100)`, `AU_setmixer_one(AU_MIXCHAN_MASTER, /VOL)` 1135-1138;
  `DPMI_InstallISR(PIC_IRQ2VEC(aui.card_irq), MAIN_InterruptPM, ...)` 1142 - implemented in
  sbemu/dpmi/dpmi_dj2.c 430-483 with `_go32_dpmi_allocate_iret_wrapper` +
  `_go32_dpmi_set_protected_mode_interrupt_vector` (or `_chain_`), plus a real-mode ISR via
  `_go32_dpmi_allocate_real_mode_callback_iret` (`DPMI_InstallRealModeISR` 1145, dpmi_dj2.c 574-582);
  `HDPMIPT_InstallIRQRoutedHandler(aui.card_irq, ...)` 1180 and `HDPMIPT_LockIRQRouting` 1183 (crazii's
  HDPMI-fork API, hdpmipt.c); `PIC_UnmaskIRQ` 1184; `AU_prestart`/`AU_start` 1186-1187 [verified:
  main.c, dpmi_dj2.c]. So it is the very same DJGPP primitive our synth uses
  (`_go32_dpmi_set_protected_mode_interrupt_vector`), wrapped so the handler survives as a TSR.
- Feeding is **interrupt-driven, not polled**: `MAIN_InterruptPM` (1265) reads the PIC, calls
  `aui.card_handler->irq_routine(&aui)` to decide whether the IRQ is the card's, then
  `MAIN_Interrupt()` (1341): `samples = AU_cardbuf_space(&aui)/sizeof(int16_t)/SBEMU_CHANNELS` 1390,
  pulls the guest's samples from the virtual 8237 state (`VDMA_GetAddress/GetCounter/GetIndex`,
  1400-1410) i.e. straight from the client's DOS-memory DMA buffer, resamples, mixes OPL/VMPU/PC
  speaker, then `aui.samplenum=..; aui.pcm_sample=MAIN_PCM; AU_writedata(&aui)` 1640-1642, and sends
  EOI itself ("some platform (i.e. VirtualBox) don't send EOI on default handler in IVT") [verified].
  The comment block at 1258-1263 documents the PM/RM IRQ routing chain through the HDPMI fork.
- DMA buffer / physical address: `MDma_alloc_cardmem()` (dmairq.c 63) -> `pds_dpmi_dos_allocmem()`
  (au_base.c 122-140) -> `__dpmi_allocate_dos_memory((size+15)>>4, &sel)`; the block holds BDL +
  CORB + RIRB + PCM ring (`snd_ihd_buffer_init`, sc_inthd.c 953-975) and, being conventional
  memory, physical == linear so BDL entries are just linear addresses. BAR0 is mapped with
  `pds_dpmi_map_physical_memory(iobase,16384)` (sc_inthd.c 1472; au_base.c 158 via
  `__dpmi_physical_address_mapping`) [verified].
- **HDA driver interrupts:** in SBEMU mode `INTELHD_start` writes `INTCTL = CTRL_EN|GLOBAL_EN|
  (1<<is_count)` and `SD_CTL |= SD_INT_COMPLETE` (1565-1566), `AZX_PERIOD_SIZE` = 512 bytes so
  `card_samples_per_int = 128` stereo frames (5.8 ms at 22050 Hz) (41, 1486), and
  `INTELHD_IRQRoutine` (1635-1675) acks `SD_STS`, `RIRBSTS`, `STATESTS`, masks them with the enable
  bits so shared IRQs are not misattributed, and returns non-zero when the interrupt was ours. In
  MPXPlay mode (`#ifndef SBEMU`) IOC is never set, `irq_routine` is NULL and the player polls
  `SD_LPIB` via `INTELHD_getbufpos` (1602-1615) [verified]. There is no "buffer-complete callback"
  abstraction beyond that: you get the IRQ, call `AU_cardbuf_space`, write with `AU_writedata`.
- Conclusion for our synth [inference]: linking SBEMU's `au_cards` (or libau) directly is feasible
  in DJGPP and gives the "direct path to the Intel hardware" the user wants, with the same codec
  bring-up code (hence identical pin/amp risk to SBEMU). Costs: (1) we then own the controller, so
  SBEMU/VSBHDA cannot be loaded for games or for the SB-client version of the synth; (2) a
  *resident* DJGPP program keeping a PM IRQ handler alive needs a host that supports PM TSRs -
  SBEMU does it with the HDPMI fork's routing API plus self-relocation (`sbemu/dpmi/dpmi_tsr.c`);
  VDPMI documents the case explicitly: "/I16TO32 ... If you write a 32bit DPMI TSR (e.g DOS4GW or
  DJGPP) and installs IRQ handler in protected mode, this option will make the TSR handler work for
  16 bit programs" [verified: VDPMI.TXT]; CWSDPMI cannot do this [recall]; (3) polling-only libau
  would need our own IOC/IRQ additions, so start from SBEMU's `sc_inthd.c` rather than Q2DOS's copy.
  The cheaper route stays: keep the synth an SB client and let SBEMU own the HDA.

### A3. SBEMU load recipe, exactly

- **SBEMU.zip (Release_1.0.0-beta.6, 2026-06-16, 340926 bytes)** contains `SBEMU/HDPMI32i.EXE`
  (38646 B, crazii fork v0.1-beta4fix2 of 2024-03-01, strings show "v3.20 (c) japheth 1993-2021"),
  `JEMMEX.EXE` (32245 B, "JemmEx v5.84 [02/12/24]"), `JLOAD.EXE`, `QPIEMU.DLL` (3072 B),
  `README.txt`, `RELEASE_NOTES.md`, `sbemu.exe` (560640 B, go32 stub v2.05T) [verified: unzip -l,
  strings]. The build script pulls JemmB_v584.zip and crazii/HX v0.1-beta4fix2 HDPMI32i.zip and also
  produces the FreeDOS 1.4 Lite USB image [verified:
  https://raw.githubusercontent.com/crazii/SBEMU/main/scripts/build-release-artifacts.sh].
- **CONFIG.SYS / FDCONFIG.SYS** (README): `DEVICE=JEMMEX.EXE X2MAX=8192 NOEMS`. The USB image
  instead does `DEVICE=\JEMM\JEMMEX.EXE MAXEXT=2097152` then `DEVICE=\JEMM\JLOAD.EXE
  \JEMM\QPIEMU.DLL` in fdconfig.sys (QPIEMU as a device line works too) [verified: build script sed].
- **AUTOEXEC.BAT order** (README "Typical Setup"): `JLOAD.EXE QPIEMU.DLL` -> `HDPMI32i -r -x` ->
  `SBEMU`. The USB image's setup.bat is `LH \HDPMI\HDPMI32I.EXE` / `LH \SBEMU\SBEMU.EXE` /
  `LH \CTMOUSE\CTMOUSE.EXE` (note: no `-r`; HDPMI.TXT: without -r HDPMI "will install as a TSR as
  well, but will terminate when the next client has terminated" - use `-r`) [verified: README.txt,
  build script, https://raw.githubusercontent.com/Baron-von-Riedesel/HX/master/Src/HDPMI/HDPMI.TXT
  option list]. Optional afterwards: `JEMMEX NOVCPI` to force VCPI extenders (DOS32A, D3X) to use
  DPMI, `JEMMEX VCPI` to restore [verified: README 5a, issue #130 working config].
- **Options** (format `/OPT[VALUE]`, no space/colon; defaults from README.txt): `/A220`; `/I7`
  (5|7|9); `/D1` (0|1|3); `/H5` (5|6|7, SB16 only; /H0-3 forces /D=/H); `/T1..6` (1 SB1.x, 2 SBPro1,
  3 SB2.0, 4 SBPro2+OPL3, 5 SBPro2 MCA, 6 SB16; T5 is reported as T4 in BLASTER); `/OPL1`; `/PM1`;
  `/RM1` (auto-off with a warning if no QPIEMU/QEMM); `/O1` (HDA only: 0 headphone, 1 speaker);
  `/VOL80` (0-100 since beta.6, was 0-9); `/K22050`; `/FIXTC1` (threshold, x1000 Hz); `/SCL`, `/SC`,
  `/SCFM`, `/SCMPU`; `/R` reset card; `/VMPU[=32..256]`, `/VMSF=file.sf2`; `/P` MPU address and
  `/MCOM` serial MIDI appear in BLASTER/usage text. Re-running `SBEMU /opt` changes a live instance;
  SBEMU sets BLASTER, or reads a pre-set BLASTER as defaults [verified: README.txt].
- **What /PM means for a DPMI client like ours:** `/PM` installs the HDPMI32i port traps
  (`HDPMIPT_Install_IOPortTrap` on 220h-22Fh, DMA 00h-0Fh/C0h-DFh, and PIC 20h/21h/A0h/A1h on demand)
  so a protected-mode client's IN/OUT reach SBEMU; the virtual SB IRQ is raised with
  `MAIN_InvokeIRQ` -> `VIRQ_Invoke` through the HDPMI fork's IRQ-routing hooks into the client's PM
  IDT vector (main.c 540-568, 1153-1183) [verified]. **Auto-init DMA from a DOS-memory buffer works
  for PM clients** because the 8237 ports are trapped either by QPIEMU (V86) or HDPMI (PM), and
  SBEMU reads the samples from the physical address the client programmed (main.c 1400-1410)
  [verified]. `MAIN_TRAP_PMPIC_ONDEMAND 0` comment: "now we need a Virtual PIC to hide some IRQ for
  protected mode games (doom especially)" (main.c 37) [verified].
- **Which DPMI host the client must use:** only HDPMI32i is trapped. `HDPMIPT_Detect()` failing
  prints "HDPMI not installed, disabling protected mode support." (main.c 862-866) [verified]. A
  DJGPP `.exe`'s go32 stub looks for a resident DPMI host first and only loads `CWSDPMI.EXE` if none
  is found (stub strings "no DPMI - Get csdpmi*b.zip", "CWSDPMI.EXE") [verified: strings of
  sbemu.exe; stub behaviour recall]. With `HDPMI32i -r` resident, every later DJGPP program
  (including our synth and SBEMU itself) becomes an HDPMI client and CWSDPMI is never loaded.
  Vogons: "run HDPMI32I with the '-r' flag to make it resident, so that DPMI apps that get started
  will use HDPMI instead of their default extender" [verified: t=93006&start=1580 summary] and
  Japheth: "CWSDPMI and HDPMi32i both are DPMI hosts, that can only be used alternatively"
  (2023-07-14) [verified: t=93006&start=800]. VSBHDA says the same: "running VSBHDA under another
  DPMI host is possible, but not recommended" and its 32-bit build "can only support 32-bit
  protected-mode games" [verified: vsbhda.txt].
- **Known DJGPP/CWSDPMI-flavoured issues:** HDPMI32i `-a` (separate address spaces) stops sound
  (wierd_w 2023-07-14); ZSNES (DJGPP) crashed for one user, worked for another with `/T6 /I5 /RM0
  /K44100` under HDPMI32i and no EMM (2023-03-08/09) [verified: t=93006&start=60 and start=800];
  ArcadeOS (CWSDPMI) keyboard freeze traced to SBEMU's HDPMI32i fork, crazii's answer 2026-05-19:
  move to VDPMI [verified: https://github.com/crazii/SBEMU/issues/154]; DOS32A/D3X in VCPI mode
  silence fixed by `JEMMEX NOVCPI` [verified: issues #130, #88]; main.c 41 notes an HDPMI bug where
  memory a TSR allocates after load is treated as the primary client's (`MAIN_VMPU_HDPMI_MEMFIX`),
  which any DJGPP TSR that mallocs late may hit [verified]. Issue #40 explains why the fork exists:
  upstream HDPMI's port-trap API lacks virtual interrupts, so crazii wrote VDPMI instead [verified].
- **VDPMI build** (`SBEMU_VDPMI.zip`, vdpmi_pre_release 2026-06-16, 367652 B): `HimemX.exe`
  (2022-11-22), `vdpmi.exe` (229638 B), `sbemu.exe` (549376 B, adapted build), `VDPMI.TXT`,
  `README.txt`, `CONFIG.SYS` = `DEVICE=HIMEMX.EXE` / `DEVICE=VDPMI.EXE` / `DOS=HIGH,UMB` /
  `STACKS=9,256`; then run `sbemu` from AUTOEXEC. VDPMI is closed source for now ("planned to release
  under GPL"), built with Open Watcom v2, DPMI 0.9 host + V86 monitor with UMB/EMS, **no VDS, no
  VCPI**, 16- and 32-bit clients, Pentium/64 MB minimum; options `/DPMIMEM` (64 MB default),
  `/XMS2MEM`, `/EMS`, `/EMSX`, `/V1`, `/I16TO32`, `/PVI`, `/SAFE`, `/I`, `/X`, `/U` [verified:
  README.txt, VDPMI.TXT, CONFIG.SYS in the zip]. crazii on Vogons: "the QEMM and HDPMI dependency,
  are removed, Virtual PIC is also removed. because VDPMI has its own virtualization"; "VDS not
  planned"; XMS/XMM integration "not top priority" [verified: viewtopic.php?p=1421687].

### A4. QEMU and SBEMU's HDA driver

- Official line: `qemu-system-i386 -drive file=SBEMU-FD13-USB.img,format=raw -device AC97`, and "If
  you wish to test Intel HDA compatibility ... replace AC97 with intel-hda", with the warning "you
  shouldn't test emulators on other emulators" [verified:
  https://raw.githubusercontent.com/crazii/SBEMU/main/user_instructions.md]. That command line is
  incomplete for HDA: QEMU's `intel-hda`/`ich9-intel-hda` is only the controller; a codec
  (`-device hda-duplex` or `hda-output`) must be added or the bus is empty [verified: qemu
  hw/audio/hda-codec.c defines "hda-output"/"hda-duplex"; inference on the omission].
- IDs: QEMU `intel-hda` is ICH6 8086:2668, `ich9-intel-hda` (q35) is 8086:293e; both are in SBEMU's
  table as `AZX_DRIVER_ICH` (sc_inthd.c 1290, 1295) - the ICH9 entry is the very same ID as the
  Toughbook CF-30 mk3/CF-19 mk3/CF-52 mk2 controllers, so QEMU q35 is a fair rehearsal of the
  *controller* code, not of the codec [verified; inference]. QEMU's codec (vendor 1af4) advertises
  16-bit only at 16-96 kHz (`QEMU_HDA_PCM_FORMATS = AC_SUPPCM_BITS_16 | 0x1fc`) and a plain 0x4a-step
  amp, so SBEMU's 16-bit stereo output at /K22050 is within range [verified: hda-codec.c 122-125].
  QEMU's HDA supports IOC/LPIB and INTx; MSI stays off unless the driver enables it, which SBEMU
  does not [inference from intel-hda.c msi property + sc_inthd.c].
- Reports: PR #79 (volkertb, CI with KVM): a PM WAV player detected the emulated SB only after
  swapping DOS/32A for DOS/4GW 1.97; the test uses `-device AC97` (+ virtio-sound dump) and an HDA
  variant was "planned"; **bonki 2025-04-26**: booting the release USB image in QEMU gives "varying
  results (hardware not found, freeze, JemmEx crashes) ... playback never works. I have also tried
  intel-hda instead of AC97 in QEMU with the same results" (while `auplay` works on QEMU's `sb16`
  without SBEMU) [verified: https://github.com/crazii/SBEMU/pull/79 comments]. Issue #140:
  VirtualBox AC97 silent but "it does work if I set VirtualBox to Intel HD Audio" (2025-02-08);
  crazii 2026-06-05: "I tested ICH AC97 and Intel HDA both works in VirtualBox" [verified:
  https://github.com/crazii/SBEMU/issues/140]. VSBHDA v1.8 (2025-10-13) "fixed: HDA in VmWare"
  [verified: GitHub releases]. **No positive report of SBEMU + QEMU intel-hda/ich9-intel-hda was
  found**; VirtualBox HDA is the VM that is known to work. If trying anyway: `-machine q35 -device
  ich9-intel-hda -device hda-duplex` (or pc + `intel-hda`), watch `SBEMU /SCL` for the IRQ (>15 means
  APIC routing; SBEMU then calls `pcibios_AssignIRQ`), and expect the same IRQ/EOI quirks crazii
  patched for VirtualBox [inference].

### A5. Current versions (2026-09-20)

- SBEMU: `Release_1.0.0-beta.6` 2026-06-16 (SBEMU.zip) and `vdpmi_pre_release` 2026-06-16
  (SBEMU_VDPMI.zip); previous: beta.6rc2 2026-06-04, beta.6rc 2026-05-16, beta.5 2024-08-18 (last
  with a FD13 USB image) [verified: https://api.github.com/repos/crazii/SBEMU/releases]. Bundled:
  HDPMI32i fork v0.1-beta4fix2 (2024-03-01; crazii/HX has no newer tag), JEMMEX 5.84.
- VSBHDA: `v2.0` 2026-09-04 (vsbhda20.zip, 201187 B): VSBHDA.EXE v2.0, VSBHDA16.EXE, SNDCARD.DRV,
  **HDPMI32i/HDPMI16i v3.24 "(c) japheth 1993-2026"** (2026-05-29), QPIEMU.DLL (2025-09-23),
  JHDPMI.DLL, JHDPMIS.EXE, XMSRES.EXE, UNINST.EXE, SETPVI/RESPVI, hwinfo/{ICHAC97,PCIIRQ,SBLIVE,
  PCISND}.EXE, Win31/ files, START.BAT = `jload -q qpiemu.dll` / `jload -q jhdpmi.dll` /
  `lh hdpmi32i -x2` / `set blaster=A220 I5 D1 H5 T6 P330` / `vsbhda.exe` [verified: unzip -l,
  strings, START.BAT]. Jemm is not bundled: needs >= 5.84; upstream Jemm v5.86 (2026-01-25),
  v5.87pre1 (2026-05-26); upstream HX v2.23 (2025-10-13), v2.24pre1 (2026-05-31) [verified: GitHub
  releases APIs].
