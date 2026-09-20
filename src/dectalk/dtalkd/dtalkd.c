/* DTALKD: resident eSpeak NG voice for DOS, a "shell wrapper" DPMI process (DJGPP + CWSDPMI).
 *
 *   DTALKD [COMn] [/SPK] [/TEST] [/11K] [/PMALL] [/C command...]
 *
 * It initialises eSpeak NG + klatt_fx, installs the outputs and the INT 14h DoubleTalk
 * emulation for COMn (default COM3), then spawns COMMAND.COM (COMSPEC).  Every program run
 * from that shell, Provox and JAWS included, sees a DoubleTalk on COMn and the speech comes
 * out on the Sound Blaster (auto-init DMA, refilled from the SB IRQ) or, with /SPK, on the PC
 * speaker (RTC IRQ 8 at 8192 Hz, PWM on PIT channel 2).  EXIT in the child uninstalls
 * everything.  The design is sbtalk/dos.c moved to protected mode:
 *
 *   - the synthesizer (espeak frontend + klatt_fx + synth.c/ring.c) is a coroutine on its
 *     own stack, resumed from the output interrupt with interrupts re-enabled, yielding when
 *     the sample ring is full or no text is pending (coro.S);
 *   - hardware IRQs are protected-mode handlers (_go32_dpmi_* iret wrappers); when the child
 *     shell (real mode) is running CWSDPMI reflects them through its own real-mode callbacks;
 *   - INT 14h: the real-mode vector points at a 256-byte real-mode stub in DOS memory
 *     (rmstub.S) which answers AH=0/2/3 (status, receive, init) for our port itself, chains
 *     other ports to the old vector, and far-jumps into a DPMI real-mode callback for AH=1
 *     (send byte), so only data bytes cost a mode switch;
 *   - /SPK: the same stub carries the IRQ 8 handler and an 8 KB PWM sample ring in DOS
 *     memory, so the 8192 Hz ticks that arrive in real mode never switch modes; ticks that
 *     arrive in protected mode go to an identical protected-mode handler; every 512 ticks
 *     (62.5 ms) the stub calls a real-mode callback that tops the ring up and resumes the
 *     synthesizer;
 *   - all memory is locked (_CRT0_FLAG_LOCK_MEMORY + explicit lock of the image), no DOS
 *     call and no malloc happens from interrupt context (malloc/realloc/free are wrapped:
 *     while on the synthesizer stack they come from a static arena and are counted).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dpmi.h>
#include <go32.h>
#include <pc.h>
#include <crt0.h>
#include <process.h>
#include <sys/farptr.h>
#include <sys/segments.h>
#include <sys/nearptr.h>          /* __djgpp_base_address */
#include <sys/exceptn.h>          /* __djgpp_set_ctrl_c */
#include <unistd.h>
#include "ring.h"
#include "synth.h"
#include "engine.h"
#include "rmstub_bin.h"

int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY | _CRT0_FLAG_NONMOVE_SBRK;
int _stklen = 65536;                       /* the foreground stack; the synthesizer has its own */

int engine_init_dectalk(int rate);
void engine_warmup(void);
extern unsigned long engine_utterances, engine_aborted, engine_utt_start, engine_resets;
extern volatile unsigned char synth_stage, synth_waiting;

struct ctx { unsigned esp, ss; };
extern void co_switch(struct ctx *save, unsigned new_ss, unsigned new_esp);
extern void isr_sb_wrapper(void), isr_rtc_wrapper(void);
extern unsigned short isr_sb_sel, isr_rtc_sel;
void sb_isr(void); void rtc_isr(void);

/* ---- real-mode stub layout (rmstub.S) ---- */
#define RM_PORT      0x00
#define RM_OLD14     0x02
#define RM_CB_PUT    0x06
#define RM_CB_WAKE   0x0A
#define RM_HEAD      0x0E
#define RM_TAIL      0x10
#define RM_WAKECTR   0x12
#define RM_WAKEPER   0x14
#define RM_IDLE      0x16
#define RM_FLAGS     0x17
#define RM_N14       0x18
#define RM_NIRQ8     0x1C
#define RM_NWAKE     0x20
#define RM_SIG       0x24
#define RM_INT14     0x40
#define RM_IRQ8      0x80
#define RM_RMWAIT    0xE0
#define RM_RING      0x100
#define RM_RING_SIZE 0x2000
#define RM_BLOCK     (RM_RING + RM_RING_SIZE)

/* ---- configuration ---- */
static unsigned port_index = 2;                      /* COM3 */
static int use_spk = 0, test_mode = 0, pm_all = 0, rate = 11025, nosti = 0, say_mode = 0, low_mem = 0;
static char say_text[512];
static int sb_base = 0x220, sb_irq = 5, sb_dma = 1, dsp_ver = 0;
#define HALF 512u                                    /* DMA half: 64 ms at 8 kHz */
#define BUF  (2 * HALF)
#define SPK_RATE 8192u
#define SPK_AHEAD 4096u                              /* keep at most 0.5 s queued in DOS memory (flush latency) */
#define WAKE_PERIOD 512u

