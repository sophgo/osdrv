/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: clock_cooling.c
 * Description: Cvitek clock cooling driver for cv182x
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/clk.h>

struct dev_freq {
	unsigned long cpu_freq;
	unsigned long tpu_freq;
};

struct cv182x_cooling_device {
	struct thermal_cooling_device *cdev;
	struct mutex lock; /* lock to protect the content of this struct */
	unsigned long clk_state;
	unsigned long max_clk_state;
	struct clk *clk_cpu;
	struct clk *clk_tpu;
	struct dev_freq *dev_freqs;
};

static const struct dev_freq default_dev_freqs[] = {
	{1000000000, 650000000},
	{500000000, 375000000},
	{500000000, 100000000},
};

/* cooling device thermal callback functions are defined below */

/**
 * cv182x_cooling_get_max_state - callback function to get the max cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the max cooling state.
 *
 * Callback for the thermal cooling device to return the
 * max cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cv182x_cooling_get_max_state(struct thermal_cooling_device *cdev,
					unsigned long *state)
{
	struct cv182x_cooling_device *cvcdev = cdev->devdata;

	mutex_lock(&cvcdev->lock);
	*state = cvcdev->max_clk_state;
	mutex_unlock(&cvcdev->lock);

	return 0;
}

/**
 * cv182x_cooling_get_cur_state - function to get the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the current cooling state.
 *
 * Callback for the thermal cooling device to return the current cooling
 * state.
 *
 * Return: 0 (success)
 */
static int cv182x_cooling_get_cur_state(struct thermal_cooling_device *cdev,
					unsigned long *state)
{
	struct cv182x_cooling_device *cvcdev = cdev->devdata;

	mutex_lock(&cvcdev->lock);
	*state = cvcdev->clk_state;
	mutex_unlock(&cvcdev->lock);

	return 0;
}

/**
 * cv182x_cooling_set_cur_state - function to set the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: set this variable to the current cooling state.
 *
 * Callback for the thermal cooling device to change the cv182x cooling
 * current cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cv182x_cooling_set_cur_state(struct thermal_cooling_device *cdev,
					unsigned long state)
{
	struct cv182x_cooling_device *cvcdev = cdev->devdata;

	dev_dbg(&cdev->device, "set cur_state=%ld\n", state);
	dev_dbg(&cdev->device, "clk_a53=%ld Hz\n", clk_get_rate(cvcdev->clk_cpu));
	dev_dbg(&cdev->device, "clk_tpu_axi=%ld Hz\n", clk_get_rate(cvcdev->clk_tpu));

	mutex_lock(&cvcdev->lock);

	if (state <= cvcdev->max_clk_state && state != cvcdev->clk_state) {
		dev_dbg(&cdev->device, "dev_freq[%ld].cpu_freq=%ld\n", state, cvcdev->dev_freqs[state].cpu_freq);
		dev_dbg(&cdev->device, "dev_freq[%ld].tpu_freq=%ld\n", state, cvcdev->dev_freqs[state].tpu_freq);

		if (cvcdev->dev_freqs[state].cpu_freq != clk_get_rate(cvcdev->clk_cpu)) {
			dev_dbg(&cdev->device, "set cpu freq=%ld\n", cvcdev->dev_freqs[state].cpu_freq);
			clk_set_rate(cvcdev->clk_cpu, cvcdev->dev_freqs[state].cpu_freq);
		}

		if (cvcdev->dev_freqs[state].tpu_freq != clk_get_rate(cvcdev->clk_tpu)) {
			dev_dbg(&cdev->device, "set tpu freq=%ld\n", cvcdev->dev_freqs[state].tpu_freq);
			clk_set_rate(cvcdev->clk_tpu, cvcdev->dev_freqs[state].tpu_freq);
		}

		cvcdev->clk_state = state;
	}

	mutex_unlock(&cvcdev->lock);
	return 0;
}

/* Bind clock callbacks to thermal cooling device ops */
static struct thermal_cooling_device_ops const cv182x_cooling_ops = {
	.get_max_state = cv182x_cooling_get_max_state,
	.get_cur_state = cv182x_cooling_get_cur_state,
	.set_cur_state = cv182x_cooling_set_cur_state,
};

