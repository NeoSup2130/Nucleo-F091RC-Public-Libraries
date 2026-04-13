#include "stm32f091xc.h"
#include "stdio.h"
#include "stdbool.h"
#include "leds.h"
#include "buttons.h"
#include "usart2.h"
#include "ad.h"
#include "main.h"
#include "ledhandler.h"
#include "printio.h"

#define SHOW_BINARY_COUNTER (0u)
#define SHOW_START_STOP (0u)
#define SHOW_VOLUME_METER (1u)

static volatile uint32_t ticks = 0;

int main(void)
{
	SystemClock_Config();
	InitButtons();
	InitLeds();
	InitUsart2(9600);
	InitAd();
	
	StringToUsart2("Reboot\r\n");
	
#if SHOW_BINARY_COUNTER
	InitLedHandler();
	
	while (1)
	{	
		BinaryCounterLoop();
	}
#endif
#if SHOW_START_STOP
	uint8_t lastButtons = 0;
	uint8_t activeButtons = 0;
	while(true)
	{
		/* Doordat de knoppen gepolled worden voor 'press' en 'release', moet dender voorkomen worden.
		 * Dit doen we door de staat tussen twee moment opname (delta 100ms) te vergelijken */
		activeButtons = (SW1Active() << 0u) | (SW4Active() << 1u);
		if (activeButtons == lastButtons) continue;

		/* Voorkom knop dender */
		WaitForMs(100u);

		lastButtons = (SW1Active() << 0u) | (SW4Active() << 1u);
		if (activeButtons == lastButtons)
		{
			if ((activeButtons & 1u) == 1u) ToggleLED(0); // LED 1
			if ((activeButtons & 2u) == 2u) ToggleLED(7); // LED 8
		}
	}
#endif
#if SHOW_VOLUME_METER
	print("Volume meter\n\r");
	uint8_t ad_level = 0, highest_level = 0, ad_step = 256u / 8u;
	uint8_t msec_count = 0;
	while(true)
	{
		ad_level = (uint8_t)(GetAdValue() >> 4u);
		/* msec_count wordt met elke 10ms verhoogd.
		 * Als msec >= 1000ms of AD is groter dan max, pas AD niveau aan. */
		if (++msec_count >= 100u || ad_level > highest_level)
		{
			highest_level = ad_level;
			msec_count = 0;
		}
		/* Doof LEDs zodat de correcte aantal LEDs getoond worden */
		ResetLEDs();
		/* Toon huidige AD niveau aan via LEDs*/
		for (uint8_t i = 0, max = ad_level / ad_step; i < (max + 1); i++)
			SetLED(i, true);

		/* Toon hoogste AD niveau van de afgelopen 1000ms */
		SetLED(highest_level / ad_step, true);

		WaitForMs(10);
	}
#endif

	return 0;
}

void SysTick_Handler(void)
{
	ticks++;
}

void WaitForMs(uint32_t timespan)
{
	uint32_t startTime = ticks;
	
	while(ticks < startTime + timespan);
}

void SystemClock_Config(void)
{
	RCC->CR |= RCC_CR_HSITRIM_4;														// HSITRIM op 16 zetten, dit is standaard (ook na reset).
	RCC->CR  |= RCC_CR_HSION;																// Internal high speed oscillator enable (8MHz)
	while((RCC->CR & RCC_CR_HSIRDY) == 0);									// Wacht tot HSI zeker ingeschakeld is
	
	RCC->CFGR &= ~RCC_CFGR_SW;															// System clock op HSI zetten (SWS is status geupdatet door hardware)	
	while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);	// Wachten to effectief HSI in actie is getreden
	
	RCC->CR &= ~RCC_CR_PLLON;																// Eerst PLL uitschakelen
	while((RCC->CR & RCC_CR_PLLRDY) != 0);									// Wacht tot PLL zeker uitgeschakeld is
	
	RCC->CFGR |= RCC_CFGR_PLLSRC_HSI_PREDIV;								// 01: HSI/PREDIV selected as PLL input clock
	RCC->CFGR2 |= RCC_CFGR2_PREDIV_DIV2;										// prediv = /2		=> 4MHz
	RCC->CFGR |= RCC_CFGR_PLLMUL12;													// PLL multiplied by 12 => 48MHz
	
	FLASH->ACR |= FLASH_ACR_LATENCY;												//  meer dan 24 MHz, dus latency op 1 (p 67)
	
	RCC->CR |= RCC_CR_PLLON;																// PLL inschakelen
	while((RCC->CR & RCC_CR_PLLRDY) == 0);									// Wacht tot PLL zeker ingeschakeld is

	RCC->CFGR |= RCC_CFGR_SW_PLL; 													// PLLCLK selecteren als SYSCLK (48MHz)
	while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);	// Wait until the PLL is switched on
		
	RCC->CFGR |= RCC_CFGR_HPRE_DIV1;												// SYSCLK niet meer delen, dus HCLK = 48MHz
	RCC->CFGR |= RCC_CFGR_PPRE_DIV1;												// HCLK niet meer delen, dus PCLK = 48MHz	
	
	SystemCoreClockUpdate();																// Nieuwe waarde van de core frequentie opslaan in SystemCoreClock variabele
	SysTick_Config(48000);																	// Interrupt genereren. Zie core_cm0.h, om na ieder 1ms een interrupt 
																													// te hebben op SysTick_Handler()
}