/* ---- state ---- */
static int dos_sel = -1, dos_seg = -1;               /* stub + PWM ring block */
static unsigned long stub_lin;
static int dma_sel = -1;
static unsigned long dma_phys;
static struct ctx isr_ctx, synth_ctx;
static volatile unsigned char synth_busy = 0;
static volatile unsigned tick = 0, last_arrival_tick = 0;
#define SYNTH_STACK (160 * 1024)                    /* DECtalk: measured by /TEST */
static unsigned char synth_stack[SYNTH_STACK];
static _go32_dpmi_seginfo old_irq, new_irq, old_rtc, new_rtc, cb_put, cb_wake, old14_rm, new14_rm, old70_rm;
static __dpmi_regs put_regs, wake_regs;
static int irq_vec, rtc_vec, hooked_irq = 0, hooked_rtc = 0, hooked14 = 0, hooked70 = 0;
static unsigned char pwm_lut[256], rtc_a, rtc_b, port61;
static unsigned char imr_saved_m, imr_saved_s, imr_bits_m = 0, imr_bits_s = 0;   /* PIC mask bits we cleared, and their old state */
static int bda_set = 0;                              /* we wrote the COM port address into the BIOS data area */
static unsigned long spk_pos_q16, spk_step_q16;
static unsigned pm_wake_ctr = WAKE_PERIOD;
/* diagnostics */
static volatile unsigned long n_sb_irq = 0, n_rtc_pm = 0, n_wake_pm = 0, n_wake_rm = 0, n_resume = 0, n_int14_pm = 0, n_underrun = 0, n_flush = 0;
static unsigned long guard_allocs = 0;

/* ---- malloc guard: nothing may reach DJGPP's heap from the synthesizer (interrupt context) ---- */
void *__real_malloc(size_t); void *__real_realloc(void *, size_t); void *__real_calloc(size_t, size_t); void __real_free(void *);
static unsigned char arena[16384];                   /* never used in practice (0 allocations reported) */
static unsigned arena_used = 0;
static int on_synth_stack(void)
{
    unsigned esp;
    __asm__ __volatile__("movl %%esp, %0" : "=r"(esp));
    return esp >= (unsigned)synth_stack && esp < (unsigned)synth_stack + SYNTH_STACK;
}
static void *arena_alloc(size_t n)
{
    void *p;
    n = (n + 15) & ~(size_t)15;
    if (arena_used + n > sizeof arena) return NULL;
    p = arena + arena_used; arena_used += n;
    return p;
}
void *__wrap_malloc(size_t n) { if (on_synth_stack()) { guard_allocs++; return arena_alloc(n); } return __real_malloc(n); }
void *__wrap_calloc(size_t a, size_t b)
{
    if (on_synth_stack()) { void *p = arena_alloc(a * b); guard_allocs++; if (p) memset(p, 0, a * b); return p; }
    return __real_calloc(a, b);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (on_synth_stack()) { void *q = arena_alloc(n); guard_allocs++; if (q && p) memcpy(q, p, n); return q; }
    return __real_realloc(p, n);
}
void __wrap_free(void *p)
{
    if (on_synth_stack()) { guard_allocs++; return; }
    if ((unsigned char *)p >= arena && (unsigned char *)p < arena + sizeof arena) return;
    __real_free(p);
}

/* ---- DSP ---- */
static int dsp_write(int v) { long t; for (t = 0; t < 100000; t++) if (!(inportb(sb_base + 0xC) & 0x80)) { outportb(sb_base + 0xC, v); return 0; } return -1; }
static int dsp_read(void)   { long t; for (t = 0; t < 100000; t++) if (inportb(sb_base + 0xE) & 0x80) return inportb(sb_base + 0xA); return -1; }
static int dsp_reset(void)  { int i; outportb(sb_base + 6, 1); for (i = 0; i < 20; i++) inportb(sb_base + 0xE); outportb(sb_base + 6, 0); return dsp_read() == 0xAA ? 0 : -1; }

