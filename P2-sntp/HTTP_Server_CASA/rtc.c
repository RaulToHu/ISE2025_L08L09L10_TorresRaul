  /**
ISE - P2
Raúl Torres Huete
(LunesTarde)
  */

#include "rtc.h"
#include "lcd.h"

osMessageQueueId_t mid_MsgQueueRTC; 						// id de la cola
osThreadId_t tid_thRTC;                        	// id del RTC
osThreadId_t tid_thAlarm;                       // id de la alarma

RTC_HandleTypeDef rtc_Handler;
RTC_TimeTypeDef rtc_TimeConfig = {0};
RTC_DateTypeDef rtc_DateConfig = {0};
RTC_AlarmTypeDef alarm_Config  = {0};

extern char lcd_hora[20+1];				// almacenamiento de la hora
extern char lcd_fecha[20+1];			// almacenamiento de la fecha

uint8_t dia, mes, ano, diaSemana, hora, minuto, segundo;

void parpadeo_Led(void);

extern osThreadId_t IdqueueLCD;
extern infoLCD infoDisp;				// tipo que se envía por la cola al lcd

tipoDate infoDate;

void ThRTC(void *argument);                   // Thdisplay function
void ThAlarm (void *argument);

volatile bool alarm_Check = false;

/**
  * @brief  Inicialización del RTC
  * @param  None
  * @retval None
  */
void RTC_Init (void){
  
  /*##-1- LSE Initialization */  
  __HAL_RCC_RTC_ENABLE ();
  
  /*##-2- Configure the RTC peripheral #######################################*/
  /* Configure RTC prescaler and RTC data registers */
  rtc_Handler.Instance = RTC;
  rtc_Handler.Init.HourFormat = RTC_HOURFORMAT_24; 
  rtc_Handler.Init.AsynchPrediv = RTC_ASYNCH_PREDIV; 
  rtc_Handler.Init.SynchPrediv = RTC_SYNCH_PREDIV; 
  rtc_Handler.Init.OutPut = RTC_OUTPUT_DISABLE; 
  rtc_Handler.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH; 
  rtc_Handler.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN; 
  __HAL_RTC_RESET_HANDLE_STATE(&rtc_Handler); 
  
  if (HAL_RTC_Init(&rtc_Handler) != HAL_OK) {
    /* Initialization Error */
  }
  
    /*##-3- Check if Data stored in BackUp register1: No Need to reconfigure RTC#*/
  /* Read the Back Up Register 1 Data */
  if (HAL_RTCEx_BKUPRead(&rtc_Handler, RTC_BKP_DR1) != 0x32F2) {
    /* Configure RTC Calendar */
    //RTC_Time_Config(hora, minuto, segundo);
    //RTC_Date_Config(dia, mes, ano, diaSemana);
    //RTC_SetAlarm ();
  }
  else {
    /* Clear source Reset Flag */
    __HAL_RCC_CLEAR_RESET_FLAGS();
  }
}


int Init_hora (void) {
  
	//Creamos e iniciamos un nuevo hilo que asignamos al identificador del Thled1,
  tid_thRTC = osThreadNew(ThRTC, NULL, NULL);
  if (tid_thRTC == NULL) {
    return(-1);
  }
  
  tid_thAlarm = osThreadNew(ThAlarm, NULL, NULL);
  if (tid_thAlarm == NULL) {
    return(-1);
  }
 
  return(0);
}

void ThRTC (void *argument) {
  
  RTC_Init();
  RTC_SetAlarm();
  //RTC_Date_Config (dia, mes, ano, diaSemana);
  //RTC_Time_Config (hora, minuto, segundo);
	
  while (1) {
			osThreadYield();                            // suspend ThLCD
  }
}


void ThAlarm (void *argument) {
  
  osStatus_t status;                            // function return status
   
  while (1) {
    
    status = osThreadFlagsWait(0x08, osFlagsWaitAny, osWaitForever);
    
    for(uint8_t i = 0; i < 20; i++){
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
      HAL_Delay(250);
    }
			osThreadYield();                            // suspend ThLCD
  }
}

/**
  * @brief  Función que configura la fecha
  * @param  Dia, Mes, Año
  * @retval None
  */
