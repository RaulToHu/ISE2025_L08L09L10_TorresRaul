/**
ISE - P2
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

		void initRESET(void);																// pin reset spi
		void initCS(void);																	// pin CS spi
		void initA0(void);																	// pin A0 spi
		
		void delay(volatile uint32_t n_microsegundos);			// delay lcd
		void LCD_reset(void);																// reset lcd
		void LCD_wr_data(unsigned char data);								// escribe dato lcd		 		(dada SBM)
		void LCD_wr_cmd(unsigned char cmd);									// escribe comando lcd 		(dada SBM)
		void LCD_init(void);																// inicializa lcd					(dada SBM)
		void LCD_update(void);															// refresca pantalla lcd	(dada SBM)
		
		void LCD_symbolToLocalBuffer(uint8_t line,uint8_t symbol, uint8_t reset, uint16_t posicionL);		
    uint8_t LCD_buscar(char text[], uint8_t sel);																										// busca un caracter
    uint8_t LCD_centrar(char text[]);																																// centra el texto 
		
    void vaciarLCD(void);																// vacía el buffer del lcd
		void clk_enable(void);															// activa todos los enable de los buses
    void PINS_reset(void);															// resetea pines spi
		
    void LCD_escribirNormal(uint8_t column, uint8_t page, uint8_t value);			// escribe en línea y columna indicados

		extern osMessageQueueId_t IdqueueLCD;								// id EXTRERNO cola
		extern ARM_DRIVER_SPI Driver_SPI1; 								  // driver EXTERNO protocolo SPI
		
		typedef struct{						 // estructura de info que procesa la cola
			uint8_t linea;
			uint8_t reset;
			uint8_t select;
      char buffer[21];
		} infoLCD;

#endif // LCD_H