/* ---- DMA (8237, 8-bit channel) ---- */
static const int dma_addr[4] = {0, 2, 4, 6}, dma_cnt[4] = {1, 3, 5, 7}, dma_page[4] = {0x87, 0x83, 0x81, 0x82};
static void dma_start(void)
{
    outportb(0x0A, 4 | sb_dma);
    outportb(0x0C, 0);
    outportb(0x0B, 0x58 | sb_dma);                   /* auto-init, increment, read (mem -> device) */
    outportb(dma_addr[sb_dma], (int)(dma_phys & 0xFF)); outportb(dma_addr[sb_dma], (int)((dma_phys >> 8) & 0xFF));
    outportb(dma_page[sb_dma], (int)((dma_phys >> 16) & 0xFF));
    outportb(0x0C, 0);
    outportb(dma_cnt[sb_dma], (BUF - 1) & 0xFF); outportb(dma_cnt[sb_dma], (BUF - 1) >> 8);
    outportb(0x0A, sb_dma);
}
static unsigned dma_pos(void)
{
    unsigned lo, hi;
    outportb(0x0C, 0); lo = inportb(dma_addr[sb_dma]); hi = inportb(dma_addr[sb_dma]);
    return (unsigned)((((unsigned long)hi << 8 | lo) - (dma_phys & 0xFFFF)) & 0xFFFF);
}
static void mixer_set(int reg, int val) { outportb(sb_base + 4, reg); outportb(sb_base + 5, val); }
static void dsp_start(void)
{
    dsp_write(0xD1);                                 /* speaker on (pre-SB16 needs it) */
    if (dsp_ver >= 0x400) {                          /* SB16 mixer: master and voice (DAC) volume to maximum; emulators reset these */
        mixer_set(0x30, 0xF8); mixer_set(0x31, 0xF8); mixer_set(0x32, 0xF8); mixer_set(0x33, 0xF8);
    } else if (dsp_ver >= 0x300) {
        mixer_set(0x22, 0xEE); mixer_set(0x04, 0xEE);   /* SB Pro master / voice */
    }
    if (dsp_ver >= 0x400) {
        dsp_write(0x41); dsp_write(rate >> 8); dsp_write(rate & 0xFF);
        dsp_write(0xC6); dsp_write(0x00);            /* 8-bit auto-init output, mono unsigned */
        dsp_write((HALF - 1) & 0xFF); dsp_write((HALF - 1) >> 8);
    } else {
        dsp_write(0x40); dsp_write(256 - (int)(1000000UL / rate));
        dsp_write(0x48); dsp_write((HALF - 1) & 0xFF); dsp_write((HALF - 1) >> 8);
        dsp_write(0x1C);                             /* 8-bit auto-init DMA output */
    }
}

/* ---- the coroutine ---- */
/* Interrupts are enabled only while on the synthesizer stack: the ISR / callback side switches
 * with IF = 0 (its stack is the DPMI host's 4 KB locked stack or the callback wrapper's stack),
 * synth_yield disables them before switching back and re-enables them when resumed. */
static void resume_synth(void)                       /* interrupt context, IF = 0 */
{
    if (synth_busy) return;
    synth_busy = 1; n_resume++;
    co_switch(&isr_ctx, synth_ctx.ss, synth_ctx.esp);
    synth_busy = 0;
}
void synth_yield(void)
{
    __asm__ __volatile__("cli");
    co_switch(&synth_ctx, isr_ctx.ss, isr_ctx.esp);
    if (!nosti) __asm__ __volatile__("sti");
}
int  synth_idle_ticks(void) { return (int)(tick - last_arrival_tick); }
void synth_on_abort(void) { }                        /* the abort unwinds through wgout_abort, no longjmp */
static void synth_entry(void) { if (!nosti) __asm__ __volatile__("sti"); synth_main(); for (;;) synth_yield(); }

/* ---- Sound Blaster interrupt: refill the half that just finished, then let the synth run ---- */
static void fill_half(unsigned half)
{
    unsigned long d = dma_phys + half * HALF, avail = ring_committed - ring_consumed;
    unsigned got = avail > HALF ? HALF : (unsigned)avail, k;
    _farsetsel(_dos_ds);
    for (k = 0; k < got; k++) _farnspokeb(d + k, vol_lut[ring[(unsigned)(ring_consumed + k) & RING_MASK]]);
    for (; k < HALF; k++) _farnspokeb(d + k, 128);
    if (got < HALF && synth_stage == 1 && !ring_abort && ring_committed > engine_utt_start) n_underrun++;   /* starved mid-utterance (not a flush; includes partial first fills) */
    ring_consumed += got;
}
void sb_isr(void)
{
    unsigned pos;
    if (dsp_ver >= 0x400) { outportb(sb_base + 4, 0x82); inportb(sb_base + 5); }
    inportb(sb_base + 0xE);                          /* acknowledge the 8-bit DMA transfer */
    tick++; n_sb_irq++;
    pos = dma_pos(); fill_half(pos >= HALF ? 0 : 1);
    if (sb_irq >= 8) outportb(0xA0, 0x20);
    outportb(0x20, 0x20);
    resume_synth();
}

/* ---- PC speaker: PWM ring in DOS memory fed from the sample ring ---- */
static void spk_fill(void)
{
    unsigned head, tail;
    unsigned char v;
    _farsetsel(_dos_ds);
    head = _farnspeekw(stub_lin + RM_HEAD); tail = _farnspeekw(stub_lin + RM_TAIL);
    while ((unsigned short)(head - tail) < SPK_AHEAD && ring_committed > ring_consumed) {
        v = vol_lut[ring[(unsigned)ring_consumed & RING_MASK]];
        _farnspokeb(stub_lin + RM_RING + (head & (RM_RING_SIZE - 1)), pwm_lut[v]);
        head++;
        spk_pos_q16 += spk_step_q16;                 /* 8000 Hz engine samples stepped to 8192 Hz */
        while (spk_pos_q16 >= 0x10000UL && ring_committed > ring_consumed) { spk_pos_q16 -= 0x10000UL; ring_consumed++; }
    }
    _farnspokew(stub_lin + RM_HEAD, head);
}
static void spk_drop(void)                           /* flush: drop what is queued in DOS memory too */
{
    _farsetsel(_dos_ds);
    _farnspokew(stub_lin + RM_HEAD, _farnspeekw(stub_lin + RM_TAIL));
}
void rtc_isr(void)                                   /* an RTC tick that arrived in protected mode */
{
    unsigned head, tail;
    unsigned char v;
    _farsetsel(_dos_ds);
    tail = _farnspeekw(stub_lin + RM_TAIL); head = _farnspeekw(stub_lin + RM_HEAD);
    if (tail != head) { v = _farnspeekb(stub_lin + RM_RING + (tail & (RM_RING_SIZE - 1))); _farnspokew(stub_lin + RM_TAIL, tail + 1); }
    else v = pwm_lut[128];
    outportb(0x42, v);
    outportb(0x70, 0x0C); inportb(0x71);
    outportb(0xA0, 0x20); outportb(0x20, 0x20);
    n_rtc_pm++;
    if (--pm_wake_ctr == 0) { pm_wake_ctr = WAKE_PERIOD; n_wake_pm++; tick++; spk_fill(); resume_synth(); }
}
static void wake_cb(__dpmi_regs *r)                  /* called by the real-mode stub every 512 ticks */
{
    (void)r;
    n_wake_rm++; tick++;
    spk_fill();
    resume_synth();
}

