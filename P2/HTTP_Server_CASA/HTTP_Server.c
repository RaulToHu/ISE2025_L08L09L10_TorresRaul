/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network
 * Copyright (c) 2004-2019 Arm Limited (or its affiliates). All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    HTTP_Server.c
 * Purpose: HTTP Server example
 *----------------------------------------------------------------------------*/
 
 /**
ISE - P1
Raúl Torres Huete
(LunesTarde)
  */

#include <stdio.h>

#include "main.h"

#include "rl_net.h"                     // Keil.MDK-Pro::Network:CORE

#include "stm32f4xx_hal.h"              // Keil::Device:STM32Cube HAL:Common
#include "leds.h"                  // ::Board Support:LED
#include "lcd.h"
#include "adc.h"
#include "rtc.h"
#include "Board_Buttons.h"              // ::Board Support:Buttons
//#include "Board_ADC.h"                  // ::Board Support:A/D Converter
#include "Board_GLCD.h"                 // ::Board Support:Graphic LCD
//#include "GLCD_Config.h"                // Keil.MCBSTM32F400::Board Support:Graphic LCD

// Main stack size must be multiple of 8 Bytes
#define APP_MAIN_STK_SZ (1024U)
uint64_t app_main_stk[APP_MAIN_STK_SZ / 8];
const osThreadAttr_t app_main_attr = {
  .stack_mem  = &app_main_stk[0],
  .stack_size = sizeof(app_main_stk)
};

extern GLCD_FONT GLCD_Font_6x8;
extern GLCD_FONT GLCD_Font_16x24;

extern uint16_t AD_in          (uint32_t ch);
extern uint8_t  get_button     (void);
extern void     netDHCP_Notify (uint32_t if_num, uint8_t option, const uint8_t *val, uint32_t len);

extern bool LEDrun;
extern char lcd_text[2][20+1];

char lcd_buffer[20+1];

extern osThreadId_t IdqueueLCD;
extern osThreadId_t TID_Led;


static infoLCD infoDisp;
bool LEDrun;
char lcd_text[2][20+1] = { "10:40:02",
                           "03/03/2025" };

/* Thread IDs */
osThreadId_t TID_Display;
osThreadId_t TID_Led;

/* Thread declarations */
static void BlinkLed (void *arg);
static void Display  (void *arg);

__NO_RETURN void app_main (void *arg);

/* Read analog inputs */
uint16_t AD_in (uint32_t ch) {
  static float val = 0;
  static uint16_t value;
  ADC_HandleTypeDef adchandle;

  if (ch == 0) {
    ADC_Init_Conversion_Single(&adchandle, ADC1);
  }
  
  val = ADC_getVolt(&adchandle, 10);
  value = (uint16_t)val;
  return (uint16_t)(val*1250);
}

/* Read digital inputs */
uint8_t get_button (void) {
  return ((uint8_t)Buttons_GetState ());
}

/* IP address change notification */
void netDHCP_Notify (uint32_t if_num, uint8_t option, const uint8_t *val, uint32_t len) {

  (void)if_num;
  (void)val;
  (void)len;

  if (option == NET_DHCP_OPTION_IP_ADDRESS) {
    /* IP address change, trigger LCD update */
    osThreadFlagsSet (TID_Display, 0x01);
  }
}

///*----------------------------------------------------------------------------
//  Thread 'Display': LCD display handler
// *---------------------------------------------------------------------------*/
static __NO_RETURN void Display (void *arg) {
  //char lcd_buf[20+1]; int i; --> Mejor como variable global para poder utilizar el watch.

  (void)arg;

  infoDisp.reset = 0;
  
  strcpy(infoDisp.buffer,"Ted Lasso");
  //strcpy (msgLcd.Buffer,lcd_text[0]);
  infoDisp.linea = 0;
  osMessageQueuePut(IdqueueLCD, &infoDisp, NULL, 100);
  
  strcpy(infoDisp.buffer,"Casa Búho");
  infoDisp.linea = 1;
  osMessageQueuePut(IdqueueLCD, &infoDisp, NULL, 100);
}


/*----------------------------------------------------------------------------
  Thread 'BlinkLed': Blink the LEDs on an eval board
 *---------------------------------------------------------------------------*/
static __NO_RETURN void BlinkLed (void *arg) {
  const uint8_t led_val[16] = { 0x48,0x88,0x84,0x44,0x42,0x22,0x21,0x11,
                                0x12,0x0A,0x0C,0x14,0x18,0x28,0x30,0x50 };
  uint32_t cnt = 0U;

  (void)arg;

  LEDrun = true;
  while(1) {
    /* Every 100 ms */
    if (LEDrun == true) {
      LED_SetOut (led_val[cnt]);
      if (++cnt >= sizeof(led_val)) {
        cnt = 0U;
      }
    }
    osDelay (100);
  }
}

/*----------------------------------------------------------------------------
  Main Thread 'main': Run Network
 *---------------------------------------------------------------------------*/
__NO_RETURN void app_main (void *arg) {
  (void)arg;

  LED_Initialize();
  Buttons_Initialize();
  ADC1_pins_F429ZI_config();
  Init_display();

  //ADC_Initialize();

  netInitialize ();
  
  Init_hora();

  TID_Led     = osThreadNew (BlinkLed, NULL, NULL);
  TID_Display = osThreadNew (Display,  NULL, NULL);

  osThreadExit();
}
