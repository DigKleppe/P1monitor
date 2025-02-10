/*
 * uart.cpp
 *
 *  Created on: Nov 2, 2023
 *      Author: dig
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "soc/uart_reg.h"
#include "crc16_table.h"
#include "p1parser.h"
#include "udpClient.h"
#include "wifiConnect.h"

static const int RX_BUF_SIZE = 2048;  // uart low level
#define P1_BUF_SIZE			   2048   // result buffer

#define TXD_PIN (GPIO_NUM_5)  // DC LCD
#define RXD_PIN (GPIO_NUM_17) // nCS LCD
#define EX_UART_NUM UART_NUM_1
static QueueHandle_t uart_queue;

static char p1Buffer[P1_BUF_SIZE];
volatile int mssgsReceived;
volatile int CRCerrors;

// @formatter:off
static void uartInit(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(EX_UART_NUM, RX_BUF_SIZE * 2, 0, 5, &uart_queue, 0);
    uart_param_config(EX_UART_NUM, &uart_config);
    uart_set_pin(EX_UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
#ifndef SIMULATE
    uart_set_line_inverse(EX_UART_NUM, UART_SIGNAL_RXD_INV);
#endif
   // uart_set_line_inverse(EX_UART_NUM, UART_SIGNAL_TXD_INV); // ??
    const uart_intr_config_t  uartIntrConfig = {
   		.intr_enable_mask = UART_RXFIFO_TOUT_INT_ENA | UART_RXFIFO_FULL_INT_ENA,
	    .rx_timeout_thresh = 10,
	    .txfifo_empty_intr_thresh = 0,
	    .rxfifo_full_thresh = SOC_UART_FIFO_LEN-10,
    };
    uart_intr_config(EX_UART_NUM,  &uartIntrConfig);
}
// @formatter:on

void uartRxTask(void *arg) {
	static const char *RX_TASK_TAG = "uart RX_TASK";
	esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
	uint8_t state = 0;
	int nrCharsInBuffer = 0;
	uart_event_t event;
	uint8_t *dtmp = (uint8_t*) malloc(RX_BUF_SIZE + 1);
	char *dest = p1Buffer;
	uartInit();
	int mssgCntr = 0;

	for (;;) {
		if (xQueueReceive(uart_queue, (void*) &event, (TickType_t) portMAX_DELAY)) {
			bzero(dtmp, RX_BUF_SIZE);
			//		ESP_LOGI(RX_TASK_TAG, "uart[%d] event:", EX_UART_NUM);
			switch (event.type) {
			//Event of UART receving data
			/*We'd better handler data event fast, there would be much more data events than
			 other types of events. If we take too much time on data event, the queue might
			 be full.*/
			case UART_DATA:
				//		ESP_LOGI(RX_TASK_TAG, "[UART DATA]: %d", event.size);
				uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);

		//		printf("%s", dtmp);

				if (dtmp[0] == '/') { // start of P1 message  /ISK5\2M550E-1013␍␍␊
					dest = p1Buffer;
					//	printf("\n**********************\n ");
					nrCharsInBuffer = event.size;
					if (nrCharsInBuffer <= sizeof(p1Buffer)) {
						memcpy(dest, dtmp, event.size);
						dest += event.size;
						state = 1;
						mssgsReceived++;
					}
				} else {
					if (state > 0) {
						if ((nrCharsInBuffer + event.size) <= sizeof(p1Buffer)) {
							memcpy(dest, dtmp, event.size);
							dest += event.size;
							nrCharsInBuffer += event.size;
							if (p1Buffer[nrCharsInBuffer - 7] == '!') { // last block with CRC received
								uint16_t crc = calculateCRC_CCITT((uint8_t*) p1Buffer, nrCharsInBuffer - 6);
								//			printf("  CRC: %x ", crc);
								unsigned int receivedeCRC;
								sscanf(&p1Buffer[nrCharsInBuffer - 6], "%x", &receivedeCRC);
#ifdef SIMULATE
								if( 1 ) {
#else
									if (crc == receivedeCRC) {
#endif
									parseP1data(p1Buffer, nrCharsInBuffer);
									ESP_LOGI(RX_TASK_TAG, "Rec: %d  %d", mssgsReceived,nrCharsInBuffer);

									if (connectStatus == IP_RECEIVED) {
										UDPsendMssg(5000, p1Buffer, nrCharsInBuffer);
										sprintf(p1Buffer, " ** %d %d\n\r", ++mssgCntr, nrCharsInBuffer);
										UDPsendMssg(5000, p1Buffer, strlen(p1Buffer));
									}
								} else {
									CRCerrors++;
									if (connectStatus == IP_RECEIVED) {
										sprintf(p1Buffer, "CRC fout %d %d\n\r", CRCerrors, nrCharsInBuffer);
										UDPsendMssg(5000, p1Buffer, strlen(p1Buffer));
									}
									printf("CRC errors: %d \n", CRCerrors);
								}
								state = 0;
								nrCharsInBuffer = 0;
							}
						}
					}
				}
				break;
				//Event of HW FIFO overflow detected
			case UART_FIFO_OVF:
				ESP_LOGI(RX_TASK_TAG, "hw fifo overflow");
				// If fifo overflow happened, you should consider adding flow control for your application.
				// The ISR has already reset the rx FIFO,
				// As an example, we directly flush the rx buffer here in order to read more data.
				uart_flush_input (EX_UART_NUM);
				xQueueReset(uart_queue);
				break;
				//Event of UART ring buffer full
			case UART_BUFFER_FULL:
				ESP_LOGI(RX_TASK_TAG, "ring buffer full");
				// If buffer full happened, you should consider increasing your buffer size
				// As an example, we directly flush the rx buffer here in order to read more data.
				uart_flush_input(EX_UART_NUM);
				xQueueReset(uart_queue);
				break;
				//Event of UART RX break detected
			case UART_BREAK:
				ESP_LOGI(RX_TASK_TAG, "uart rx break");
				break;
				//Event of UART parity check error
			case UART_PARITY_ERR:
				ESP_LOGI(RX_TASK_TAG, "uart parity error");
				break;
				//Event of UART frame error
			case UART_FRAME_ERR:
				ESP_LOGI(RX_TASK_TAG, "uart frame error");
				break;
				//UART_PATTERN_DET
			case UART_PATTERN_DET:
			default:
				ESP_LOGI(RX_TASK_TAG, "uart event type: %d", event.type);
				break;
			}
		}
	}
	free(dtmp);
	dtmp = NULL;
	vTaskDelete (NULL);
}

// test only
extern "C" const char _3PhaseSimData[];
void uartTxTask(void *arg)
{
	int len = strlen(_3PhaseSimData);
	while (1) {
		uart_write_bytes(EX_UART_NUM, (const char *) _3PhaseSimData, len);
		vTaskDelay( 1000/portTICK_PERIOD_MS);
	}
}