/* ---- INT 14h: the virtual serial port (AH=1 arrives here; the stub answers the rest) ---- */
static void int14_cb(__dpmi_regs *r)
{
    n_int14_pm++;
    switch (r->h.ah) {
    case 1:
        if (r->h.al == 24) { text_clear(); ring_flush(); n_flush++; if (use_spk) spk_drop(); }   /* ^X: stop now */
        text_put(r->h.al);
        last_arrival_tick = tick;
        r->h.ah = 0x60; break;
    case 0: case 3: r->h.ah = 0x60; r->h.al = 0xB0; break;      /* THRE+TEMT; CTS+DSR+DCD */
    case 2: r->h.ah = 0x80; break;
    default: r->h.ah = 0x60; break;
    }
}

/* ---- helpers ---- */
static unsigned long bios_ticks(void) { return _farpeekl(_dos_ds, 0x46C); }
static void parse_blaster(void)
{
    char *e = getenv("BLASTER"), *p;
    if (!e) return;
    for (p = e; *p; p++) {
        if (*p == 'A') sb_base = (int)strtol(p + 1, NULL, 16);
        else if (*p == 'I') sb_irq = atoi(p + 1);
        else if (*p == 'D') sb_dma = atoi(p + 1);
    }
}
static unsigned dos_free_kb(void)
{
    __dpmi_regs r; memset(&r, 0, sizeof r);
    r.h.ah = 0x48; r.x.bx = 0xFFFF; __dpmi_int(0x21, &r);
    return (unsigned)(((unsigned long)r.x.bx * 16) / 1024);
}
/* Lock everything the interrupt-time code can touch: the image (text, data, bss: ring, stacks,
 * arena) and every heap block DJGPP's non-moving sbrk allocated during init (data pack,
 * dictionary, translator, callback wrapper stacks): under CWSDPMI those blocks are contiguous
 * after the image, so one region from the first mapped page to the current break covers them.
 * CWSDPMI aborts the program on a page fault inside an RMCB or hardware interrupt, so a failed
 * lock is fatal here rather than a warning.  (CWSDPR0 never pages; its lock call is a no-op.) */
static int lock_all(void)
{
    __dpmi_meminfo m;
    unsigned long brk = (unsigned long)sbrk(0);
    m.address = __djgpp_base_address + 4096;                     /* page 0 is DJGPP's unmapped null page */
    m.size = ((brk + 4095) & ~4095UL) - 4096;
    if (__dpmi_lock_linear_region(&m)) {
        printf("DTALKD: cannot lock %lu KB of memory (DPMI error %x): not enough free extended memory,\n"
               "       or a DPMI host that cannot lock; DTALKD needs about 1.5 MB of XMS.\n", m.size / 1024, __dpmi_error);
        return -1;
    }
    return 0;
}
static unsigned synth_stack_used(void)
{
    unsigned i;
    for (i = 0; i < SYNTH_STACK; i++) if (synth_stack[i] != 0xEE) break;
    return SYNTH_STACK - i;
}

