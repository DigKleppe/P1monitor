/*

 . /home/dig/esp/esp-idf/export.sh
 idf.py monitor -p /dev/ttyUSB2

 -s ${openocd_path}/share/openocd/scripts -f interface/ftdi/esp32_devkitj_v1.cfg -f target/esp32.cfg -c "program_esp /mnt/linuxData/projecten/git/thermostaat/SensirionSCD30/build//app.bin 0x10000 verify"

 */


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"

#include "esp_system.h"
#include "driver/gpio.h"

#include "esp_http_client.h"
#include "esp_image_format.h"
#include "esp_log.h"

#include "esp_wifi.h"

#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "mdns.h"

#include "wifiConnect.h"
#include "settings.h"
#include "main.h"
#include "p1parser.h"

#include "scripts.h"

static const char *TAG = "main";

//#define SIMULATE

esp_err_t init_spiffs(void);

extern bool settingsChanged; // from settingsScreen
uint32_t stackWm[5];
uint32_t upTime;

void uartRxTask(void *arg);
extern const char _3PhaseSimData[];


extern "C" {
void app_main() {
	esp_err_t err;

	bool toggle = false;


	esp_rom_gpio_pad_select_gpio(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_drive_capability(LED_PIN, GPIO_DRIVE_CAP_0);

	gpio_set_level(LED_PIN, 0);

	ESP_ERROR_CHECK(esp_event_loop_create_default());

	ESP_LOGI(TAG, "\n **************** start *****************\n");

	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
		ESP_LOGI(TAG, "nvs flash erased");
	}
	ESP_ERROR_CHECK(err);

	ESP_ERROR_CHECK(init_spiffs());

	err = loadSettings();
	getSensorNameScript(NULL, 0);  // pull in  scripts

	wifiConnect();

	do {
		toggle = !toggle;
		gpio_set_level(LED_PIN, toggle);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	} while (connectStatus != IP_RECEIVED);

	gpio_set_level(LED_PIN, 0);

	xTaskCreate(uartRxTask, "uartRxTask", 1024 * 3, NULL, configMAX_PRIORITIES, NULL);

	while (1) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		upTime++;
#ifdef SIMULATE
		parseP1data( (char *)_3PhaseSimData, strlen((char *)_3PhaseSimData));
#endif

		if (connectStatus != IP_RECEIVED) {
			toggle = !toggle;
			gpio_set_level(LED_PIN, toggle);
		} else {
		//	gpio_set_level(LED_PIN, false);

			if (wifiSettings.updated) {
				wifiSettings.updated = false;
				saveSettings();
			}
			if (settingsChanged) {
				settingsChanged = false;
				vTaskDelay(1000 / portTICK_PERIOD_MS);
				saveSettings();
			}
		//	stats_display();
		}
	}
}
}

