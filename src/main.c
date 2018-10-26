/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <board.h>
#include <device.h>
#include <sensor.h>
#include <gpio.h>
#include <stdio.h>
#include <misc/util.h>


#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

// #include <gatt/hrs.h>
// #include <gatt/dis.h>
// #include <gatt/bas.h>
// #include <gatt/cts.h>
#include "gatt_service_test.h"


/* Change this if you have an LED connected to a custom port */
#ifndef LED0_GPIO_CONTROLLER
#define LED0_GPIO_CONTROLLER 	LED0_GPIO_PORT
#endif

#define LED_PORT LED0_GPIO_CONTROLLER

/* Change this if you have an LED connected to a custom pin */
#define LED	LED0_GPIO_PIN

/* 1000 msec = 1 sec */
#define SLEEP_TIME 	1000



static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      0x0d, 0x18, 0x0f, 0x18, 0x05, 0x18),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
		      0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
};

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	gatt_service_test_init();

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}












void main(void)
{
	int err;

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);


	int cnt = 0;
	struct device *dev;

	dev = device_get_binding(LED_PORT);
	/* Set LED pin as output */
	gpio_pin_configure(dev, LED, GPIO_DIR_OUT);

	struct sensor_value temp, hum;
	struct device *dev_hts221 = device_get_binding("HTS221");

	if (dev_hts221 == NULL) {
		printf("Could not get HTS221 device\n");
		return;
	}

	while (1) {
		if (sensor_sample_fetch(dev_hts221) < 0) {
			printf("Sensor sample update error\n");
			return;
		}

		if (sensor_channel_get(dev_hts221, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
			printf("Cannot read HTS221 temperature channel\n");
			return;
		}

		if (sensor_channel_get(dev_hts221, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
			printf("Cannot read HTS221 humidity channel\n");
			return;
		}

		/* display temperature */
		printf("Temperature:%.1f C\n", sensor_value_to_double(&temp));

		/* display humidity */
		printf("Relative Humidity:%.1f%%\n",
		       sensor_value_to_double(&hum));

		/* Set pin to HIGH/LOW every 1 second */
		gpio_pin_write(dev, LED, cnt % 2);


		gatt_service_test_notify(sensor_value_to_double(&temp));

		cnt++;
		k_sleep(SLEEP_TIME);
	}
}
