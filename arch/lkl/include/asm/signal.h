struct ucontext;
void do_signal(struct pt_regs *regs);
void lkl_process_trap(int signr, struct ucontext *uctx);

#include <asm-generic/signal.h>
