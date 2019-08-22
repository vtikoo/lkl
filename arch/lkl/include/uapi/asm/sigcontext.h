#ifndef _ASM_UAPI_LKL_SIGCONTEXT_H
#define _ASM_UAPI_LKL_SIGCONTEXT_H

/* x86_64 architecture specific settings */

struct pt_regs {
/*
 * C ABI says these regs are callee-preserved. They aren't saved on kernel entry
 * unless syscall needs a complete, fully filled "struct pt_regs".
 */
    unsigned long r15;
    unsigned long r14;
    unsigned long r13;
    unsigned long r12;
    unsigned long rbp;
    unsigned long rbx;

/* These regs are callee-clobbered. Always saved on kernel entry. */
    unsigned long r11;
    unsigned long r10;
    unsigned long r9;
    unsigned long r8;
    unsigned long rax;
    unsigned long rcx;
    unsigned long rdx;
    unsigned long rsi;
    unsigned long rdi;

/*
 * On syscall entry, this is syscall#. On CPU exception, this is error code.
 * On hw interrupt, it's IRQ number:
 */
    unsigned long orig_rax;
/* Return frame for iretq */
    unsigned long rip;
    unsigned long cs;
    unsigned long eflags;
    unsigned long rsp;
    unsigned long ss;
/* top of stack page */
};

struct sigcontext {
    unsigned long       r8;
    unsigned long       r9;
    unsigned long       r10;
    unsigned long       r11;
    unsigned long       r12;
    unsigned long       r13;
    unsigned long       r14;
    unsigned long       r15;
    unsigned long       rdi;
    unsigned long       rsi;
    unsigned long       rbp;
    unsigned long       rbx;
    unsigned long       rdx;
    unsigned long       rax;
    unsigned long       rcx;
    unsigned long       rsp;
    unsigned long       rip;
    unsigned long       eflags;         /* RFLAGS */
    unsigned long       cs;

    unsigned short      gs;
    unsigned short      fs;
    union {
        unsigned short  ss;     /* If UC_SIGCONTEXT_SS */
        unsigned short  __pad0; /* Alias name for old (!UC_SIGCONTEXT_SS) user-space */
    };
    unsigned long       err;
    unsigned long       trapno;
    unsigned long       oldmask;
    unsigned long       cr2;
    struct _fpstate __user          *fpstate;       /* Zero when no FPU context */
    unsigned long       reserved1[8];
};

/* Interrupts/Exceptions */
enum {
    X86_TRAP_DE = 0,        /*  0, Divide-by-zero */
    X86_TRAP_DB,            /*  1, Debug */
    X86_TRAP_NMI,           /*  2, Non-maskable Interrupt */
    X86_TRAP_BP,            /*  3, Breakpoint */
    X86_TRAP_OF,            /*  4, Overflow */
    X86_TRAP_BR,            /*  5, Bound Range Exceeded */
    X86_TRAP_UD,            /*  6, Invalid Opcode */
    X86_TRAP_NM,            /*  7, Device Not Available */
    X86_TRAP_DF,            /*  8, Double Fault */
    X86_TRAP_OLD_MF,        /*  9, Coprocessor Segment Overrun */
    X86_TRAP_TS,            /* 10, Invalid TSS */
    X86_TRAP_NP,            /* 11, Segment Not Present */
    X86_TRAP_SS,            /* 12, Stack Segment Fault */
    X86_TRAP_GP,            /* 13, General Protection Fault */
    X86_TRAP_PF,            /* 14, Page Fault */
    X86_TRAP_SPURIOUS,      /* 15, Spurious Interrupt */
    X86_TRAP_MF,            /* 16, x87 Floating-Point Exception */
    X86_TRAP_AC,            /* 17, Alignment Check */
    X86_TRAP_MC,            /* 18, Machine Check */
    X86_TRAP_XF,            /* 19, SIMD Floating-Point Exception */
    X86_TRAP_IRET = 32,     /* 32, IRET Exception */
};

#endif
