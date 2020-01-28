/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_LKL_DMA_MAPPING_H
#define _ASM_LKL_DMA_MAPPING_H
#include <linux/string.h>
#include <linux/device.h>
#include <linux/dma-debug.h>

extern const struct dma_map_ops *dma_ops;
static inline const struct dma_map_ops *get_arch_dma_ops(struct bus_type *bus)
{
	return dma_ops;
}

#endif /* _ASM_LKL_DMA_MAPPING_H */
