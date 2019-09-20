
#include <linux/seq_file.h>

#ifdef CONFIG_PROC_FS

/*
 * TODO: This currently just hard-codes some output for /proc/cpuinfo that is
 * plausible and can be parsed by an application. Note though that CPU features, 
 * the virtual CPU core count and socket count are wrong.
 */
static int show_cpuinfo(struct seq_file *m, void *v)
{
    seq_printf(m, "processor       : 0\n"
                "cpu family      : 6\n"
                "model           : 158\n"
                "model name      : Intel(R) Xeon(R) CPU E3-1280 v6 @ 3.90GHz\n"
                "stepping        : 9\n"
                "microcode       : 0xb4\n"
                "cpu MHz         : 800.063\n"
                "cache size      : 8192 KB\n"
                "physical id     : 0\n"
                "siblings        : 4\n"
                "core id         : 0\n"
                "cpu cores       : 4\n"
                "apicid          : 0\n"
                "initial apicid  : 0\n"
                "fpu             : yes\n"
                "fpu_exception   : yes\n"
                "cpuid level     : 22\n"
                "wp              : yes\n"
                "flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf tsc_known_freq pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault epb invpcid_single pti ssbd ibrs ibpb stibp tpr_shadow vnmi flexpriority ept vpid fsgsbase tsc_adjust bmi1 hle avx2 smep bmi2 erms invpcid rtm mpx rdseed adx smap clflushopt intel_pt xsaveopt xsavec xgetbv1 xsaves dtherm ida arat pln pts hwp hwp_notify hwp_act_window hwp_epp md_clear flush_l1d\n"
                "bugs            : cpu_meltdown spectre_v1 spectre_v2 spec_store_bypass l1tf mds swapgs\n"
                "bogomips        : 7824.00\n"
                "clflush size    : 64\n"
                "cache_alignment : 64\n"
                "address sizes   : 39 bits physical, 48 bits virtual\n"
                "power management: \n");

    seq_puts(m, "\n\n");

    return 0;
}

int __cpuinfo_accessed = 0;

static void *c_start(struct seq_file *m, loff_t *pos)
{
  // TODO: missing correct implementation
  
  // Only output information once
  if (__cpuinfo_accessed)
    return NULL;

  __cpuinfo_accessed = 1;
  return (void*)1;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
  // TODO: missing implementation
  return NULL;
}

static void c_stop(struct seq_file *m, void *v)
{
  // TODO: missing implementation
}

const struct seq_operations cpuinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= show_cpuinfo,
};

#endif