  /**
ISE - P2
Raúl Torres Huete
(LunesTarde)
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RTC_H
#define __RTC_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <time.h>
#include "stdio.h"
#include "string.h"
#include "stdbool.h"

#ifdef _RTE_
#include "RTE_Components.h"             // Component selection
#endif
#ifdef RTE_CMSIS_RTOS2                  // when RTE component CMSIS RTOS2 is used
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
#endif

/* Exported constants --------------------------------------------------------*/
/* Defines related to Clock configuration */
#define RTC_ASYNCH_PREDIV  0x7F   /* LSE as RTC clock */ //(Valor = 127)
#define RTC_SYNCH_PREDIV   0x00FF /* LSE as RTC clock */ //(Valor = 255)

/* Exported variables --------------------------------------------------------*/
extern RTC_HandleTypeDef rtc_Handler;
extern volatile bool alarm_Check;
extern RTC_TimeTypeDef rtc_TimeConfig;
extern RTC_DateTypeDef rtc_DateConfig;

typedef struct{
  char hora[10];
  char fecha[10];
}tipoDate;

/* Exported functions --------------------------------------------------------*/
void RTC_Init (void);
void RTC_Time_Config (uint8_t hh, uint8_t mm, uint8_t ss);
void RTC_Date_Config (uint8_t dd, uint8_t ms, uint8_t yr, uint8_t wday);
void RTC_Show(uint8_t *showtime, uint8_t *showdate);
static void init_LSE_Clock (void);
void RTC_SetAlarm (void);

#endif /* __RTC_H */