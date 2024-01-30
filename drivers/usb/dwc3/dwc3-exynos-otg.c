// SPDX-License-Identifier: GPL-2.0
/**
 * dwc3-exynos-otg.c - DesignWare Exynos USB3 DRD Controller OTG
 *
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 */

#include <dwc3/core.h> /* $(srctree)/drivers/usb/dwc3/core.h */
#include <dwc3/io.h> /* $(srctree)/drivers/usb/dwc3/io.h */

#include <linux/mutex.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/usb/composite.h>
#include <linux/usb/dwc3-exynos.h>
#include <linux/reboot.h>
#include <linux/suspend.h>
#include <linux/workqueue.h>

#include "core-exynos.h"
#include "exynos-otg.h"
#include "dwc3-exynos-ldo.h"

#define OTG_NO_CONNECT		0
#define OTG_CONNECT_ONLY	1
#define OTG_DEVICE_CONNECT	2
#define LINK_DEBUG_L		(0x0C)
#define LINK_DEBUG_H		(0x10)
#define BUS_ACTIVITY_CHECK	(0x3F << 16)
#define READ_TRANS_OFFSET	10

/* -------------------------------------------------------------------------- */
static int dwc3_otg_reboot_notify(struct notifier_block *nb, unsigned long event, void *buf);
static struct notifier_block dwc3_otg_reboot_notifier = {
	.notifier_call = dwc3_otg_reboot_notify,
};

/*
 * dwc3_exynos_wait_role - wait for pending role switch to complete and return
 * the current role. The role switch is done asynchronously in a workqueue,
 * hence the previouse requested role could be overrideen by the later one.
 * The caller could use this function to ensure the role has switched as
 * intended before requesting another role switch.
 * Example use case: toggling the gadget mode off and on.
 */
enum usb_role dwc3_exynos_wait_role(struct dwc3_otg *dotg) {
	flush_work(&dotg->work);
	return dotg->current_role;
}

static void dwc3_exynos_set_role_work(struct work_struct *work) {
	struct dwc3_otg *dotg = container_of(work, struct dwc3_otg, work);
	struct dwc3_exynos *exynos = dotg->exynos;
	enum usb_role current_role, desired_role;
	int ret = 0;

	mutex_lock(&dotg->role_lock);
	desired_role = dotg->desired_role;
	current_role = dotg->current_role;

	if (desired_role == current_role) {
		dev_info(exynos->dev, "role unchanged %s\n",
			 usb_role_string(current_role));
		goto out;
	}

	switch (current_role) {
	case USB_ROLE_NONE:
		break;
	case USB_ROLE_HOST:
		ret = dwc3_otg_start_host(dotg, 0);
		break;
	case USB_ROLE_DEVICE:
		ret = dwc3_otg_start_gadget(dotg, 0);
		break;
	default:
		break;
	}

	if (ret) {
		dev_err(exynos->dev, "failed to stop %s\n",
			usb_role_string(current_role));
		goto out;
	}

	switch (desired_role) {
	case USB_ROLE_NONE:
		break;
	case USB_ROLE_HOST:
		ret = dwc3_otg_start_host(dotg, 1);
		break;
	case USB_ROLE_DEVICE:
		ret = dwc3_otg_start_gadget(dotg, 1);

		break;
	default:
		break;
	}

	if (ret) {
		dev_err(exynos->dev, "failed to start %s\n",
			usb_role_string(desired_role));
		goto out;
	}

	dotg->current_role = desired_role;
	dev_info(exynos->dev, "role switched from %s to %s\n",
		 usb_role_string(current_role), usb_role_string(desired_role));

out:
	mutex_unlock(&dotg->role_lock);
}

void dwc3_exynos_set_role(struct dwc3_otg *dotg) {
	enum usb_role new_role;

	/* Favor host mode when we have both host_on & device_on. */
	if (dotg->host_ready && dotg->host_on) {
		new_role = USB_ROLE_HOST;
	} else if (dotg->device_on) {
		new_role = USB_ROLE_DEVICE;
	} else {
		new_role = USB_ROLE_NONE;
	}

	dev_info(dotg->exynos->dev, "set desired role to %s\n",
		 usb_role_string(new_role));

	dotg->desired_role = new_role;
	if (!dotg->desired_role_kn)
		dotg->desired_role_kn = sysfs_get_dirent(dotg->exynos->dev->kobj.sd,
							 "new_data_role");
	if (dotg->desired_role_kn)
		sysfs_notify_dirent(dotg->desired_role_kn);

	schedule_work(&dotg->work);
}

