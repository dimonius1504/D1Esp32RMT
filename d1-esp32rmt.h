
#pragma once

#if defined(ESP32)

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"

#define RMT_RX_ACTIVE_LEVEL  0

#define RMT_CLK_DIV      80     /*!< RMT counter clock divider */
#define RMT_TICK_10_US   (80000000/RMT_CLK_DIV/100000)   /*!< RMT counter value for 10 us.(Source clock is APB clock) */

#define NEC_HEADER_HIGH_US    9000                         /*!< NEC protocol header: positive 9ms */
#define NEC_HEADER_LOW_US     4500                         /*!< NEC protocol header: negative 4.5ms*/
#define NEC_BIT_ONE_HIGH_US    560                         /*!< NEC protocol data bit 1: positive 0.56ms */
#define NEC_BIT_ONE_LOW_US    (2250-NEC_BIT_ONE_HIGH_US)   /*!< NEC protocol data bit 1: negative 1.69ms */
#define NEC_BIT_ZERO_HIGH_US   560                         /*!< NEC protocol data bit 0: positive 0.56ms */
#define NEC_BIT_ZERO_LOW_US   (1120-NEC_BIT_ZERO_HIGH_US)  /*!< NEC protocol data bit 0: negative 0.56ms */
#define NEC_BIT_END            560                         /*!< NEC protocol end: positive 0.56ms */
#define NEC_BIT_MARGIN         400                         /*!< NEC parse margin time */

#define NEC_BIT_MARGIN_HDR     2000                        

#define NEC_ITEM_DURATION(d)  ((d & 0x7fff)*10/RMT_TICK_10_US)  /*!< Parse duration time from memory register value */
#define NEC_DATA_ITEM_NUM   34  /*!< NEC code item number: header + 32bit data + end */
#define RMT_TX_DATA_NUM  100    /*!< NEC tx test data number */
#define rmt_item32_tIMEOUT_US  10000   /*!< RMT receiver timeout value(us) 9500*/

typedef void (d1Esp32RMT_rx) (uint16_t addr, uint16_t cmd, void* parameters);
typedef void (d1Esp32RMT_log) (const char* logString, bool newLine);

class D1Esp32RMT
{
public:
    D1Esp32RMT():rxCallBack(NULL),logCallBack(NULL){};
    uint32_t rxInit(int rxChannel, int gpioNum);
    uint32_t rxStart(d1Esp32RMT_rx* rxCb, void* taskParameters, UBaseType_t priority, d1Esp32RMT_log* logCb);
    uint32_t rxStop();

    void* rxParameters;
    d1Esp32RMT_rx* rxCallBack;
    int rxChannel;
    d1Esp32RMT_log* logCallBack;

private:

    TaskHandle_t handleRxTask;
    rmt_config_t rmt_rx;

    static void taskRxFunction(void* parameters);

    bool NEC_CheckInRange(int duration_ticks, int target_us, int margin_us);
    bool NEC_HeaderIf(rmt_item32_t* item);
    bool NEC_BitOneIf(rmt_item32_t* item);
    bool NEC_BitZeroIf(rmt_item32_t* item);
    int NEC_ParseItems(rmt_item32_t* item, int item_num, uint16_t* addr, uint16_t* data);
};

#endif
