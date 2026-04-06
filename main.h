#include "stm32f091xc.h"
#include "stdbool.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#if !defined(MAIN_DEFINED)
	#define MAIN_DEFINED
	
	// Functieprototypes.
	void SystemClock_Config(void);
	void WaitForMs(uint32_t timespan);
#endif