/* ---- install / uninstall ---- */
static int install(void)
{
    __dpmi_version_ret ver;
    unsigned *stk, i;
    __dpmi_get_version(&ver);
    irq_vec = sb_irq < 8 ? ver.master_pic + sb_irq : ver.slave_pic + sb_irq - 8;
    rtc_vec = ver.slave_pic;

    /* DOS block: real-mode stub + PWM ring. Prefer an upper memory block when a UMB provider is present
     * (EMM386/JEMM386 on MS-DOS, DR-DOS, FreeDOS): DOS allocation strategy "high first" with UMBs linked. */
    { __dpmi_regs r; int strat = -1, link = -1;
      memset(&r, 0, sizeof r); r.x.ax = 0x5800; __dpmi_int(0x21, &r); if (!(r.x.flags & 1)) strat = r.x.ax;
      memset(&r, 0, sizeof r); r.x.ax = 0x5802; __dpmi_int(0x21, &r); if (!(r.x.flags & 1)) link = r.h.al;
      if (!low_mem) {
      memset(&r, 0, sizeof r); r.x.ax = 0x5803; r.x.bx = 1; __dpmi_int(0x21, &r);
      memset(&r, 0, sizeof r); r.x.ax = 0x5801; r.x.bx = 0x80; __dpmi_int(0x21, &r);
      }
      dos_seg = __dpmi_allocate_dos_memory(((use_spk ? RM_BLOCK : RM_RING) + 15) / 16, &dos_sel);   /* the 8 KB PWM ring only with /SPK */
      if (strat >= 0) { memset(&r, 0, sizeof r); r.x.ax = 0x5801; r.x.bx = strat; __dpmi_int(0x21, &r); }
      if (link >= 0) { memset(&r, 0, sizeof r); r.x.ax = 0x5803; r.x.bx = link; __dpmi_int(0x21, &r); }
    }
    if (dos_seg < 0) { printf("DTALKD: no DOS memory for the stub\n"); return -1; }
    stub_lin = (unsigned long)dos_seg << 4;
    dosmemput(rmstub_bin, sizeof rmstub_bin, stub_lin);
    _farsetsel(_dos_ds);
    _farnspokew(stub_lin + RM_PORT, port_index);
    _farnspokeb(stub_lin + RM_FLAGS, pm_all ? 1 : 0);
    _farnspokeb(stub_lin + RM_IDLE, pwm_lut[128]);
    _farnspokew(stub_lin + RM_WAKECTR, WAKE_PERIOD); _farnspokew(stub_lin + RM_WAKEPER, WAKE_PERIOD);
    /* old INT 14h (already ours? then refuse) */
    _go32_dpmi_get_real_mode_interrupt_vector(0x14, &old14_rm);
    if (_farpeekl(_dos_ds, ((unsigned long)old14_rm.rm_segment << 4) + RM_SIG) == 0x4B4C5444UL /* "DTLK" */) {
        printf("DTALKD: already installed (INT 14h points at an DTALKD stub); type EXIT first\n"); return -1;
    }
    _farnspokew(stub_lin + RM_OLD14, old14_rm.rm_offset); _farnspokew(stub_lin + RM_OLD14 + 2, old14_rm.rm_segment);
    /* real-mode callbacks (DJGPP gives each wrapper a 31.5 KB locked stack by default; ours run with
     * IF = 0 and hand the real work to the synthesizer stack, so a small one is plenty) */
    _go32_rmcb_stack_size = 8192;
    cb_put.pm_offset = (unsigned long)int14_cb; cb_put.pm_selector = _my_cs();
    if (_go32_dpmi_allocate_real_mode_callback_iret(&cb_put, &put_regs)) { printf("DTALKD: no real-mode callback\n"); return -1; }
    _farnspokew(stub_lin + RM_CB_PUT, cb_put.rm_offset); _farnspokew(stub_lin + RM_CB_PUT + 2, cb_put.rm_segment);
    if (use_spk) {
        cb_wake.pm_offset = (unsigned long)wake_cb; cb_wake.pm_selector = _my_cs();
        if (_go32_dpmi_allocate_real_mode_callback_iret(&cb_wake, &wake_regs)) { printf("DTALKD: no real-mode callback\n"); return -1; }
        _farnspokew(stub_lin + RM_CB_WAKE, cb_wake.rm_offset); _farnspokew(stub_lin + RM_CB_WAKE + 2, cb_wake.rm_segment);
    }
    /* BIOS data area: make the COM port look present */
    if (_farpeekw(_dos_ds, 0x400 + port_index * 2) == 0) { _farpokew(_dos_ds, 0x400 + port_index * 2, 0x3E8); bda_set = 1; }
    /* synthesizer stack: pop edi/esi/ebx/ebp then "ret" into synth_entry */
    memset(synth_stack, 0xEE, SYNTH_STACK);
    stk = (unsigned *)(synth_stack + SYNTH_STACK - 32);
    stk[0] = stk[1] = stk[2] = stk[3] = 0; stk[4] = (unsigned)synth_entry; stk[5] = 0;
    synth_ctx.ss = _my_ds(); synth_ctx.esp = (unsigned)stk;

    if (!use_spk) {
        if (dsp_reset()) { printf("DTALKD: no Sound Blaster DSP at %Xh\n", sb_base); return -1; }
        dsp_write(0xE1); dsp_ver = dsp_read() << 8; dsp_ver |= dsp_read();
        {   /* DMA buffer: 2 * BUF of DOS memory, pick a block not crossing a 64 KB page */
            int seg = __dpmi_allocate_dos_memory(2 * BUF / 16, &dma_sel);
            if (seg < 0) { printf("DTALKD: no DOS memory for the DMA buffer\n"); return -1; }
            dma_phys = (unsigned long)seg << 4;
            if ((dma_phys & 0xFFFF) + BUF > 0x10000UL) dma_phys = (dma_phys + 0xFFFFUL) & ~0xFFFFUL;
            for (i = 0; i < BUF; i++) _farnspokeb(dma_phys + i, 128);
        }
        isr_sb_sel = _my_ds();                       /* patch the wrapper's "mov ax, selector" (text is writable through DS) */
        new_irq.pm_offset = (unsigned long)isr_sb_wrapper; new_irq.pm_selector = _my_cs();
        _go32_dpmi_get_protected_mode_interrupt_vector(irq_vec, &old_irq);
        __asm__ __volatile__("cli");
        _go32_dpmi_set_protected_mode_interrupt_vector(irq_vec, &new_irq); hooked_irq = 1;
        imr_bits_m = sb_irq < 8 ? (unsigned char)(1 << sb_irq) : 4; imr_bits_s = sb_irq < 8 ? 0 : (unsigned char)(1 << (sb_irq - 8));
        imr_saved_m = inportb(0x21); imr_saved_s = inportb(0xA1);
        outportb(0x21, imr_saved_m & ~imr_bits_m); if (imr_bits_s) outportb(0xA1, imr_saved_s & ~imr_bits_s);
        __asm__ __volatile__("sti");
        dma_start(); dsp_start();
    } else {
        unsigned long period = 1193182UL / SPK_RATE;   /* 145 PIT ticks per sample */
        for (i = 0; i < 256; i++) pwm_lut[i] = (unsigned char)(1 + (unsigned long)i * (period - 2) / 255);
        _farsetsel(_dos_ds);
        for (i = 0; i < RM_RING_SIZE; i++) _farnspokeb(stub_lin + RM_RING + i, pwm_lut[128]);
        _farnspokeb(stub_lin + RM_IDLE, pwm_lut[128]);
        spk_step_q16 = ((unsigned long)rate << 16) / SPK_RATE; spk_pos_q16 = 0;
        isr_rtc_sel = _my_ds();
        new_rtc.pm_offset = (unsigned long)isr_rtc_wrapper; new_rtc.pm_selector = _my_cs();
        _go32_dpmi_get_protected_mode_interrupt_vector(rtc_vec, &old_rtc);
        _go32_dpmi_get_real_mode_interrupt_vector(0x70, &old70_rm);
        __asm__ __volatile__("cli");
        port61 = inportb(0x61);
        outportb(0x43, 0xB0);                        /* channel 2, lobyte only, mode 0 */
        outportb(0x42, pwm_lut[128]);
        outportb(0x61, port61 | 3);                  /* gate + speaker data on */
        _go32_dpmi_set_protected_mode_interrupt_vector(rtc_vec, &new_rtc); hooked_rtc = 1;   /* ticks arriving in PM */
        new14_rm.rm_segment = dos_seg; new14_rm.rm_offset = RM_IRQ8;
        _go32_dpmi_set_real_mode_interrupt_vector(0x70, &new14_rm); hooked70 = 1;             /* ticks arriving in RM (over CWSDPMI's reflector) */
        outportb(0x70, 0x8A); rtc_a = inportb(0x71); outportb(0x70, 0x8A); outportb(0x71, (rtc_a & 0xF0) | 0x03);   /* rate 3 = 8192 Hz */
        outportb(0x70, 0x8B); rtc_b = inportb(0x71); outportb(0x70, 0x8B); outportb(0x71, rtc_b | 0x40);            /* periodic interrupt enable */
        outportb(0x70, 0x0C); inportb(0x71);
        imr_bits_m = 4; imr_bits_s = 1; imr_saved_m = inportb(0x21); imr_saved_s = inportb(0xA1);
        outportb(0xA1, imr_saved_s & ~1); outportb(0x21, imr_saved_m & ~4);   /* unmask IRQ 8 and the cascade */
        __asm__ __volatile__("sti");
    }
    /* INT 14h last */
    new14_rm.rm_segment = dos_seg; new14_rm.rm_offset = RM_INT14;
    _go32_dpmi_set_real_mode_interrupt_vector(0x14, &new14_rm); hooked14 = 1;
    return 0;
}
static void uninstall(void)
{
    __asm__ __volatile__("cli");
    if (hooked14) { _go32_dpmi_set_real_mode_interrupt_vector(0x14, &old14_rm); hooked14 = 0; }
    if (hooked_irq) {
        dsp_write(0xD0); dsp_write(0xDA); dsp_reset(); dsp_write(0xD3);
        outportb(0x0A, 4 | sb_dma);                  /* mask the DMA channel too */
        _go32_dpmi_set_protected_mode_interrupt_vector(irq_vec, &old_irq); hooked_irq = 0;
    }
    if (hooked_rtc) {
        outportb(0x70, 0x8B); outportb(0x71, rtc_b); outportb(0x70, 0x8A); outportb(0x71, rtc_a);
        outportb(0x70, 0x0C); inportb(0x71);
        _go32_dpmi_set_protected_mode_interrupt_vector(rtc_vec, &old_rtc); hooked_rtc = 0;
        outportb(0x61, (inportb(0x61) & ~3) | (port61 & 3));   /* gate and speaker data as found */
    }
    if (hooked70) { _go32_dpmi_set_real_mode_interrupt_vector(0x70, &old70_rm); hooked70 = 0; }
    if (imr_bits_m | imr_bits_s) {                   /* PIC masks: put back only the bits we cleared */
        outportb(0x21, (inportb(0x21) & ~imr_bits_m) | (imr_saved_m & imr_bits_m));
        if (imr_bits_s) outportb(0xA1, (inportb(0xA1) & ~imr_bits_s) | (imr_saved_s & imr_bits_s));
        imr_bits_m = imr_bits_s = 0;
    }
    if (bda_set) { _farpokew(_dos_ds, 0x400 + port_index * 2, 0); bda_set = 0; }
    __asm__ __volatile__("sti");
    if (cb_put.rm_segment) { _go32_dpmi_free_real_mode_callback(&cb_put); cb_put.rm_segment = 0; }
    if (cb_wake.rm_segment) { _go32_dpmi_free_real_mode_callback(&cb_wake); cb_wake.rm_segment = 0; }
    if (dma_sel >= 0) { __dpmi_free_dos_memory(dma_sel); dma_sel = -1; }
    if (dos_sel >= 0) { __dpmi_free_dos_memory(dos_sel); dos_sel = -1; }
}

