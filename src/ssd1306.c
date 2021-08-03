#include "ssd1306.h"

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
		__HAL_RCC_GPIOA_CLK_ENABLE();
		__HAL_RCC_SPI1_CLK_ENABLE();

		G.Pin  = DC | CE | RS;
		G.Mode  = GPIO_MODE_OUTPUT_PP;
		G.Pull  = GPIO_PULLUP;
		G.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(IOGPIO, &G);

		G.Pin = Clk | DIn;
		G.Mode  = GPIO_MODE_AF_PP;
		G.Alternate = GPIO_AF0_SPI1;
		HAL_GPIO_Init(IOGPIO, &G);

		/* Screen power is taken directly from supply!
		G.GPIO_Pin = VCC;
		G.GPIO_OType = GPIO_OType_OD;
		G.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_Init(IOGPIO, &G);
		*/

		hspi1.Instance               = SPI1;
		hspi1.Init.Mode              = SPI_MODE_MASTER;
		hspi1.Init.Direction         = SPI_DIRECTION_1LINE;
		hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
		hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
		hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
		hspi1.Init.NSS               = SPI_NSS_SOFT;
		hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
		hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
		hspi1.Init.TIMode            = SPI_TIMODE_DISABLED;
		hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLED;
		hspi1.Init.CRCPolynomial     = 7;
		HAL_SPI_Init(&hspi1);

		/* Enable 1LINE TX mode */
		SPI_1LINE_TX(&hspi1);

		__HAL_SPI_ENABLE(&hspi1);
	}

	//GPIO_ResetBits(IOGPIO, VCC);
	Delay(100);
	HAL_GPIO_WritePin(IOGPIO, Clk, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(IOGPIO, DIn, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(IOGPIO, DC, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(IOGPIO, CE, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(IOGPIO, RS, GPIO_PIN_RESET);
	Delay(1);
	HAL_GPIO_WritePin(IOGPIO, RS, GPIO_PIN_SET);
	Delay(1);
	HAL_GPIO_WritePin(IOGPIO, RS, GPIO_PIN_RESET);
	Delay(1);
	HAL_GPIO_WritePin(IOGPIO, RS, GPIO_PIN_SET);
	HAL_GPIO_WritePin(IOGPIO, DC, GPIO_PIN_SET);
	HAL_GPIO_WritePin(IOGPIO, CE, GPIO_PIN_SET);
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
	if(CmdDat == Reg) HAL_GPIO_WritePin(IOGPIO, DC, GPIO_PIN_RESET);
	else HAL_GPIO_WritePin(IOGPIO, DC, GPIO_PIN_SET);

	if(En) HAL_GPIO_WritePin(IOGPIO, CE, GPIO_PIN_RESET);

	/* Bypass all the ckecks from HAL and write directly to SPI1 */
	*(__IO uint8_t *) (&(hspi1.Instance)->DR) = Data;
	while(__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_BSY));

	if(En) HAL_GPIO_WritePin(IOGPIO, CE, GPIO_PIN_SET);
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

extern volatile uint32_t ticks;

void Delay(uint32_t ms){
	uint32_t ref = ticks;
	while ((ticks - ref) < (ms * 1000)) __asm__ __volatile__ ("nop");
}