static struct cv182x_cooling_device *
cv182x_cooling_device_register(struct device *dev)
{
	struct thermal_cooling_device *cdev;
	struct cv182x_cooling_device *cvcdev = NULL;
	struct device_node *np = dev->of_node;
	unsigned long clk_cpu_max_freq;
	unsigned long clk_tpu_max_freq;
	int proplen;
	int i;

	cvcdev = devm_kzalloc(dev, sizeof(*cvcdev), GFP_KERNEL);
	if (!cvcdev)
		return ERR_PTR(-ENOMEM);

	mutex_init(&cvcdev->lock);

	/* get clk_cpu */
	cvcdev->clk_cpu = devm_clk_get(dev, "clk_a53");
	if (IS_ERR(cvcdev->clk_cpu)) {
		dev_err(dev, "failed to get clk_a53\n");
		return ERR_PTR(-EINVAL);
	}

	cvcdev->clk_tpu = devm_clk_get(dev, "clk_tpu_axi");
	if (IS_ERR(cvcdev->clk_tpu)) {
		dev_err(dev, "failed to get clk_tpu_axi\n");
		return ERR_PTR(-EINVAL);
	}

	/* get device frequency list from device tree */
	proplen = of_property_count_u32_elems(np, "dev-freqs");
	dev_info(dev, "elems of dev-freqs=%d\n", proplen);

	if (proplen < 0 || proplen % 2) {
		dev_info(dev, "No 'dev-freqs' property found, use default value\n");

		cvcdev->dev_freqs = devm_kmemdup(dev, &default_dev_freqs, sizeof(default_dev_freqs), GFP_KERNEL);
		if (!cvcdev->dev_freqs)
			return ERR_PTR(-ENOMEM);

		cvcdev->max_clk_state = ARRAY_SIZE(default_dev_freqs) - 1;
	} else {
		cvcdev->dev_freqs = devm_kzalloc(dev, sizeof(struct dev_freq) * (proplen / 2), GFP_KERNEL);

		if (!cvcdev->dev_freqs)
			return ERR_PTR(-ENOMEM);

		for (i = 0; i < proplen / 2; i++) {
			of_property_read_u32_index(np, "dev-freqs", i * 2,
						   (u32 *)&cvcdev->dev_freqs[i].cpu_freq);
			of_property_read_u32_index(np, "dev-freqs", i * 2 + 1,
						   (u32 *)&cvcdev->dev_freqs[i].tpu_freq);
		}

		cvcdev->max_clk_state = (proplen / 2) - 1;
	}

	/* 1st cpu/tpu freq should be the original clk freq */
	cvcdev->dev_freqs[0].cpu_freq = clk_get_rate(cvcdev->clk_cpu);
	cvcdev->dev_freqs[0].tpu_freq = clk_get_rate(cvcdev->clk_tpu);

	/* Get the cpu/tpu original clk freq as max freq */
	clk_cpu_max_freq = clk_get_rate(cvcdev->clk_cpu);
	clk_tpu_max_freq = clk_get_rate(cvcdev->clk_tpu);

	/* Check if the cpu/tpu clk freq exceeds max freq */
	for (i = 1; i <= cvcdev->max_clk_state; i++) {
		if (cvcdev->dev_freqs[i].cpu_freq > clk_cpu_max_freq) {
			dev_info(dev, "dev_freqs[%d].cpu_freq=%ld exceeds max_freq=%ld\n",
				 i, cvcdev->dev_freqs[i].cpu_freq, clk_cpu_max_freq);
			cvcdev->dev_freqs[i].cpu_freq = clk_cpu_max_freq;
		}

		if (cvcdev->dev_freqs[i].tpu_freq > clk_tpu_max_freq) {
			dev_info(dev, "dev_freqs[%d].tpu_freq=%ld exceeds max_freq=%ld\n",
				 i, cvcdev->dev_freqs[i].tpu_freq, clk_tpu_max_freq);
			cvcdev->dev_freqs[i].tpu_freq = clk_tpu_max_freq;
		}

		dev_info(dev, "dev_freqs[%d]: %ld %ld\n",
			 i, cvcdev->dev_freqs[i].cpu_freq, cvcdev->dev_freqs[i].tpu_freq);
	}

	cvcdev->clk_state = 0;
	cdev = thermal_of_cooling_device_register(dev->of_node,
						  "cv182x_cooling", cvcdev,
						  &cv182x_cooling_ops);
	if (IS_ERR(cdev))
		return ERR_PTR(-EINVAL);

	cvcdev->cdev = cdev;

	return cvcdev;
}

static void cv182x_cooling_device_unregister(struct cv182x_cooling_device
						 *cvcdev)
{
	thermal_cooling_device_unregister(cvcdev->cdev);
}

static int cv182x_cooling_probe(struct platform_device *pdev)
{
	struct cv182x_cooling_device *cvcdev;

	cvcdev = cv182x_cooling_device_register(&pdev->dev);
	if (IS_ERR(cvcdev)) {
		int ret = PTR_ERR(cvcdev);

		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev, "Failed to register cooling device %d\n", ret);

		return ret;
	}

	platform_set_drvdata(pdev, cvcdev);
	dev_info(&pdev->dev, "Cooling device registered: %s\n", cvcdev->cdev->type);

	return 0;
}

static int cv182x_cooling_remove(struct platform_device *pdev)
{
	struct cv182x_cooling_device *cvcdev = platform_get_drvdata(pdev);

	if (!IS_ERR(cvcdev))
		cv182x_cooling_device_unregister(cvcdev);

	return 0;
}

static const struct of_device_id cv182x_cooling_match[] = {
	{.compatible = "cvitek,cv182x-cooling"},
	{},
};
MODULE_DEVICE_TABLE(of, cv182x_cooling_match);

static struct platform_driver cv182x_cooling_driver = {
	.driver = {
		.name = "cv182x-cooling",
		.of_match_table = of_match_ptr(cv182x_cooling_match),
	},
	.probe = cv182x_cooling_probe,
	.remove = cv182x_cooling_remove,
};

module_platform_driver(cv182x_cooling_driver);

MODULE_AUTHOR("Cvitek Co.");
MODULE_DESCRIPTION("cv182x clock cooling driver");
MODULE_LICENSE("GPL");
