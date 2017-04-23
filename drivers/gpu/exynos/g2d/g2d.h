/*
 * linux/drivers/gpu/exynos/g2d/g2d.h
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 * Samsung Graphics 2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_G2D_H__
#define __EXYNOS_G2D_H__

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

struct g2d_task; /* defined in g2d_task.h */

struct g2d_device {
	struct miscdevice	misc;
	struct device		*dev;
	struct clk		*clock;
	void __iomem		*reg;

	/* task management */
	spinlock_t		lock_task;
	struct g2d_task		*tasks;
	struct list_head	tasks_free;
	struct list_head	tasks_prepared;
	struct list_head	tasks_active;
	struct workqueue_struct	*schedule_workq;
};

struct g2d_context {
	struct g2d_device	*g2d_dev;
};

/* job mask (hwfc or not) */
#define G2D_JOBMASK_HWFC 0x7
#define G2D_JOBMASK_DEFAULT 0xFFF8
#define g2d_job_full(id, job_mask) ((id & job_mask) == job_mask)
#define g2d_job_empty(id, job_mask) ((id & job_mask) == 0)

int g2d_device_run(struct g2d_device *g2d_dev, struct g2d_task *task);

#endif /* __EXYNOS_G2D_H__ */
