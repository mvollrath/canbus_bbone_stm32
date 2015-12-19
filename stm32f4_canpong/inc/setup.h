#ifndef _SETUP_H_
#define _SETUP_H_

#include "stm32f4xx.h"

#define SETUP_SUCCESS ((uint8_t)0x00)
#define SETUP_FAIL    ((uint8_t)0x01)

uint8_t setup();

#endif