static void send(const char *s) { while (*s) text_put((unsigned char)*s++); last_arrival_tick = tick; }
static void report(const char *tag)
{
    unsigned imr, irr, isr, sbst = 0, dpos = 0;
    outportb(0x20, 0x0A); irr = inportb(0x20); outportb(0x20, 0x0B); isr = inportb(0x20); imr = inportb(0x21);
    if (!use_spk) { outportb(sb_base + 4, 0x82); sbst = inportb(sb_base + 5); dpos = dma_pos(); }
    _farsetsel(_dos_ds);
    printf("%s: t=%lu imr=%02X irr=%02X isr=%02X sbint=%02X dmapos=%u sbirq=%lu rtc(pm=%lu rm=%lu) wake(pm=%lu rm=%lu) resumes=%lu int14(pm=%lu rm=%lu) ring=%lu/%lu text=%u/%u utt=%lu/%lu/%lu underrun=%lu flush=%lu stage=%u busy=%u\n",
           tag, bios_ticks() & 0xFFFF, imr, irr, isr, sbst, dpos, n_sb_irq, n_rtc_pm, _farnspeekl(stub_lin + RM_NIRQ8), n_wake_pm, _farnspeekl(stub_lin + RM_NWAKE), n_resume,
           n_int14_pm, _farnspeekl(stub_lin + RM_N14), ring_committed, ring_consumed, text_head, text_tail,
           engine_utterances, engine_aborted, engine_resets, n_underrun, n_flush, synth_stage, synth_busy);
}

