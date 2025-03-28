/**
ISE - P2
Raúl Torres Huete
(LunesTarde)
  */

#include "stm32f4xx_hal.h"

#define RESOLUTION_12BITS 4096U			// resolución del adc de 12 bits (2^12 = 4096)
#define VREF 3.3f										// tensión de referencia del ADC


// Función de configuración de pines del ADC
void ADC1_pins_F429ZI_config(){
	  GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_ADC1_CLK_ENABLE();			// habilitamos el reloj del ADC1 
	__HAL_RCC_GPIOC_CLK_ENABLE();			// habilitamos el reloj del bus (está en el PC0 y PC3, es decir, bus C)
		
	/*PC0   ------> ADC1_IN10
    PC3   ------> ADC1_IN13	*/
		
    GPIO_InitStruct.Pin = GPIO_PIN_0;						// asignamos el PIN_0 del GPIO que le digamos después		
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;		// configuramos el modo analógico
    GPIO_InitStruct.Pull = GPIO_NOPULL;					// indicamos que no resistencia de pull
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);			// inicializamos el PIN con la configuración anterior,
																									// y lo asignamos al bus correspondiente (el antes habilitado)

    GPIO_InitStruct.Pin = GPIO_PIN_3;						// asignamos el PIN_3 del GPIO que le digamos después
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;		// configuramos el modo analógico
    GPIO_InitStruct.Pull = GPIO_NOPULL;					// indicamos que no resistencia de pull
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);			// inicializamos el PIN con la configuración anterior,
																									// y lo asignamos al bus correspondiente (el antes habilitado)
 }

	
// Función de inicialización del ADC
int ADC_Init_Conversion_Single(ADC_HandleTypeDef *hadc, ADC_TypeDef  *ADC_Instance){
   /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc->Instance = ADC_Instance;
  hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc->Init.Resolution = ADC_RESOLUTION_12B;
  hadc->Init.ScanConvMode = DISABLE;
  hadc->Init.ContinuousConvMode = DISABLE;
  hadc->Init.DiscontinuousConvMode = DISABLE;
  hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc->Init.NbrOfConversion = 1;
  hadc->Init.DMAContinuousRequests = DISABLE;
  hadc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  
  if (HAL_ADC_Init(hadc) != HAL_OK) {
    return -1;
  }
	return 0;
}
 

/**
  * @brief Configure a specific channels ang gets the voltage in float type. This funtion calls to  HAL_ADC_PollForConversion that needs HAL_GetTick()
  * @param ADC_HandleTypeDef
	* @param channel number
	* @retval voltage in float (resolution 12 bits and VRFE 3.3)
  */


//// Función lectura del voltaje del ADC (medida que se verá en el servidor)
//float ADC_getVolt(ADC_HandleTypeDef *hadc, uint32_t Channel){
//	ADC_ChannelConfTypeDef config = {0};
//	HAL_StatusTypeDef status;

//	uint32_t val = 0;
//	static float voltage = 0;
//	/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
//  */

//  config.Channel = Channel;
//  config.Rank = 1;
//  config.SamplingTime = ADC_SAMPLETIME_3CYCLES;
//  if (HAL_ADC_ConfigChannel(hadc, &config) != HAL_OK) {
//    return -1;
//  }

//	HAL_ADC_Start(hadc);

//	do {
//		status = HAL_ADC_PollForConversion(hadc, 0);
//	}  //This funtions uses the HAL_GetTick(), then it only can be executed wehn the OS is running
//	while(status != HAL_OK);

//  val = HAL_ADC_GetValue(hadc);

//  voltage = val*VREF/RESOLUTION_12BITS; 

//  return voltage;
//}


// Función lectura del voltaje del ADC (medida que se verá en el servidor)
uint32_t ADC_getVolt(ADC_HandleTypeDef *hadc, uint32_t Channel){
	
	ADC_ChannelConfTypeDef config = {0};
	HAL_StatusTypeDef status;

	uint32_t val = 0;
	/** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */

  config.Channel = Channel;
  config.Rank = 1;
  config.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(hadc, &config) != HAL_OK) {
    return -1;
  }

	HAL_ADC_Start(hadc);

	do {
		status = HAL_ADC_PollForConversion(hadc, 0);
	}  //This funtions uses the HAL_GetTick(), then it only can be executed wehn the OS is running
	while(status != HAL_OK);

  val = HAL_ADC_GetValue(hadc);

  return val;
}

