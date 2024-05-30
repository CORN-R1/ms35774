/*

Status: Incomplete, very WIP!!!!

*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>

int ms35774_run_thread_handler(void *data);
int ms35774_probe(struct platform_device *);
void ms35774_shutdown(struct platform_device *);
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
	int angle_now;
	int angle_target;
	bool needs_update;
};

struct platform_driver ms35774_pdrv = {
	.probe = ms35774_probe,
	.shutdown = ms35774_shutdown,
	.driver = {
		.name = "step_motor_ms35774",
	}
};

int ms35774_run_thread_handler(void *data)
{
	return 0; // TODO
}

int ms35774_probe(struct platform_device *pdev)
{
	struct ms35774_pdata *state;
	struct device *dev;

	dev = &pdev->dev;
	
	state = devm_kmalloc(dev, sizeof(*state), 0); // TODO: figure out correct flags!!! (or maybe there's a relevant helper func)
	if (state == NULL) {
		return -ENOMEM;
	}

	state->dev = dev;
	dev->platform_data = state;
	
	dev_info(dev, "[%s] enter\n", "ms35774_probe"); // TODO: function name is clearly inserted by some macro or other - find it if it exists, or create a new one

	// TODO: break this out into its own function

	state->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(state->pinctrl)) {
		dev_err(dev, "[%s] fail to get pinctrl\n", "ms35774_parse_dts"); // TODO: as above
		return -ENODEV;
	}

	for (size_t i = 0; i < MS35774_PIN_COUNT; i++) {
		state->pctrlstates[i] = pinctrl_lookup_state(state->pinctrl, MS35774_PIN_NAMES[i]);
		if (IS_ERR(state->pctrlstates[i])) {
			dev_err(dev, "[%s] fail to get pinctrl %s\n", "ms35774_parse_dts", MS35774_PIN_NAMES[i]);
			return -ENODEV;
		}
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

	struct task_struct *thread = kthread_create_on_node(
		ms35774_run_thread_handler,
		state,
		NUMA_NO_NODE,
		"ms35774_step_motor_run_thread"
	);

	if (IS_ERR(thread)) {
		dev_err(dev, "[%s] fail to create motor run thread\n", "ms35774_probe");
		return PTR_ERR(thread);
	}

	wake_up_process(thread);

	return 0; // TODO
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