void RTC_Date_Config (uint8_t dd, uint8_t ms, uint8_t yr, uint8_t wday){
  
  /*##-2- Configure the Date #################################################*/
  /* Set Date: Sunday March 02nd 2025 */
  rtc_DateConfig.Date = dd;
  rtc_DateConfig.Month = ms;
  rtc_DateConfig.Year = yr; 
  rtc_DateConfig.WeekDay = RTC_WEEKDAY_FRIDAY;
  
  if(HAL_RTC_SetDate(&rtc_Handler,&rtc_DateConfig,RTC_FORMAT_BCD) != HAL_OK){
    /* Initialization Error */
   // Error_Handler();
  }
  
  HAL_RTCEx_BKUPWrite(&rtc_Handler, RTC_BKP_DR1, 0x32F2);
}

void RTC_Show(uint8_t *showtime, uint8_t *showdate){
  
  /* Get the RTC current Time */
  HAL_RTC_GetTime(&rtc_Handler, &rtc_TimeConfig, RTC_FORMAT_BIN);
  /* Get the RTC current Date */
  HAL_RTC_GetDate(&rtc_Handler, &rtc_DateConfig, RTC_FORMAT_BIN);
  
  /* Display time Format : hh:mm:ss */
  sprintf((char *)showtime, "%02d:%02d:%02d", rtc_TimeConfig.Hours, rtc_TimeConfig.Minutes, rtc_TimeConfig.Seconds);
  strcpy(infoDisp.buffer,(char *)showtime);
  infoDisp.linea = 0;												// ... se asigna a la primera línea del lcd
  osMessageQueuePut(IdqueueLCD, &infoDisp, NULL, 100); 	// se mete en la cola

  /* Display date Format : mm-dd-yy */
  sprintf((char *)showdate, "%02d-%02d-%d", rtc_DateConfig.Date, rtc_DateConfig.Month, 2000 + rtc_DateConfig.Year);
  strcpy(infoDisp.buffer,(char *)showdate);
  infoDisp.linea = 1;												// ... se asigna a la primera línea del lcd
  osMessageQueuePut(IdqueueLCD, &infoDisp, NULL, 100); 	// se mete en la cola

}

/**
  * @brief  Función que configura la hora
  * @param  Hora, minutos, segundos, dia, mes y años a configurar
  * @retval None
  */
void RTC_Time_Config (uint8_t hh, uint8_t mm, uint8_t ss){
  
  /*##-1- Configure the Time #################################################*/
  /* Set Time: 18:24:02 */
  rtc_TimeConfig.Hours = hh;
  rtc_TimeConfig.Minutes = mm;
  rtc_TimeConfig.Seconds = ss;
  rtc_TimeConfig.TimeFormat = RTC_HOURFORMAT_24;
  rtc_TimeConfig.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  rtc_TimeConfig.StoreOperation = RTC_STOREOPERATION_RESET;
  
  if (HAL_RTC_SetTime(&rtc_Handler, &rtc_TimeConfig, RTC_FORMAT_BCD) != HAL_OK){
    /* Initialization Error */
   // Error_Handler();
  }
}


void RTC_SetAlarm (void){
  
  HAL_RTC_GetTime(&rtc_Handler, &rtc_TimeConfig, RTC_FORMAT_BCD);
  alarm_Config.AlarmTime.Hours = rtc_TimeConfig.Hours;
  alarm_Config.AlarmTime.Minutes = rtc_TimeConfig.Minutes;
  alarm_Config.AlarmTime.Seconds = 0;
  alarm_Config.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  alarm_Config.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  alarm_Config.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY | RTC_ALARMMASK_HOURS | RTC_ALARMMASK_MINUTES;
  alarm_Config.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  alarm_Config.AlarmDateWeekDay = 0x1;
  alarm_Config.Alarm = RTC_ALARM_A;
  
  HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn); 
  HAL_RTC_SetAlarm_IT(&rtc_Handler, &alarm_Config, RTC_FORMAT_BCD);
}

void RTC_Alarm_IRQHandler(void){
  
  HAL_RTC_AlarmIRQHandler(&rtc_Handler);
}

//Función callback de la alarma
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc) {
    osThreadFlagsSet(tid_thAlarm, 0x08);
}
