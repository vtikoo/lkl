#ifndef _ASM_LKL_UACCESS_H
#define _ASM_LKL_UACCESS_H

/* Platform should implement lkl_access_check to override the default behaviour */
extern int __attribute__((weak)) lkl_access_ok(unsigned long addr, unsigned long size);

#define __access_ok(addr,size)  __lkl_access_ok(addr, size)

static inline int __lkl_access_ok(unsigned long addr, unsigned long size)
{
    int ret = 1; /* Default is ok*/

    /* Application specfic access_check is invoked if defined. */
    if ( lkl_access_ok ) ret = lkl_access_ok(addr, size);
    return ret;
}

/* copied from old include/asm-generic/uaccess.h */
static inline __must_check long raw_copy_from_user(void *to,
		const void __user *from, unsigned long n)
{
	if (__builtin_constant_p(n)) {
		switch (n) {
		case 1:
			*(u8 *)to = *(u8 __force *)from;
			return 0;
		case 2:
			*(u16 *)to = *(u16 __force *)from;
			return 0;
		case 4:
			*(u32 *)to = *(u32 __force *)from;
			return 0;
#ifdef CONFIG_64BIT
		case 8:
			*(u64 *)to = *(u64 __force *)from;
			return 0;
#endif
		default:
			break;
		}
	}

	memcpy(to, (const void __force *)from, n);
	return 0;
}

static inline __must_check long raw_copy_to_user(void __user *to,
		const void *from, unsigned long n)
{
	if (__builtin_constant_p(n)) {
		switch (n) {
		case 1:
			*(u8 __force *)to = *(u8 *)from;
			return 0;
		case 2:
			*(u16 __force *)to = *(u16 *)from;
			return 0;
		case 4:
			*(u32 __force *)to = *(u32 *)from;
			return 0;
#ifdef CONFIG_64BIT
		case 8:
			*(u64 __force *)to = *(u64 *)from;
			return 0;
#endif
		default:
			break;
		}
	}

	memcpy((void __force *)to, from, n);
	return 0;
}


#include <asm-generic/uaccess.h>

#endif
