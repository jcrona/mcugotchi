#include "stm32f0xx_hal_conf.h"

static uint32_t tick = 0;

void SysTick_Handler(void)
{

	tick++;

	if (!(tick % 1000)) {
			tick = 0;
			GPIOC->ODR ^= (1 << 8);
	}
}

int main(void)
{
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	GPIOC->MODER = (1 << 16);
	SysTick_Config(SystemCoreClock/1000);
	while(1);
}
