/**
ISE - P1
Raúl Torres Huete
(LunesTarde)
  */

#ifndef LCD_H

#include "cmsis_os2.h"

#include "RTE_Components.h"
#include <stdint.h>
#include "Driver_SPI.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_spi.h"

	#define LCD_H
  	
		int Init_display (void);

		void initRESET(void);
		void initCS(void);
		void initA0(void);
		void delay(volatile uint32_t n_microsegundos);
		void LCD_reset(void);
		void LCD_wr_data(unsigned char data);
		void LCD_wr_cmd(unsigned char cmd);
		void LCD_init(void);
		void LCD_update(void);
		void LCD_symbolToLocalBuffer(uint8_t line,uint8_t symbol, uint8_t reset, uint16_t posicionL);
		void LCD_modificarByteBuffer(uint8_t pos, uint8_t page, char byte);
    void vaciarLCD(void);
    void PINS_reset(void);
    uint8_t LCD_buscar(char text[], uint8_t sel);
    uint8_t LCD_centrar(char text[]);
    void LCD_escribirNormal(uint8_t column, uint8_t page, uint8_t value);


  extern osMessageQueueId_t IdqueueLCD;

		
		void clk_enable(void);
		
		extern ARM_DRIVER_SPI Driver_SPI1; //driver del protocolo SPI
		
		typedef struct{ //Estructura de un gpio para mayor comodidad
			uint8_t linea;
			uint8_t reset;
			uint8_t select;
      char buffer[21];
		} infoLCD;

#endif // LCD_H
