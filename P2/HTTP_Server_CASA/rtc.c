  /**
ISE - P2
Raúl Torres Huete
(LunesTarde)
  */

#include "rtc.h"

osMessageQueueId_t mid_MsgQueueRTC; // id de la cola
osThreadId_t tid_thRTC;                        // RTC id

RTC_HandleTypeDef rtc_Handler;
RTC_TimeTypeDef rtc_TimeConfig = {0};
RTC_DateTypeDef rtc_DateConfig = {0};
RTC_AlarmTypeDef alarm_Config  = {0};

uint8_t errDia = 0;
uint8_t errHora = 0;
uint8_t errLSE = 0;
uint8_t errPeriferico = 0;
uint8_t errQueue = 0;

tipoDate infoDate;

void ThRTC(void *argument);                   // Thdisplay function

volatile bool alarm_Check = false;

/**
  * @brief  Inicialización del RTC
  * @param  None
  * @retval None
  */
void RTC_Init (void){
  
  /*##-1- LSE Initialization */
  init_LSE_Clock ();
  
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
  
  if (HAL_RTC_Init(&rtc_Handler) != HAL_OK)
  {
    /* Initialization Error */
  }
  
    /*##-3- Check if Data stored in BackUp register1: No Need to reconfigure RTC#*/
  /* Read the Back Up Register 1 Data */
  if (HAL_RTCEx_BKUPRead(&rtc_Handler, RTC_BKP_DR1) != 0x32F2)
  {
    /* Configure RTC Calendar */
//    RTC_Time_Config(hora, min, seg);
//    RTC_Date_Config(dia, mes, anio);
    RTC_SetAlarm ();
  }
  else
  {
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
 
  return(0);
}

void ThRTC (void *argument) {
  
  uint8_t dia, mes, ano, diaSemana, hora, minuto, segundo;
  
  RTC_Init();
  init_LSE_Clock();
  RTC_SetAlarm();
  RTC_Date_Config (dia, mes, ano, diaSemana);
  RTC_Time_Config (hora, minuto, segundo);

  
  while (1) {
    
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
  rtc_DateConfig.Date = 0x01;
  rtc_DateConfig.Month = 0x01;
  rtc_DateConfig.Year = 0x00; 
  rtc_DateConfig.WeekDay = RTC_WEEKDAY_FRIDAY;
  
  if(HAL_RTC_SetDate(&rtc_Handler,&rtc_DateConfig,RTC_FORMAT_BCD) != HAL_OK)
  {
    /* Initialization Error */
   // Error_Handler();
    errDia += 1;
    while(1);
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
  /* Display date Format : mm-dd-yy */
  sprintf((char *)showdate, "%02d-%02d-%d", rtc_DateConfig.Date, rtc_DateConfig.Month, 2000 + rtc_DateConfig.Year);
}

/**
  * @brief  Función que configura la hora
  * @param  Hora, minutos, segundos, dia, mes y años a configurar
  * @retval None
  */
void RTC_Time_Config (uint8_t hh, uint8_t mm, uint8_t ss){
  
  /*##-1- Configure the Time #################################################*/
  /* Set Time: 18:24:02 */
  rtc_TimeConfig.Hours = 0x00;
  rtc_TimeConfig.Minutes = 0x00;
  rtc_TimeConfig.Seconds = 0x00;
  rtc_TimeConfig.TimeFormat = RTC_HOURFORMAT_24;
  rtc_TimeConfig.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  rtc_TimeConfig.StoreOperation = RTC_STOREOPERATION_RESET;
  
  if (HAL_RTC_SetTime(&rtc_Handler, &rtc_TimeConfig, RTC_FORMAT_BCD) != HAL_OK)
  {
    /* Initialization Error */
    errHora += 1;
    while(1);
  }
}

static void init_LSE_Clock (void){
  
  RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct = {0};
  RCC_OscInitTypeDef        RCC_OscInitStruct = {0};
  
  /* LSE Enable */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
//  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    errLSE += 1;
    while (1);  
  }
  
  /* Select LSE as RTC source clock*/
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
    errPeriferico += 1;
    while (1);  
  }
}

void RTC_SetAlarm (void){
  
  HAL_RTC_GetTime(&rtc_Handler, &rtc_TimeConfig, RTC_FORMAT_BIN);
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
  HAL_RTC_SetAlarm_IT(&rtc_Handler, &alarm_Config, RTC_FORMAT_BIN);
}