/* -------------------------------------------------------------------------- */

static void dwc3_otg_set_mode(struct dwc3 *dwc, u32 mode)
{
	u32 reg;

	reg = dwc3_exynos_readl(dwc->regs, DWC3_GCTL);
	reg &= ~(DWC3_GCTL_PRTCAPDIR(DWC3_GCTL_PRTCAP_OTG));
	reg |= DWC3_GCTL_PRTCAPDIR(mode);
	dwc3_exynos_writel(dwc->regs, DWC3_GCTL, reg);
}

static void dwc3_otg_set_host_mode(struct dwc3_otg *dotg)
{
	struct dwc3 *dwc = dotg->dwc;
	u32 reg;

	/* Disable undefined length burst mode */
	reg = dwc3_exynos_readl(dwc->regs, DWC3_GSBUSCFG0);
	reg &= ~(DWC3_GSBUSCFG0_INCRBRSTEN);
	dwc3_exynos_writel(dwc->regs, DWC3_GSBUSCFG0, reg);

	dwc3_otg_set_mode(dwc, DWC3_GCTL_PRTCAP_HOST);
}

static void dwc3_otg_set_peripheral_mode(struct dwc3_otg *dotg)
{
	struct dwc3 *dwc = dotg->dwc;

	dwc3_otg_set_mode(dwc, DWC3_GCTL_PRTCAP_DEVICE);
}

void dwc3_otg_phy_tune(struct dwc3 *dwc, bool is_host)
{
	int phy_state;

	// Phy driver maps OTG state to host/device mode.
	if (is_host) {
		phy_state = OTG_STATE_A_IDLE;
	} else {
		phy_state = OTG_STATE_B_IDLE;
	}

	exynos_usbdrd_phy_tune(dwc->usb2_generic_phy,
			       phy_state);
#ifdef CONFIG_EXYNOS_USBDRD_PHY30
	exynos_usbdrd_phy_tune(dwc->usb3_generic_phy,
			       phy_state);
#endif
}

int dwc3_otg_start_host(struct dwc3_otg *dotg, int on)
{
	struct dwc3	*dwc = dotg->dwc;
	struct device	*dev = dotg->dwc->dev;
	struct dwc3_exynos *exynos = dotg->exynos;
	static struct usb_gadget_driver *temp_gadget_driver;
	struct usb_composite_driver *composite;
	int ret = 0;
	int ret1 = -1;
	int wait_counter = 0;

	__pm_stay_awake(dotg->wakelock);

	if (on) {
		if (!dwc3_otg_check_usb_suspend(exynos))
			dev_err(dev, "too long to wait for dwc3 suspended\n");

		dotg->otg_connection = 1;
		while (dwc->gadget_driver == NULL) {
			wait_counter++;
			msleep(20);

			if (wait_counter > 50) {
				dev_err(dev, "Can't wait host start!\n");
				break;
			}
		}

		if (!dwc->xhci) {
			ret = dwc3_exynos_host_init(exynos);
			if (ret) {
				dev_err(dev, "%s: failed to init dwc3 host\n", __func__);
				goto err1;
			}
		}

		/* To ignore gadget operation, it set gadget_driver to NULL */
		temp_gadget_driver = dwc->gadget_driver;
		dwc->gadget_driver = NULL;

		mutex_lock(&dotg->lock);
		exynos->need_dr_role = 1;

		ret = pm_runtime_get_sync(dev);
		if (ret < 0) {
			dev_err(dev, "failed to resume exynos device\n");
			pm_runtime_set_suspended(dev);
			mutex_unlock(&dotg->lock);
			goto err1;
		}
		exynos->need_dr_role = 0;

		/* To ignore gadget suspend/resume on host l2 suspend */
		exynos->dwc->current_dr_role = DWC3_EXYNOS_IGNORE_CORE_OPS;
		mutex_unlock(&dotg->lock);

		dwc3_otg_phy_tune(dwc, 1);

		dwc3_exynos_core_init(dwc, exynos);
		dwc3_otg_set_host_mode(dotg);

		ret = platform_device_add(dwc->xhci);
		if (ret) {
			dev_err(dev, "%s: cannot add xhci\n", __func__);
			goto err1;
		}

	} else {
		dotg->otg_connection = 0;

		if (!dwc->xhci) {
			dev_err(dev, "%s: stop USB host without xhci device\n",
				__func__);
			return -EINVAL;
		}

		if (dotg->dwc3_suspended) {
			dev_dbg(dev, "wait resume completion\n");
			ret1 = wait_for_completion_timeout(&dotg->resume_cmpl,
							msecs_to_jiffies(5000));
		}

		if (temp_gadget_driver) {
			composite = to_cdriver(temp_gadget_driver);
			if (composite && composite->gadget_driver.udc_name)
				dwc->gadget_driver = temp_gadget_driver;
		}

		dwc3_exynos_host_exit(exynos);
		dwc->xhci = NULL;
err1:
		mutex_lock(&dotg->lock);
		exynos->dwc->current_dr_role = DWC3_GCTL_PRTCAP_DEVICE;
		pm_runtime_put_sync_suspend(dev);
		mutex_unlock(&dotg->lock);
	}
	__pm_relax(dotg->wakelock);
	return ret;
}

