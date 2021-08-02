#ifndef SSD1306_H
#define SSD1306_H

/*
 * SSD1306.h
 * Author: Harris Shallcross
 * Year: 2014-ish
 *
 * SSD1306 driver library using the STM32F0 discovery board
 * STM32F0 communicates with OLED display through SPI, pinouts described below.
 *
 * This code is provided AS IS and no warranty is included!
 */

#include "stm32f0xx_hal.h"

#include <string.h>
#include <inttypes.h>

//SSD1306 Pin definitions
/*
 * LCD Clock - PA5
 * LCD Data - PA7
 * LCD DC - PA1
 * LCD CE (or CS depending on LCD model) - PA2
 * LCD Reset - PA6
 * LCD VCC (if used) - PA3
 */
#define Clk GPIO_PIN_5
#define DIn GPIO_PIN_7
#define DC GPIO_PIN_1
#define CE GPIO_PIN_2
#define RS GPIO_PIN_6
#define VCC GPIO_PIN_3

#define IOGPIO GPIOA

#define NormDisp 0xA6
#define InvDisp 0xA7
#define DispOnRAM 0xA4
#define DispOnAll 0xA5
#define DispOff 0xAE
#define DispOn 0xAF
#define Contrast 0x81

#define SetRemap 0xA0
#define SetMuxRatio 0xA8
#define SetComScanDir 0xC0
#define SetDispOff 0xD3
#define SetComPinHW 0xDA
#define SetDispFreq 0xD5
#define SetPreChrg 0xD9
#define ChargePump 0x8D
#define SetComHLvl 0xDB

#define SetDispOffset 0xD3
#define SetDispStartLine 0x40

#define SetLowAdd 0x00
#define SetHighAdd 0x10
#define MemAddMode 0x20
#define MModeH 0x00
#define MModeV 0x01
#define MModeP 0x02
#define PageStrtAdd 0xB0
#define SetColAdd 0x21
#define SetPageAdd 0x22

#define XPix 128
#define YPix 64
#define GBufS (XPix*YPix)/8

typedef enum{
	LCDInv,
	LCDNorm,
} LCDScrnMode;

typedef enum{
	LCDSleep,
	LCDWake,
} LCDPwrMode;

typedef enum{
	Dat,
	Reg
} WMode;

void SSD1306_InitSetup(void);
void LCDScreenMode(LCDScrnMode);
void LCDSleepMode(LCDPwrMode);
void SB(uint8_t, WMode, uint8_t);
void Delay(uint32_t);
void PScrn(void);
void ClrBuf(void);

#endif
