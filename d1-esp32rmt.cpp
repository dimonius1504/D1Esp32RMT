
#if defined(ESP32)
#include "d1-esp32rmt.h"

uint32_t D1Esp32RMT::rxInit(int rxChannel, int gpioNum)
{
    this->rxChannel = rxChannel;

    rmt_rx.channel = (rmt_channel_t)rxChannel;
    rmt_rx.gpio_num = (gpio_num_t)gpioNum;
    rmt_rx.clk_div = RMT_CLK_DIV;
    rmt_rx.mem_block_num = 1;
    rmt_rx.rmt_mode = RMT_MODE_RX;
    rmt_rx.rx_config.filter_en = true;
    rmt_rx.rx_config.filter_ticks_thresh = 200;
    rmt_rx.rx_config.idle_threshold = (uint16_t)(rmt_item32_tIMEOUT_US / 10 * (RMT_TICK_10_US));
    rmt_config(&rmt_rx);
    rmt_driver_install(rmt_rx.channel, 1000, 0);
}

uint32_t D1Esp32RMT::rxStart(d1Esp32RMT_rx* rxCb, void* taskParameters, UBaseType_t priority, d1Esp32RMT_log* logCb)
{
    rxCallBack = rxCb;
    rxParameters = taskParameters;
    logCallBack = logCb;

    xTaskCreatePinnedToCore(taskRxFunction, "D1Esp32RMT_rx_task", 2048, this, priority, &handleRxTask, 1);
}

uint32_t D1Esp32RMT::rxStop()
{
    vTaskDelete(handleRxTask);
}

void D1Esp32RMT::taskRxFunction(void* parameters)
{
    D1Esp32RMT* obj = (D1Esp32RMT*) parameters;
    if (obj->logCallBack)
        obj->logCallBack("RX task started", true);

    RingbufHandle_t rb = NULL;
    rmt_get_ringbuf_handle((rmt_channel_t) obj->rxChannel, &rb);
    rmt_rx_start((rmt_channel_t)obj->rxChannel, 1);
    int id = 0;
    char strBuf[256];

    while(rb) 
    {
        size_t rx_size = 0;
        rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, 3000);
        if (item) 
        {
            uint16_t rmt_addr = 0;
            uint16_t rmt_cmd = 0;
            int offset = 0;
            while(1) 
            {
                int res = obj->NEC_ParseItems(item + offset, rx_size / 4 - offset, &rmt_addr, &rmt_cmd);
                if(res > 0) 
                {
                    offset += res + 1;
                    if (obj)
                        obj->rxCallBack(rmt_addr, rmt_cmd, obj->rxParameters);

                    if (obj->logCallBack)
                    {
                        sprintf(strBuf, "Rx addr: %x, cmd: %x", rmt_addr, rmt_cmd);
                        obj->logCallBack(strBuf, true);
                    }
                    if ((rx_size / 4 <= offset) || (item + offset)->duration0 == 0 || (item + offset)->duration1 == 0)
                        break;
                } else 
                {
                    if (obj->logCallBack)
                    {
                        sprintf(strBuf, "Not parsed: [%d, %d | %d, %d], ", (item + offset)->duration0, (item + offset)->level0, (item + offset)->duration1, (item + offset)->level1);
                        obj->logCallBack(strBuf, true);
                    };
                    break;
                };

            };
            vRingbufferReturnItem(rb, (void*) item);
        } else 
        {
            //Serial.println("rc task breaked");
            //break;
        }
    };
    if (obj->logCallBack)
        obj->logCallBack("RX task finished", true);
    vTaskDelete(NULL);    
}

//-------------------------------------------------------------------------------------------------------
//

bool D1Esp32RMT::NEC_CheckInRange(int duration_ticks, int target_us, int margin_us)
{
    if(( NEC_ITEM_DURATION(duration_ticks) < (target_us + margin_us))
        && ( NEC_ITEM_DURATION(duration_ticks) > (target_us - margin_us))) {
        return true;
    } else {
        return false;
    }
}

bool D1Esp32RMT::NEC_HeaderIf(rmt_item32_t* item)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && NEC_CheckInRange(item->duration0, NEC_HEADER_HIGH_US, NEC_BIT_MARGIN_HDR)
        && NEC_CheckInRange(item->duration1, NEC_HEADER_LOW_US, NEC_BIT_MARGIN_HDR)) {
        return true;
    }
    return false;
}

bool D1Esp32RMT::NEC_BitOneIf(rmt_item32_t* item)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && NEC_CheckInRange(item->duration0, NEC_BIT_ONE_HIGH_US, NEC_BIT_MARGIN)
        && NEC_CheckInRange(item->duration1, NEC_BIT_ONE_LOW_US, NEC_BIT_MARGIN)) {
        return true;
    }
    return false;
}

bool D1Esp32RMT::NEC_BitZeroIf(rmt_item32_t* item)
{
    if((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
        && NEC_CheckInRange(item->duration0, NEC_BIT_ZERO_HIGH_US, NEC_BIT_MARGIN)
        && NEC_CheckInRange(item->duration1, NEC_BIT_ZERO_LOW_US, NEC_BIT_MARGIN)) {
        return true;
    }
    return false;
}

int D1Esp32RMT::NEC_ParseItems(rmt_item32_t* item, int item_num, uint16_t* addr, uint16_t* data)
{
    int w_len = item_num;
    if(w_len < NEC_DATA_ITEM_NUM) {
        //Serial.println("rc task wlen");
        return -1;
    }
    int i = 0, j = 0;
    if(!NEC_HeaderIf(item++)) {
        //Serial.println("rc task hdr");
        return -1;
    }
    uint16_t addr_t = 0;
    for(j = 0; j < 16; j++) {
        if(NEC_BitOneIf(item)) {
            addr_t |= (1 << j);
        } else if(NEC_BitZeroIf(item)) {
            addr_t |= (0 << j);
        } else {
            return -1;
        }
        item++;
        i++;
    }
    uint16_t data_t = 0;
    for(j = 0; j < 16; j++) {
        if(NEC_BitOneIf(item)) {
            data_t |= (1 << j);
        } else if(NEC_BitZeroIf(item)) {
            data_t |= (0 << j);
        } else {
            return -1;
        }
        item++;
        i++;
    }
    *addr = addr_t;
    *data = data_t;
    return i;
}

#endif