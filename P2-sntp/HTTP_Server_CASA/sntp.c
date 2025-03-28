  /**
ISE - P2
Raúl Torres Huete
(LunesTarde)
  */

#include "sntp.h"

struct tm infoSNTP;
netStatus SNTP_OK;
uint8_t errSNTP = 0;
uint8_t errSNTP_2 = 0;
uint8_t errHora_sntp = 0;
uint8_t errDia_sntp = 0;

static void netSNTP_Callback (uint32_t seconds, uint32_t seconds_fracation);

const NET_ADDR4 ntp_server = {NET_ADDR_IP4, NULL, 129, 6, 15, 28 };

void sntp_Init (void){
  SNTP_OK = netSNTPc_GetTime ((NET_ADDR *)&ntp_server, netSNTP_Callback);
  
  if(SNTP_OK != netOK){
    errSNTP += 1; 
    while(1);
  }
}

static void netSNTP_Callback (uint32_t seconds, uint32_t seconds_fracation){
  
  if(seconds == 0){
    errSNTP_2 += 1;
    
  }else{
   
    infoSNTP = *localtime(&seconds); //Segundos desde 1 enero de 1970.
    
    /* Configure Date */
    rtc_DateConfig.Year = infoSNTP.tm_year - 100;
    rtc_DateConfig.Month = infoSNTP.tm_mon + 1;
    rtc_DateConfig.Date = infoSNTP.tm_mday;
    rtc_DateConfig.WeekDay = infoSNTP.tm_wday;
    
    if(HAL_RTC_SetDate(&rtc_Handler, &rtc_DateConfig, RTC_FORMAT_BIN) != HAL_OK)
    {
      /* Initialization Error */
     // Error_Handler();
      errDia_sntp += 1;
      while(1);
    }
    
        /* Configure Time */
    rtc_TimeConfig.Hours = (infoSNTP.tm_hour > 23) ? 0 : (infoSNTP.tm_hour + 1);
    rtc_TimeConfig.Minutes = infoSNTP.tm_min;
    rtc_TimeConfig.Seconds = infoSNTP.tm_sec;
    rtc_TimeConfig.TimeFormat = (infoSNTP.tm_hour < 12) ? RTC_HOURFORMAT12_AM : RTC_HOURFORMAT12_PM;
    rtc_TimeConfig.DayLightSaving = infoSNTP.tm_isdst;
    rtc_TimeConfig.StoreOperation = RTC_STOREOPERATION_RESET;
    
    if (HAL_RTC_SetTime(&rtc_Handler, &rtc_TimeConfig, RTC_FORMAT_BIN) != HAL_OK)
    {
    /* Initialization Error */
      errHora_sntp += 1;
    }
    
    HAL_RTCEx_BKUPWrite(&rtc_Handler, RTC_BKP_DR1, 0x32F2);
    
    for(uint8_t i = 0; i < 8; i++){
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
      HAL_Delay(250);
    }
  }
}

void init_Boton (void){

  GPIO_InitTypeDef GPIO_InitStruct;

  /*Enable clock to GPIO-C*/
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
	
	//implementamos el ISR para decirle en que pin tiene que gestionar las interrupciones
void EXTI15_10_IRQHandler(void){
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}
	
		//configuramos el callback, que es la rutina que seguir? el programa cada vez que haya una interrupci?n
	void HAL_GPIO_EXTI_Callback(uint16_t GPIO_PIN){
		
		//Avisamos de como debe actuar cuando la interrupci?n venga del PIN_13 (el del bot?n)
		if(GPIO_PIN == GPIO_PIN_13){
      RTC_Time_Config(0x00, 0x00, 0x00);
      RTC_Date_Config(0x01, 0x01, 0x00, RTC_WEEKDAY_SATURDAY);

		}
	}

