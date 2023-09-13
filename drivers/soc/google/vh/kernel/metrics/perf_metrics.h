/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Support for Perf metrics
 *
 * Copyright 2022 Google LLC
 */

#include <linux/interrupt.h>

#define RESUME_LATENCY_STEP_SMALL 10
#define RESUME_LATENCY_STEP_MID 50
#define RESUME_LATENCY_STEP_LARGE 100

#define RESUME_LATENCY_BOUND_SMALL 250
#define RESUME_LATENCY_BOUND_MID 500
#define RESUME_LATENCY_BOUND_MAX 1000

#define RESUME_LATENCY_DEFAULT_THRESHOLD 200

#define MAX_IRQ_NUM 2048
#define IRQ_ARR_LIMIT 100

#define LATENCY_CNT_SMALL (RESUME_LATENCY_BOUND_SMALL / RESUME_LATENCY_STEP_SMALL)
#define LATENCY_CNT_MID ((RESUME_LATENCY_BOUND_MID - RESUME_LATENCY_BOUND_SMALL) / \
	RESUME_LATENCY_STEP_MID)
#define LATENCY_CNT_LARGE ((RESUME_LATENCY_BOUND_MAX - RESUME_LATENCY_BOUND_MID) / \
	RESUME_LATENCY_STEP_LARGE)
#define RESUME_LATENCY_ARR_SIZE (LATENCY_CNT_SMALL + LATENCY_CNT_MID + LATENCY_CNT_LARGE + 1)

struct irq_storm_data {
	atomic64_t storm_count;
	s64 max_storm_count;
	atomic64_t irq_storm_start;
};

struct  resume_latency {
	u64 resume_start;
	u64 resume_end;
	spinlock_t resume_latency_stat_lock;
	s64 resume_count[RESUME_LATENCY_ARR_SIZE];
	u64 resume_latency_max_ms;
	u64 resume_latency_sum_ms;
	u64 resume_latency_threshold;
	bool display_warning;
};

struct long_irq {
	ktime_t softirq_start[CONFIG_VH_SCHED_MAX_CPU_NR][NR_SOFTIRQS];
	ktime_t irq_start[CONFIG_VH_SCHED_MAX_CPU_NR][MAX_IRQ_NUM];
	atomic64_t long_softirq_count;
	atomic64_t long_irq_count;
	atomic64_t long_softirq_count_arr[CONFIG_VH_SCHED_MAX_CPU_NR];
	atomic64_t long_irq_count_arr[CONFIG_VH_SCHED_MAX_CPU_NR];
	s64 long_softirq_arr[NR_SOFTIRQS];
	s64 long_irq_arr[MAX_IRQ_NUM];
	struct irq_storm_data irq_storms[MAX_IRQ_NUM];
	s64 long_softirq_threshold;
	s64 long_irq_threshold;
	s64 irq_storm_threshold_us;
	bool display_warning;
};
