/*
 * HTTPS GET Example using plain Mbed TLS sockets
 *
 * Contacts the howsmyssl.com API via TLS v1.2 and reads a JSON
 * response.
 *
 * Adapted from the ssl_client1 example in Mbed TLS.
 *
 * SPDX-FileCopyrightText: The Mbed TLS Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-FileContributor: 2015-2022 Espressif Systems (Shanghai) CO LTD
 */

//Default:0x3ffda280 "GET https://digkleppe.github.io//OTAtemplate//storageVersion.txt HTTP/1.1\r\n Host: www.github.com\r\n User-Agent: esp-idf/1.0 esp32\r\n\r\n"
/* OK
 <0x1b>[0;32mI (5137) example: Connection established...<0x1b>[0m
 GET https://digkleppe.github.io//OTAtemplate//storageVersion.txt HTTP/1.1
 Host: www.github.com
 User-Agent: esp-idf/1.0 esp32

 <0x1b>[0;32mI (5147) example: 130 bytes written<0x1b>[0m

 <0x1b>[0;32mI (3311) httpsRequest: Connection established...<0x1b>[0m
 GET https://digkleppe.github.io//OTAtemplate//storage.bin HTTP/1.1
 Host: www.github.com
 User-Agent: esp-idf/1.0 esp32


 */

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "lwip/err.h"

#include "esp_tls.h"
#include "sdkconfig.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

#include "httpsRequest.h"

/* Constants that aren't configurable in menuconfig */
//#define WEB_SERVER "www.githubhowsmyssl.com"
//#define WEB_PORT "443"
//#define WEB_URL "https://www.howsmyssl.com/a/check"
//#define WEB_SERVER "www.github.com"
//#define WEB_PORT "443"
//#define WEB_URL "https://digkleppe.github.io//OTAtemplate//storageVersion.txt"
//#define WEB_URL "https://github.com//index.html"
//#define WEB_URL "https://digkleppe.github.io//index.html"
//#define WEB_URL "https://digkleppe.github.io//OTAtemplate//storage.bin"
static const char *TAG = "httpsRequest";

//
//static const char HOWSMYSSL_REQUEST[] = "GET " WEB_URL " HTTP/1.1\r\n"
//"Host: "WEB_SERVER"\r\n"
//"User-Agent: esp-idf/1.0 esp32\r\n"
//"\r\n";

extern const uint8_t server_root_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[] asm("_binary_ca_cert_pem_end");

QueueHandle_t httpsReqMssgBox;
QueueHandle_t httpsReqRdyMssgBox;

uint8_t buf[HTTPSBUFSIZE];