int dwc3_otg_start_gadget(struct dwc3_otg *dotg, int on)
{
	struct dwc3	*dwc = dotg->dwc;
	struct dwc3_exynos *exynos = dotg->exynos;
	struct device	*dev = dotg->dwc->dev;
	int ret = 0;
	int wait_counter = 0;
	u32 evt_count, evt_buf_cnt;

	if (!dwc->gadget) {
		dev_err(dev, "%s does not have any gadget\n", __func__);
		return -EINVAL;
	}

	if (on) {
		__pm_stay_awake(dotg->wakelock);
		if (!dwc3_otg_check_usb_suspend(exynos))
			dev_err(dev, "too long to wait for dwc3 suspended\n");

		while (dwc->gadget_driver == NULL) {
			wait_counter++;
			usleep_range(100, 200);

			if (wait_counter > 500) {
				dev_err(dev, "Can't wait gadget start!\n");
				break;
			}
		}

		mutex_lock(&dotg->lock);
		exynos->need_dr_role = 1;
		dwc->connected = true;

		ret = pm_runtime_get_sync(dev);
		if (ret < 0) {
			dev_err(dev, "failed to resume exynos device\n");
			pm_runtime_set_suspended(dev);
		}
		exynos->need_dr_role = 0;
		mutex_unlock(&dotg->lock);

		dwc3_otg_phy_tune(dwc, 0);
		dwc3_exynos_core_init(dwc, exynos);

		/* connect gadget */
		usb_udc_vbus_handler(dwc->gadget, true);

		exynos->gadget_state = true;
		dwc3_otg_set_peripheral_mode(dotg);
	} else {
		evt_buf_cnt = dwc->ev_buf->count;

		/* Wait until gadget stop */
		wait_counter = 0;
		evt_count = dwc3_readl(dwc->regs, DWC3_GEVNTCOUNT(0));
		evt_count &= DWC3_GEVNTCOUNT_MASK;
		while (evt_count || evt_buf_cnt) {
			wait_counter++;
			mdelay(20);

			if (wait_counter > 20) {
				dev_err(dev, "Can't wait event buffer empty!\n");
				break;
			}
			evt_count = dwc3_readl(dwc->regs, DWC3_GEVNTCOUNT(0));
			evt_count &= DWC3_GEVNTCOUNT_MASK;
			evt_buf_cnt = dwc->ev_buf->count;
		}
		dev_dbg(dev, "%s, evt compl wait cnt = %d\n",
			 __func__, wait_counter);

		/* disconnect gadget */
		usb_udc_vbus_handler(dwc->gadget, false);

		if (exynos->config.is_not_vbus_pad && exynos_usbdrd_get_ldo_status() &&
				!dotg->in_shutdown)
			dwc3_exynos_gadget_disconnect_proc(dwc);

		if (exynos->extra_delay)
			msleep(100);

		if (!dwc3_otg_check_usb_activity(exynos))
			dev_err(dev, "too long to suspend after cable plug-out\n");

		mutex_lock(&dotg->lock);
		pm_runtime_put_sync_suspend(dev);
		mutex_unlock(&dotg->lock);

		exynos->gadget_state = false;

		__pm_relax(dotg->wakelock);
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
static struct device_node *exynos_dwusb_parse_dt(void)
{
	struct device_node *np = NULL;

	np = of_find_compatible_node(NULL, NULL, "samsung,exynos9-dwusb");
	if (!np) {
		pr_err("%s: failed to get the usbdrd node\n", __func__);
		goto err;
	}
	return np;
err:
	return NULL;
}

static struct dwc3_exynos *exynos_dwusb_get_struct(void)
{
	struct device_node *np = NULL;
	struct platform_device *pdev = NULL;
	struct device *dev;
	struct dwc3_exynos *exynos;

	np = exynos_dwusb_parse_dt();
	if (np) {
		pdev = of_find_device_by_node(np);
		dev = &pdev->dev;
		of_node_put(np);
		if (pdev) {
			exynos = dev->driver_data;
			return exynos;
		}
	}

	pr_err("%s: failed to get the platform_device\n", __func__);
	return NULL;
}

int dwc3_otg_host_ready(bool ready)
{
	struct dwc3_exynos *exynos;
	struct dwc3_otg *dotg;

	exynos = exynos_dwusb_get_struct();
	if (!exynos) {
		pr_err("%s: error exynos_dwusb_get_struct\n", __func__);
		return -ENODEV;
	}

	dotg = exynos->dotg;
	if (!dotg)
		return -ENOENT;

	dotg->host_ready = ready;
	dev_info(exynos->dev, "host mode %s\n", ready ? "ready" : "unready");
	dwc3_exynos_set_role(dotg);
	dwc3_exynos_wait_role(dotg);

	return 0;
}
EXPORT_SYMBOL_GPL(dwc3_otg_host_ready);

bool dwc3_otg_check_usb_suspend(struct dwc3_exynos *exynos)
{
	int wait_counter = 0;
	bool exynos_suspend, dwc_suspend;

	do {
		exynos_suspend = (pm_runtime_suspend(exynos->dev) &
				  (atomic_read(&exynos->dev->power.usage_count) < 1));
		dwc_suspend = (pm_runtime_suspend(exynos->dwc->dev) &
			       (atomic_read(&exynos->dwc->dev->power.usage_count) < 1));

		if (exynos_suspend && dwc_suspend)
			break;

		wait_counter++;
		msleep(20);
	} while (wait_counter < DWC3_EXYNOS_MAX_WAIT_COUNT);

	return wait_counter < DWC3_EXYNOS_MAX_WAIT_COUNT;
}

bool dwc3_otg_check_usb_activity(struct dwc3_exynos *exynos)
{
	int wait_counter = 0;

	do {
		if ((atomic_read(&exynos->dwc->dev->power.usage_count)) < 2)
			break;

		wait_counter++;
		msleep(20);
	} while (wait_counter < DWC3_EXYNOS_DISCONNECT_COUNT);

	return wait_counter < DWC3_EXYNOS_DISCONNECT_COUNT;
}

static int dwc3_otg_reboot_notify(struct notifier_block *nb, unsigned long event, void *buf)
{
	struct dwc3_exynos *exynos;
	struct dwc3_otg *dotg;

	exynos = exynos_dwusb_get_struct();
	if (!exynos)
		return -ENODEV;

	dotg = exynos->dotg;

	switch (event) {
	case SYS_HALT:
	case SYS_RESTART:
	case SYS_POWER_OFF:
		exynos->dwc->current_dr_role = DWC3_EXYNOS_IGNORE_CORE_OPS;
		dotg->in_shutdown = true;
		break;
	}

	return 0;
}

u32 dwc3_otg_is_connect(void)
{
	struct dwc3_exynos *exynos;
	struct dwc3_otg *dotg;

	exynos = exynos_dwusb_get_struct();
	if (!exynos || !exynos->dotg) {
		pr_err("[%s] error\n", __func__);
		return -ENODEV;
	}
	dotg = exynos->dotg;

	if (!dotg->otg_connection)
		return OTG_NO_CONNECT;
	else
		return OTG_DEVICE_CONNECT;
}
EXPORT_SYMBOL_GPL(dwc3_otg_is_connect);

int dwc3_otg_get_idle_ip_index(void)
{
	struct dwc3_exynos *exynos;

	exynos = exynos_dwusb_get_struct();

	if (exynos == NULL)
		return -ENODEV;

	return exynos->idle_ip_index;
}
EXPORT_SYMBOL_GPL(dwc3_otg_get_idle_ip_index);

static int dwc3_otg_pm_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct dwc3_otg *dotg
		= container_of(nb, struct dwc3_otg, pm_nb);

	switch (action) {
	case PM_SUSPEND_PREPARE:
		dotg->dwc3_suspended = 1;
		reinit_completion(&dotg->resume_cmpl);
		break;
	case PM_POST_SUSPEND:
		dotg->dwc3_suspended = 0;
		complete(&dotg->resume_cmpl);
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static int psy_changed(struct notifier_block *nb, unsigned long evt, void *ptr)
{
	struct dwc3_otg *dotg = container_of(nb, struct dwc3_otg, psy_notifier);
	struct power_supply *psy = ptr;

	if (!strstr(psy->desc->name, "usb") || evt != PSY_EVENT_PROP_CHANGED)
		return NOTIFY_OK;

	if (dotg->dwc->gadget->state == USB_STATE_CONFIGURED && !dotg->usb_charged) {
		dotg->usb_charged = true;
		if (dotg->dwc->speed >= DWC3_DSTS_SUPERSPEED) {
			if (dotg->pm_qos_int_usb3_val) {
				dev_dbg(dotg->dwc->dev, "pm_qos set value = %d\n",
					dotg->pm_qos_int_usb3_val);
				exynos_pm_qos_update_request(&dotg->pm_qos_int_req,
							     dotg->pm_qos_int_usb3_val);
			}
		} else {
			if (dotg->pm_qos_int_usb2_val) {
				dev_dbg(dotg->dwc->dev, "pm_qos set value = %d\n",
					dotg->pm_qos_int_usb2_val);
				exynos_pm_qos_update_request(&dotg->pm_qos_int_req,
							     dotg->pm_qos_int_usb2_val);
			}
		}
	} else if (dotg->dwc->gadget->state != USB_STATE_CONFIGURED && dotg->usb_charged) {
		dotg->usb_charged = false;
		dev_dbg(dotg->dwc->dev, "clear pm_qos value\n");
		if (dotg->pm_qos_int_usb2_val || dotg->pm_qos_int_usb3_val)
			exynos_pm_qos_update_request(&dotg->pm_qos_int_req, 0);
	}

	return NOTIFY_OK;
}

int dwc3_exynos_otg_init(struct dwc3 *dwc, struct dwc3_exynos *exynos)
{
	struct dwc3_otg *dotg;
	int ret = 0;

	dotg = devm_kzalloc(dwc->dev, sizeof(struct dwc3_otg), GFP_KERNEL);
	if (!dotg)
		return -ENOMEM;

	dotg->dwc = dwc;
	dotg->exynos = exynos;

	ret = of_property_read_u32(exynos->dev->of_node, "usb-pm-qos-usb2-int",
				   &dotg->pm_qos_int_usb2_val);

	ret = of_property_read_u32(exynos->dev->of_node, "usb-pm-qos-usb3-int",
				   &dotg->pm_qos_int_usb3_val);

	if (ret < 0) {
		dev_err(dwc->dev, "couldn't read usb-pm-qos-int %s node, error = %d\n",
			dwc->dev->of_node->name, ret);
		dotg->pm_qos_int_usb2_val = 0;
		dotg->pm_qos_int_usb3_val = 0;
	} else {
		exynos_pm_qos_add_request(&dotg->pm_qos_int_req,
					  PM_QOS_DEVICE_THROUGHPUT, 0);
	}

	dotg->current_role = USB_ROLE_NONE;
	dotg->desired_role = USB_ROLE_NONE;
	dotg->host_on = 0;
	dotg->device_on = 0;
	dotg->host_ready = false;
	dotg->in_shutdown = false;

	INIT_WORK(&dotg->work, dwc3_exynos_set_role_work);

	dotg->wakelock = wakeup_source_register(dwc->dev, "dwc3-otg");

	mutex_init(&dotg->lock);
	mutex_init(&dotg->role_lock);

	init_completion(&dotg->resume_cmpl);
	dotg->dwc3_suspended = 0;
	dotg->pm_nb.notifier_call = dwc3_otg_pm_notifier;
	register_pm_notifier(&dotg->pm_nb);

	ret = register_reboot_notifier(&dwc3_otg_reboot_notifier);
	if (ret)
		dev_err(dwc->dev, "failed register reboot notifier\n");

	dotg->psy_notifier.notifier_call = psy_changed;
	ret = power_supply_reg_notifier(&dotg->psy_notifier);
	if (ret)
		dev_err(dwc->dev, "failed register power supply notifier\n");

	/* Make dwc3_otg accessible after member variable initialization completes */
	exynos->dotg = dotg;

	dev_dbg(dwc->dev, "otg_init done\n");

	return 0;
}

void dwc3_exynos_otg_exit(struct dwc3 *dwc, struct dwc3_exynos *exynos)
{
	struct dwc3_otg *dotg = exynos->dotg;

	sysfs_put(dotg->desired_role_kn);
	unregister_pm_notifier(&dotg->pm_nb);
	cancel_work_sync(&dotg->work);
	wakeup_source_unregister(dotg->wakelock);
	kfree(dotg);
	exynos->dotg = NULL;
}
