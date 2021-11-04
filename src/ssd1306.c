#include "ssd1306.h"
#include "time.h"
#include "spi.h"
#include "gpio.h"
#include "board.h"

/*
 * SSD1306.c
 * Author: Harris Shallcross
 * Year: 2014-ish
 *
 * SSD1306 driver library using the STM32F0 discovery board
 * STM32F0 communicates with OLED display through SPI, pinouts described below.
 *
 * This code is provided AS IS and no warranty is included!
 */

GPIO_InitTypeDef G;
SPI_InitTypeDef S;
SPI_HandleTypeDef hspi1;

void SSD1306_InitSetup(void){
	static uint8_t Init = 1;
	if(Init == 1){
		Init = 0;
		spi_init();
	}

	//GPIO_ResetBits(IOGPIO, VCC);
	Delay(100);
	gpio_clear(BOARD_SCREEN_SCLK_PORT, BOARD_SCREEN_SCLK_PIN);
	gpio_clear(BOARD_SCREEN_MOSI_PORT, BOARD_SCREEN_MOSI_PIN);
	gpio_clear(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);
	gpio_clear(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
	gpio_clear(BOARD_SCREEN_RST_PORT, BOARD_SCREEN_RST_PIN);
	Delay(1);
	gpio_set(BOARD_SCREEN_RST_PORT, BOARD_SCREEN_RST_PIN);
	Delay(1);
	gpio_clear(BOARD_SCREEN_RST_PORT, BOARD_SCREEN_RST_PIN);
	Delay(1);
	gpio_set(BOARD_SCREEN_RST_PORT, BOARD_SCREEN_RST_PIN);
	gpio_set(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);
	gpio_set(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
	Delay(10);

	SB(SetMuxRatio, Reg, 1);
	SB(0x3F, Reg, 1);
	SB(SetDispOffset, Reg, 1);
	SB(0x00, Reg, 1);
	SB(SetDispStartLine|0, Reg, 1);
	SB(SetRemap|0, Reg, 1);
	SB(SetComPinHW, Reg, 1);
	SB(0x12, Reg, 1);
	SB(SetComScanDir|0, Reg, 1);

	SB(Contrast, Reg, 1);
	SB(0x7F, Reg, 1);
	SB(MemAddMode, Reg, 1);
	SB(MModeH, Reg, 1);
	SB(SetColAdd, Reg, 1);
	SB(0x00, Reg, 1);
	SB(0x7F, Reg, 1);
	SB(SetPageAdd, Reg, 1);
	SB(0x00, Reg, 1);
	SB(0x07, Reg, 1);

	SB(NormDisp, Reg, 1);
	SB(SetComHLvl, Reg, 1);
	SB(0x00, Reg, 1);

	//SB(DispOnAll, Reg, 1); //Test whole display
	SB(DispOnRAM, Reg, 1);
	SB(SetDispFreq, Reg, 1);
	SB(0x80, Reg, 1);
	SB(ChargePump, Reg, 1);
	SB(0x14, Reg, 1);
	SB(DispOn, Reg, 1);

	ClrBuf();
	PScrn();
}

void SB(uint8_t Data, WMode CmdDat, uint8_t En){
	if(CmdDat == Reg) gpio_clear(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);
	else gpio_set(BOARD_SCREEN_DC_PORT, BOARD_SCREEN_DC_PIN);

	if(En) gpio_clear(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);

	spi_write(Data);

	if(En) gpio_set(BOARD_SCREEN_NSS_PORT, BOARD_SCREEN_NSS_PIN);
}

void LCDScreenMode(LCDScrnMode Mode){
	if(Mode == LCDInv) SB(InvDisp, Reg, 1);
	else SB(NormDisp, Reg, 1);
}

void LCDSleepMode(LCDPwrMode Mode){
	if(Mode == LCDSleep){
		SB(DispOff, Reg, 1);
		SB(ChargePump, Reg, 1);
		SB(0x10, Reg, 1);
	}
	else{
		SB(ChargePump, Reg, 1);
		SB(0x14, Reg, 1);
		SB(DispOn, Reg, 1);
	}
}

void Delay(uint32_t ms){
	time_delay(ms * 1000);
}
