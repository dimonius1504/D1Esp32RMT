# D1Esp32RMT

IR remote control library for ESP32 RMT module (only NEC and only receiver)

## Usage

```c++
#include "d1-esp32rmt.h"

#define rmtChannel 0
#define gpioNum 33
#define rmtTaskPriority 2

D1Esp32RMT esp32Rmt;

void rxRMTCallBack(uint16_t addr, uint16_t cmd, void* parameters)
{
    int someParameter = (int) *parameters;
    if (addr == 0xEF00)
    {
	switch (cmd)
        {
    	    case 0xFC03:
		doSome1(someParameter, "str");
        	break;
	    case 0xFD02:
        	doSome2(someParameter, "str2");
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

int someParameter;
void setup()
{
    someParameter = 1977;
    esp32Rmt.rxInit(rmtChannel, gpioNum);
    esp32Rmt.rxStart(rxRMTCallBack, &someParameter, rmtTaskPriority, logRMTCallBack);
};
```