static void https_get_request(esp_tls_cfg_t cfg, const httpsRegParams_t *httpsRegParams, const char *REQUEST) {
	int ret, len;
	int totalLen = 0;
	bool doRead = true;
	int blocks = 0;
	int contentLength = -1;
	httpsMssg_t mssg;
	httpsMssg_t dummy;
	mssg.len = -1;  //default
	bool err = false;

	esp_tls_t *tls = esp_tls_init();
	if (!tls) {
		ESP_LOGE(TAG, "Failed to allocate esp_tls handle!");
		err = true;
	}
	if (!err) {
		if (esp_tls_conn_http_new_sync(httpsRegParams->httpsURL, &cfg, tls) == 1) {
			ESP_LOGI(TAG, "Connection established...");
		} else {
			ESP_LOGE(TAG, "Connection failed...");
			err = true;
		}
	}

	if (!err) {
		size_t written_bytes = 0;
		do {
			ret = esp_tls_conn_write(tls, REQUEST + written_bytes, strlen(REQUEST) - written_bytes);
			if (ret >= 0) {
				ESP_LOGI(TAG, "%d bytes written", ret);
				written_bytes += ret;
			} else if (ret != ESP_TLS_ERR_SSL_WANT_READ && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
				ESP_LOGE(TAG, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
				err = true;
			}
		} while (written_bytes < strlen(REQUEST) && !err);
	}
	if (!err) {
		ESP_LOGI(TAG, "Reading HTTP response...");
		do {
			len = sizeof(buf);
			memset(buf, 0x00, sizeof(buf));
			ret = esp_tls_conn_read(tls, (char*) buf, len);

			blocks++;

			if (ret == ESP_TLS_ERR_SSL_WANT_WRITE || ret == ESP_TLS_ERR_SSL_WANT_READ) {
				ESP_LOGI(TAG, "ret2: %d %s", ret, esp_err_to_name(ret));
				doRead = false;
				err = true;
			} else if (ret < 0) {
				ESP_LOGE(TAG, "esp_tls_conn_read  returned [-0x%02X](%s)", -ret, esp_err_to_name(ret));
				break;
			} else if (ret == 0) {
				ESP_LOGI(TAG, "connection closed");
				mssg.len = 0;  //no more chars availble
				break;
			}
			len = ret;
			switch (blocks) { // workaround for github pages on github final read returns error! ?? after timeout instead of 0
			case 1: {
				char *p = strstr((char*) buf, " 404 ");
				if (p) { // file not found ?
					ESP_LOGE(TAG, "File not found: %s", httpsRegParams->httpsURL);
					len = 0;
					err = true;
				} else {
					p = strstr((char*) buf, "Content-Length:"); // search content  length, on github last read returns error! ??
					if (p) {
						sscanf((p + 15), "%d", &contentLength);
						ESP_LOGI(TAG, "Content-Length: %d", contentLength);
					} else {
						ESP_LOGE(TAG, "No content-length!");
					}
				}
			}
				break;
			case 2:
				totalLen = 0; // start counting from block 2
				ESP_LOGI(TAG, "Start receiving: %d", len);
				break;
			default:
				break;
			}

			if (len > 0) {

//				for (int i = 0; i < len; i++) {
//					putchar(buf[i]);
//				}
//				putchar('\n');
				if (blocks >= 2) {  // do not export header ( BUFSIZE 1024 )
					//		if (blocks >= 3) {  // do not export header
					if (xQueueReceive(httpsReqRdyMssgBox, (void*) &dummy, MSSGBOX_TIMEOUT) == pdFALSE) {
						ESP_LOGE(TAG, "Rdy timeout");
						err = true;
					} else {

						if (len > httpsRegParams->maxChars)
							memcpy(httpsRegParams->destbuffer, buf, httpsRegParams->maxChars);
						else
							memcpy(httpsRegParams->destbuffer, buf, len);

						mssg.len = len;
						totalLen += len;

						if ( xQueueSend(httpsReqMssgBox, &mssg, MSSGBOX_TIMEOUT) == errQUEUE_FULL) {
							ESP_LOGE(TAG, " send mssg failed %d ", blocks);
							err = true;
						}
					}
				}
			}
			if (contentLength > 0) {
				if (totalLen >= contentLength) { // stop github pages??
					doRead = false;
					mssg.len = 0;
				}
			}

		} while (doRead && !err);
	}

	if (tls)
		esp_tls_conn_destroy(tls);

	if (err)
		mssg.len = -1;

	xQueueSend(httpsReqMssgBox, &mssg, MSSGBOX_TIMEOUT); // last message

	ESP_LOGI(TAG, "End, send %d mssgs, %d bytes", blocks-1, totalLen);

}

//static void https_get_request_using_crt_bundle(void) {
//	ESP_LOGI(TAG, "https_request using crt bundle");
//	esp_tls_cfg_t cfg = { .crt_bundle_attach = esp_crt_bundle_attach, };
//	https_get_request(cfg, WEB_URL, HOWSMYSSL_REQUEST);
//}

void httpsGetRequestTask(void *pvparameters) {
	ESP_LOGI(TAG, "Start https_task");
	httpsRegParams_t *httpsRegParams = (httpsRegParams_t*) pvparameters;
	char request[256];
	snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: esp-idf/1.0 esp32\r\n\r\n", httpsRegParams->httpsURL, httpsRegParams->httpsServer);

	if (httpsReqMssgBox == NULL) { // once
		httpsReqMssgBox = xQueueCreate(1, sizeof(httpsMssg_t));
		httpsReqRdyMssgBox = xQueueCreate(1, sizeof(httpsMssg_t));
	} else {
		xQueueReset(httpsReqMssgBox);
		xQueueReset(httpsReqRdyMssgBox);
	}

	esp_tls_cfg_t cfg = { .crt_bundle_attach = esp_crt_bundle_attach, };
	https_get_request(cfg, httpsRegParams, request);

	ESP_LOGI(TAG, "Finish https_request task");
	vTaskDelete(NULL);
}

//void app_main(void) {
//	ESP_ERROR_CHECK(nvs_flash_init());
//	ESP_ERROR_CHECK(esp_netif_init());
//	ESP_ERROR_CHECK(esp_event_loop_create_default());
//
//	/* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
//	 * Read "Establishing Wi-Fi or Ethernet Connection" section in
//	 * examples/protocols/README.md for more information about this function.
//	 */
//	ESP_ERROR_CHECK(example_connect());
//
//	if (esp_reset_reason() == ESP_RST_POWERON) {
//		ESP_LOGI(TAG, "Updating time from NVS");
//		ESP_ERROR_CHECK(update_time_from_nvs());
//	}
//
//	const esp_timer_create_args_t nvs_update_timer_args = { .callback = (void*) &fetch_and_store_time_in_nvs, };
//
//	esp_timer_handle_t nvs_update_timer;
//	ESP_ERROR_CHECK(esp_timer_create(&nvs_update_timer_args, &nvs_update_timer));
//	ESP_ERROR_CHECK(esp_timer_start_periodic(nvs_update_timer, TIME_PERIOD));
//
//	xTaskCreate(&https_request_task, "https_get_task", 8192, NULL, 5, NULL);
//}
