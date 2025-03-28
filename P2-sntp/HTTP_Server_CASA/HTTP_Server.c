/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network
 * Copyright (c) 2004-2019 Arm Limited (or its affiliates). All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    HTTP_Server.c
 * Purpose: HTTP Server example
 *----------------------------------------------------------------------------*/
 
 /**
ISE - P2
Raúl Torres Huete
(LunesTarde)
  */

#include <stdio.h>

#include "main.h"
#include "rl_net.h"                     // Keil.MDK-Pro::Network:CORE

#include "stm32f4xx_hal.h"              // Keil::Device:STM32Cube HAL:Common
#include "Board_Buttons.h"              // ::Board Support:Buttons
#include "Board_GLCD.h"                 // ::Board Support:Graphic LCD  					-- BORRAR --

#include "leds.h"         // incluimos nuestro fichero de los leds (P1)
#include "lcd.h"					// incluimos nuestro fichero del lcd (P1)
#include "adc.h"					// incluimos nuestro fichero del adc (P1)
#include "rtc.h"					// incluimos nuestro fichero del rtc (P2)
#include "sntp.h"					// incluimos nuestro fichero del sntp (P2)


// Main stack size must be multiple of 8 Bytes - configuración de pila
#define APP_MAIN_STK_SZ (1024U)
uint64_t app_main_stk[APP_MAIN_STK_SZ / 8];
const osThreadAttr_t app_main_attr = {
  .stack_mem  = &app_main_stk[0],
  .stack_size = sizeof(app_main_stk)
};

extern GLCD_FONT GLCD_Font_6x8;
extern GLCD_FONT GLCD_Font_16x24;

extern uint16_t AD_in          (uint32_t ch);				// adc
extern uint8_t  get_button     (void);
extern void     netDHCP_Notify (uint32_t if_num, uint8_t option, const uint8_t *val, uint32_t len);

extern bool LEDrun;									// indica si leds encendidos o no
extern char lcd_text[2][20+1];			// almacena 2 líneas de texto para el lcd

char lcd_buffer[20+1];

extern char lcd_hora[20+1];			// almacenamiento de la hora
extern char lcd_fecha[20+1];			// almacenamiento de la fecha

extern osThreadId_t IdqueueLCD;
extern osThreadId_t TID_Led;

// timer virtual del rtc
osTimerId_t tim_id, tim_id_sntp;                            // timer id
static void Timer_Callback (void const *arg);
static void Timer_Callback_3min (void const *arg);
static uint32_t exec2, exec3;

infoLCD infoDisp;				// tipo que se envía por la cola al lcd
bool LEDrun;
char lcd_text[2][20+1] = { "Casa Buho",
                           "Amity"};

extern struct tm infoSNTP;

/* Thread IDs */
osThreadId_t TID_Display;
osThreadId_t TID_Led;

/* Thread declarations */
static void BlinkLed (void *arg);
static void Display  (void *arg);

__NO_RETURN void app_main (void *arg);

/* Read analog inputs */
//Función que lee la señal analógica del canal elegido (en este caso ADC1)
uint16_t AD_in (uint32_t ch) {
  static uint32_t val = 0;
  ADC_HandleTypeDef adchandle;

  if (ch == 0) {
    ADC_Init_Conversion_Single(&adchandle, ADC1);		//inicializa el adc del canal ADC1
  }
  
  val = ADC_getVolt(&adchandle, 10);		// convierte la señal leída en un valor de voltaje
	
  return (uint16_t)val;						// retorno con cast a entero de 16 bits que es el que necesitamos
}

/* Read digital inputs */
//NO IMPLEMENTADA NI RETOCADA EN B1: Función que lee el estado de los botones físicos
uint8_t get_button (void) {
  return ((uint8_t)Buttons_GetState ());
}


/* IP address change notification */
// NO IMPLEMENTADA NI RETOCADA EN B1: Función que detecta cambios en IP
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
//Función que maneja el lcd, envía mensajes a la cola del lcd
static __NO_RETURN void Display (void *arg) {

  (void)arg;

  infoDisp.reset = 0;			// desactiva el reset del display
  
  strcpy(infoDisp.buffer,"00:00:00");		// escribe ese texto en el buffer y...
  infoDisp.linea = 0;												// ... se asigna a la primera línea del lcd
  osMessageQueuePut(IdqueueLCD, &infoDisp, NULL, 100); 	// se mete en la cola
  
  strcpy(infoDisp.buffer,"01-01-2000");		// escribe ese texto en el buffer y...
  infoDisp.linea = 1;												// ... se asigna a la segunda línea del lcd
  osMessageQueuePut(IdqueueLCD, &infoDisp, NULL, 100);	// se mete en la cola
}


/*----------------------------------------------------------------------------
  Thread 'BlinkLed': Blink the LEDs on an eval board
 *---------------------------------------------------------------------------*/
//Función que maneja los leds, se encarga del bucle "Running LEDs" del servidor web
static __NO_RETURN void BlinkLed (void *arg) {
  const uint8_t led_val[16] = { 0x48,0x88,0x84,0x44,0x42,0x22,0x21,0x11,
                                0x12,0x0A,0x0C,0x14,0x18,0x28,0x30,0x50 };
  uint32_t cnt = 0U;

  (void)arg;

  LEDrun = false; 					// asigna true a LEDrun (siempre entrará al bucle) tras reset
  while(1) {
    /* Every 100 ms */
    if (LEDrun == true) {				// ejecución del bucle si LEDRUN es true
      LED_SetOut (led_val[cnt]);
      if (++cnt >= sizeof(led_val)) {
        cnt = 0U;
      }
    }
    osDelay (100);	// cada led dura encendido 100ms
  }
}

/*----------------------------------------------------------------------------
  Main Thread 'main': Run Network
 *---------------------------------------------------------------------------*/
//Función principal, inicializa todos los módulos y crea las tareas que se ejecutarán de inicio
__NO_RETURN void app_main (void *arg) {
  (void)arg;
  
   osStatus_t status;                            // function return status


  LED_Initialize();								// inicialización de los leds
  //Buttons_Initialize();
  ADC1_pins_F429ZI_config();			// incialización del adc
  Init_display();									// inicialización del display lcd
  netInitialize();                // inicialización del servidor
  init_Boton();                   // inicialización del botón
  Init_hora();										// inicialización del rtc
  sntp_Init();                    // inicialización del sntp
  
  // Create periodic timer
  exec2 = 2U;
  tim_id = osTimerNew((osTimerFunc_t)&Timer_Callback, osTimerPeriodic, &exec2, NULL);
  status = osTimerStart(tim_id, 1000U);  

  // Create periodic timer
  exec3 = 3U;
  tim_id_sntp = osTimerNew((osTimerFunc_t)&Timer_Callback_3min, osTimerPeriodic, &exec3, NULL);
  status = osTimerStart(tim_id_sntp, 180000U);  
  
  TID_Led     = osThreadNew (BlinkLed, NULL, NULL);		// tarea del bucle de iniciación de los leds
  //TID_Display = osThreadNew (Display,  NULL, NULL);		// tarea que imprime en el lcd las 2 líneas configuradas más arriba

  osThreadExit();
}



static void Timer_Callback (void const *arg) {
  // add user code here
  RTC_Show(lcd_hora, lcd_fecha);
}

static void Timer_Callback_3min (void const *arg) {
  // add user code here
  sntp_Init();
}
