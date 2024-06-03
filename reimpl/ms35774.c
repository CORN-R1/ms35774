/*

Status: Incomplete, very WIP!!!!

*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/delay.h>


#define LOG_INFO(dev, fmt, ...) dev_info(dev, "[%s] " fmt, __FUNCTION__, ##__VA_ARGS__)
#define LOG_ERR(dev, fmt, ...) dev_err(dev, "[%s] " fmt, __FUNCTION__, ##__VA_ARGS__)

#define STEPS_PER_DEGREE 6

int ms35774_run_thread_handler(void *);
void ms35774_init_work_handler(struct work_struct *);
int ms35774_probe(struct platform_device *);
void ms35774_shutdown(struct platform_device *);
ssize_t ms35774_show_orientation(struct device *, struct device_attribute *, char *);
ssize_t ms35774_store_orientation(struct device *, struct device_attribute *, const char *, size_t);
int ms35774_driver_init(void);
void ms35774_driver_exit(void);

enum {
	MS35774_VM_HIGH,
	MS35774_VM_LOW,
	MS35774_ENN_HIGH,
	MS35774_ENN_LOW,
	MS35774_DIR_HIGH,
	MS35774_DIR_LOW,
	MS35774_STEP_HIGH,
	MS35774_STEP_LOW,
	MS35774_PDN_HIGH,
	MS35774_PDN_LOW,
	MS35774_MS1_HIGH,
	MS35774_MS1_LOW,
	MS35774_MS2_HIGH,
	MS35774_MS2_LOW,

	MS35774_PIN_COUNT,
};

static const char *MS35774_PIN_NAMES[] = {
	[MS35774_VM_HIGH]   = "ms35774_vm_high",
	[MS35774_VM_LOW]    = "ms35774_vm_low",
	[MS35774_ENN_HIGH]  = "ms35774_enn_high",
	[MS35774_ENN_LOW]   = "ms35774_enn_low",
	[MS35774_DIR_HIGH]  = "ms35774_dir_high",
	[MS35774_DIR_LOW]   = "ms35774_dir_low",
	[MS35774_STEP_HIGH] = "ms35774_step_high",
	[MS35774_STEP_LOW]  = "ms35774_step_low",
	[MS35774_PDN_HIGH]  = "ms35774_pdn_high",
	[MS35774_PDN_LOW]   = "ms35774_pdn_low",
	[MS35774_MS1_HIGH]  = "ms35774_ms1_high",
	[MS35774_MS1_LOW]   = "ms35774_ms1_low",
	[MS35774_MS2_HIGH]  = "ms35774_ms2_high",
	[MS35774_MS2_LOW]   = "ms35774_ms2_low",
};

struct ms35774_pdata {
	struct device *dev;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pctrlstates[MS35774_PIN_COUNT];
	struct work_struct *work;
	int angle_now; // TODO: maybe store as uint? (also maybe rename to "orientation")
	int angle_target;
	bool needs_update;
};

int ms35774_parse_dts(struct ms35774_pdata *);

struct platform_driver ms35774_pdrv = {
	.probe = ms35774_probe,
	.shutdown = ms35774_shutdown,
	.driver = {
		.name = "step_motor_ms35774",
	}
};

struct device_attribute ms35774_devattr = {
	.attr.name = "orientation",
	.show = ms35774_show_orientation,
	.store = ms35774_store_orientation,
};

DECLARE_WAIT_QUEUE_HEAD(ms35774_wq);

ssize_t ms35774_show_orientation(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ms35774_pdata *state = dev->platform_data;
	return sprintf(buf, "%d\n", state->angle_now);
}

ssize_t ms35774_store_orientation(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	struct ms35774_pdata *state = dev->platform_data;
	unsigned int angle;
	
	if (kstrtoint(buf, 10, &angle) != 0) {
		LOG_ERR(dev, "format error, request int 0~180\n");
		return len; // This should probably return an error value but that's not what the original does...
	}
	
	if (angle > 180) {
		LOG_ERR(dev, "wrong request orientation(%d), must be 0~180\n", angle);
		return len; // likewise
	}

	state->angle_target = angle;
	state->needs_update = true;
	wake_up(&ms35774_wq);

	return len;
}

int ms35774_run_thread_handler(void *data)
{
	struct ms35774_pdata *state = data;
	unsigned int angle_to_turn = 0;

	while (!kthread_should_stop())
	{
		if (!state->needs_update) {
			wait_event(ms35774_wq, state->needs_update);
		}
		LOG_INFO(state->dev, "run\n");

		if (state->angle_now == state->angle_target) {
			LOG_INFO(state->dev, "same orientation\n");
			break;
		}

		if (state->angle_target < state->angle_now) {
			pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_DIR_LOW]);
			angle_to_turn = state->angle_now - state->angle_target;
		} else {
			pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_DIR_HIGH]);
			angle_to_turn = state->angle_target - state->angle_now;
		}

		LOG_INFO(state->dev, "o:%d, or:%d, step:%d\n", state->angle_now, state->angle_target, angle_to_turn * STEPS_PER_DEGREE);

		pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_VM_HIGH]);
		udelay(10000); // 10ms
		pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_ENN_LOW]);

		for (int i = 0; i < angle_to_turn * STEPS_PER_DEGREE; i++) {
			pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_STEP_HIGH]);
			udelay(50);
			pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_STEP_LOW]);
			udelay(360);
		}

		udelay(50); // this one seems kinda unnecessary, but whatever
		pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_ENN_HIGH]);
		pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_VM_LOW]);

		state->needs_update = false;
		state->angle_now = state->angle_target;

		LOG_INFO(state->dev, "done\n");
	}
	return 0;
}

void ms35774_init_work_handler(struct work_struct *work)
{
	return; // TODO
}

int ms35774_parse_dts(struct ms35774_pdata *state)
{
	state->pinctrl = devm_pinctrl_get(state->dev);
	if (IS_ERR(state->pinctrl)) {
		LOG_ERR(state->dev, "failed to get pinctrl\n"); // TODO: as above
		return -ENODEV;
	}

	for (size_t i = 0; i < MS35774_PIN_COUNT; i++) {
		state->pctrlstates[i] = pinctrl_lookup_state(state->pinctrl, MS35774_PIN_NAMES[i]);
		if (IS_ERR(state->pctrlstates[i])) {
			LOG_ERR(state->dev, "failed to get pinctrl %s\n", MS35774_PIN_NAMES[i]);
			return -ENODEV;
		}
	}

	return 0;
}

int ms35774_probe(struct platform_device *pdev)
{
	struct ms35774_pdata *state;
	struct device *dev;
	int err;

	dev = &pdev->dev;
	
	state = devm_kmalloc(dev, sizeof(*state), 0); // TODO: figure out correct flags!!! (or maybe there's a relevant helper func)
	if (state == NULL) {
		return -ENOMEM;
	}

	state->dev = dev;
	dev->platform_data = state;
	
	LOG_INFO(dev, "enter\n"); // TODO: function name is clearly inserted by some macro or other - find it if it exists, or create a new one

	err = ms35774_parse_dts(state);
	if (err < 0) {
		LOG_ERR(dev,"failed to parse dts\n");
		return err;
	}
	
	pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_VM_LOW]);
	pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_ENN_HIGH]);
	pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_DIR_LOW]);
	pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_STEP_LOW]);
	pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_PDN_LOW]);

	// half-step mode
	pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_MS1_HIGH]);
	pinctrl_select_state(state->pinctrl, state->pctrlstates[MS35774_MS2_LOW]);

	state->angle_now = 0;
	state->angle_target = 0;
	state->needs_update = false;

	struct task_struct *thread = kthread_create(
		ms35774_run_thread_handler,
		state,
		"ms35774_step_motor_run_thread"
	);

	if (IS_ERR(thread)) {
		LOG_ERR(dev, "failed to create motor run thread\n");
		return PTR_ERR(thread);
	}

	wake_up_process(thread);

	INIT_WORK(state->work, ms35774_init_work_handler);

	err = device_create_file(dev, &ms35774_devattr);
	if (err != 0) {
		LOG_ERR(dev, "failed to create sysfs attr! ret:%d\n", err);
		return err;
	}

	schedule_work(state->work);
	LOG_INFO(dev, "success!\n");
	return 0;
}

void ms35774_shutdown(struct platform_device *pdev)
{
	return; //TODO
}

int ms35774_driver_init(void)
{
	return platform_driver_register(&ms35774_pdrv);
}

/*
nb: this function doesn't exist in the R1's driver (since it isn't a module)
*/
void ms35774_driver_exit(void)
{
	return; // TODO: something?
}

module_init(ms35774_driver_init); 
module_exit(ms35774_driver_exit);

MODULE_LICENSE("GPL");
