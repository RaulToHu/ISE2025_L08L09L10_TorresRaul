/**
ISE - P1
Raúl Torres Huete
(LunesTarde)
  */

#include "stm32f4xx_hal.h"
#ifndef __ADC_H
	void ADC1_pins_F429ZI_config(void);
	int ADC_Init_Conversion_Single(ADC_HandleTypeDef *, ADC_TypeDef  *);
	float ADC_getVolt(ADC_HandleTypeDef * , uint32_t );
#endif
