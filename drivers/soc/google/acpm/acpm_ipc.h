/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright 2020 Google LLC
 *
 */
#ifndef __ACPM_IPC_H_
#define __ACPM_IPC_H_

#include <soc/google/acpm_ipc_ctrl.h>

struct buff_info {
	void __iomem *rear;
	void __iomem *front;
	void __iomem *base;
	void __iomem *direction;

	unsigned int size;
	unsigned int len;
	unsigned int d_buff_size;
};

struct callback_info {
	void (*ipc_callback)(unsigned int *cmd, unsigned int size);
	struct device_node *client;
	struct list_head list;
};

struct acpm_ipc_ch {
	struct buff_info rx_ch;
	struct buff_info tx_ch;
	struct list_head list;

	unsigned int id;
	unsigned int type;
	unsigned int seq_num;
	unsigned int *cmd;
	struct mutex rx_lock;
	struct mutex tx_lock;
	struct mutex ch_lock;

	struct completion wait;
	bool polling;
};

struct acpm_ipc_info {
	unsigned int num_channels;
	struct device *dev;
	struct acpm_ipc_ch *channel;
	unsigned int irq;
	void __iomem *intr;
	void __iomem *sram_base;
	bool w_mode;
	struct acpm_framework *initdata;
	unsigned int initdata_base;
	unsigned int intr_status;
};

struct acpm_debug_info {
	unsigned int period;
	void __iomem *time_index;
	unsigned int num_timestamps;
	unsigned long long *timestamps;

	void __iomem *log_buff_rear;
	void __iomem *log_buff_front;
	void __iomem *log_buff_base;
	unsigned int log_buff_len;
	unsigned int log_buff_size;
	void __iomem *dump_base;
	unsigned int dump_size;
	void __iomem *dump_dram_base;
	unsigned int debug_log_level;
	struct delayed_work periodic_work;
	struct work_struct update_log_work;

	spinlock_t lock; /* generic spin-lock for debug */
};

#define LOG_ID_SHIFT				(28)
#define LOG_TIME_INDEX				(20)
#define LOG_LEVEL				(19)
#define BUSY_WAIT				(0)
#define SLEEP_WAIT				(1)
#define INTGR0					0x0020
#define INTCR0					0x0024
#define INTMR0					0x0028
#define INTSR0					0x002c
#define INTMSR0					0x0030
#define INTGR1					0x0040
#define INTMR1					0x0048
#define INTSR1					0x004c
#define INTMSR1					0x0050
#define	APM_INTGR				(INTGR1)
#define AP_INTMR				(INTMR0)
#define AP_INTCR				(INTCR0)
#define AP_INTSR				(INTSR0)
#define SR0					0x0080
#define SR1					0x0084
#define SR2					0x0088
#define SR3					0x008C

#define IPC_TIMEOUT				(15000000)
#define APM_PERITIMER_NS_PERIOD			(10416)

#define UNTIL_EQUAL(arg0, arg1, flag)			\
do {							\
	u64 timeout = sched_clock() + IPC_TIMEOUT;	\
	bool t_flag = true;				\
	do {						\
		if ((arg0) == (arg1)) {			\
			t_flag = false;			\
			break;				\
		} else {				\
			cpu_relax();			\
		}					\
	} while (timeout >= sched_clock());		\
	if (t_flag) {					\
		pr_err("%s %d Timeout error!\n",	\
				__func__, __LINE__);	\
	}						\
	(flag) = t_flag;				\
} while (0)

#define REGULATOR_INFO_ID	8

extern void acpm_log_print(void);
extern void timestamp_write(void);
extern void acpm_ramdump(void);
extern void acpm_fw_log_level(unsigned int on);
extern void acpm_ipc_set_waiting_mode(bool mode);

extern int acpm_ipc_remove(struct platform_device *pdev);
extern int acpm_ipc_probe(struct platform_device *pdev);
#endif
