# D1Esp32RMT

IR remote control library for ESP32 RMT module (only NEC and only receiver)

## Using

```c++
#include "d1-esp32rmt.h"
D1Esp32RMT esp32Rmt;

void rxRMTCallBack(uint16_t addr, uint16_t cmd, void* parameters)
{
    uint16_t w01, w02;
    if (addr == 0xEF00)
    {
	switch (cmd)
        {
    	    case 0xFC03:
		doSome1(0xF0, "str");
        	break;
	    case 0xFD02:
        	doSome2(0xF1, "str2");
        	break;
        }
    }
}

void logRMTCallBack(const char* logString, bool newLine)
{
    if (!newLine)
	Serial.print(logString);
    else
	Serial.println(logString);
}

void setup()
{
    esp32Rmt.rxInit(0, 33);
    esp32Rmt.rxStart(rxRMTCallBack, NULL, 2, logRMTCallBack);
};
```