int main(int argc, char **argv)
{
    char *comspec;
    char *child_args[32]; int n_child = 0;
    int i, r;
    unsigned long heap0, heap1;
    /* Ctrl-C / Ctrl-Break in the child must stay ordinary keys: DJGPP's keyboard hook would otherwise
     * raise SIGINT by shrinking our DS limit to 4 KB, and the next output interrupt would fault. */
    __djgpp_set_ctrl_c(0);
    for (i = 1; i < argc; i++) {
        if (strnicmp(argv[i], "COM", 3) == 0 && argv[i][3] >= '1' && argv[i][3] <= '4') port_index = argv[i][3] - '1';
        else if (stricmp(argv[i], "/TEST") == 0) test_mode = 1;
        else if (stricmp(argv[i], "/SPK") == 0) use_spk = 1;
        else if (stricmp(argv[i], "/8K") == 0) rate = 8000;
        else if (stricmp(argv[i], "/PMALL") == 0) pm_all = 1;
        else if (stricmp(argv[i], "/NOSTI") == 0) nosti = 1;
        else if (stricmp(argv[i], "/LOW") == 0) low_mem = 1;        /* stub in conventional memory, never in a UMB */
        else if (stricmp(argv[i], "/C") == 0) { for (i++; i < argc && n_child < 30; i++) child_args[n_child++] = argv[i]; break; }
        else if (stricmp(argv[i], "/SAY") == 0) { say_mode = 1; for (i++; i < argc; i++) { if (*say_text) strncat(say_text, " ", sizeof say_text - strlen(say_text) - 1); strncat(say_text, argv[i], sizeof say_text - strlen(say_text) - 1); } break; }
        else { printf("usage: DTALKD [COM1..COM4] [/SPK] [/TEST] [/8K] [/LOW] [/PMALL] [/C command | /SAY text]\n"); return 1; }
    }

    parse_blaster();
    if (_go32_dpmi_remaining_physical_memory() < 2000000UL) {   /* CWSDPMI dies with a page fault instead of failing malloc */
        printf("DTALKD: only %lu KB of extended memory is free; the voice needs about 2 MB (a RAM disk or EMS pool may be using it)\n",
               _go32_dpmi_remaining_physical_memory() / 1024);
        return 1;
    }
    heap0 = (unsigned long)sbrk(0);
    if (engine_init_dectalk(rate)) { printf("DTALKD: DECtalk init failed\n"); return 1; }
    engine_warmup();
    heap1 = (unsigned long)sbrk(0);
    if (lock_all()) return 1;
    if (install()) { uninstall(); return 1; }
    set_volume(getenv("DTVOL") ? (unsigned)atoi(getenv("DTVOL")) : 5);   /* DTVOL=0..9 overrides the DoubleTalk default */
    if (use_spk) printf("DTALKD 0.1: DECtalk at %d Hz on the PC speaker (RTC %u Hz PWM), DoubleTalk on COM%u; heap %lu KB locked, DOS free %u KB, XMS free %lu KB\n",
                        rate, SPK_RATE, port_index + 1, (heap1 - heap0) / 1024, dos_free_kb(), _go32_dpmi_remaining_physical_memory() / 1024);
    else printf("DTALKD 0.1: DECtalk at %d Hz, DSP %d.%02d at %Xh IRQ %d DMA %d, DoubleTalk on COM%u; heap %lu KB locked, DOS free %u KB, XMS free %lu KB\n",
                rate, dsp_ver >> 8, dsp_ver & 0xFF, sb_base, sb_irq, sb_dma, port_index + 1, (heap1 - heap0) / 1024, dos_free_kb(), _go32_dpmi_remaining_physical_memory() / 1024);
    { __dpmi_version_ret v; __dpmi_get_version(&v);
      printf("DPMI host %d.%02d (%s, %s, %s), stub at %04Xh\n", v.major, v.minor,
             (v.flags & 1) ? "32-bit" : "16-bit", (v.flags & 2) ? "V86 reflection" : "real-mode reflection",
             (v.flags & 4) ? "virtual memory" : "no paging", dos_seg); }
    if (say_mode) {                                  /* one shot: speak the text, wait for it, unload */
        unsigned long t0 = bios_ticks();
        send("\x01" "50P\x01" "5S\x01" "9V ");           /* menus: full volume */
        send(say_text); send("\r");
        while (bios_ticks() - t0 < 1092 && !(synth_waiting && text_head == text_tail && ring_committed <= ring_consumed)) ;
        /* the DMA buffer holds up to 2 halves and an emulator (SBEMU) may buffer more: let it play out before the DSP reset */
        t0 = bios_ticks(); while (bios_ticks() - t0 < 27) ;
        if (getenv("DTDEBUG")) report("say");
        uninstall();
        return 0;
    }
    send("\x01" "50P\x01" "5S\x01" "5V DEC talk ready.\r");

    if (test_mode) {                                 /* ~3 s in protected mode, ~3 s in real mode, then uninstall */
        unsigned long t0 = bios_ticks(), last = 0; int sent = 0;
        __dpmi_regs rr;
        printf("test: vectors irq=%02Xh rtc=%02Xh, stub at %04X:0000, callbacks put=%04X:%04X wake=%04X:%04X, %s\n",
               irq_vec, rtc_vec, dos_seg, cb_put.rm_segment, cb_put.rm_offset, cb_wake.rm_segment, cb_wake.rm_offset,
               pm_all ? "every INT 14h function in protected mode" : "status/init/receive answered in real mode");
        while (bios_ticks() - t0 < 55) {
            if (bios_ticks() - last >= 18) { last = bios_ticks(); report("pm"); }
            if (!sent && bios_ticks() - t0 > 20) { sent = 1; send("Test one two three.\r"); }
        }
        printf("now spinning in real mode for 3 s (interrupts arrive in real mode, as under the child shell)\n");
        send("Four five six seven eight nine ten.\r");
        memset(&rr, 0, sizeof rr);
        rr.x.cs = dos_seg; rr.x.ip = RM_RMWAIT; rr.x.cx = 55; rr.x.ss = rr.x.sp = 0;
        __dpmi_simulate_real_mode_procedure_retf(&rr);
        report("rm");
        while (bios_ticks() - t0 < 200 && !(synth_waiting && text_head == text_tail && ring_committed <= ring_consumed)) ;
        report("end");
        printf("synth stack used %u of %u bytes, heap allocations from the synthesizer %lu (arena %u bytes), heap %lu KB\n",
               synth_stack_used(), SYNTH_STACK, guard_allocs, arena_used, ((unsigned long)sbrk(0) - heap0) / 1024);
        uninstall();
        return 0;
    }

    comspec = getenv("COMSPEC"); if (!comspec) comspec = "C:\\COMMAND.COM";
    setenv("PROMPT", "[DEC talk] $p$g", 1);            /* the child shell shows it is the talking shell; EXIT unloads */
    if (n_child) {
        char *av[36]; int k = 0;
        av[k++] = comspec; av[k++] = "/C";
        for (i = 0; i < n_child; i++) av[k++] = child_args[i];
        av[k] = NULL;
        r = spawnv(P_WAIT, comspec, av);
    } else {
        printf("Type EXIT to unload DTALKD.\n");
        r = spawnl(P_WAIT, comspec, comspec, NULL);
    }
    if (r < 0) perror("DTALKD: cannot run the shell");
    /* let the last words out (at most 30 s): text still queued, an utterance in progress, or samples not yet played */
    { unsigned long t0 = bios_ticks(); while (bios_ticks() - t0 < 546 && !(synth_waiting && text_head == text_tail && ring_committed <= ring_consumed)) ; }
    { unsigned long t0 = bios_ticks(); while (bios_ticks() - t0 < 27) ; }   /* drain the DMA buffer / emulator latency before the DSP reset */
    report("unload");
    uninstall();
    printf("DTALKD unloaded (%lu utterances, %lu heap allocations from interrupt context, synth stack %u bytes)\n", engine_utterances, guard_allocs, synth_stack_used());
    return r < 0 ? 1 : 0;
}
