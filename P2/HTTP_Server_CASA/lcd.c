/**
ISE - P2
Raúl Torres Huete
(LunesTarde)
  */

#include "lcd.h"
#include "Arial12x12.h"
#include "string.h"
#include "cmsis_os2.h"                          // CMSIS RTOS header file  
#include "stdio.h"

#define MSGQUEUE_OBJECTS 16 												// tamaño de la cola

/* Private typedef -----------------------------------------------------------*/
extern ARM_DRIVER_SPI Driver_SPI1;									// instanciamos SPI1
static TIM_HandleTypeDef htim7; 										// creamos la estructura del timer7 (timer del delay)
static GPIO_InitTypeDef GPIO_InitStructurePIN; 			// creamos la estructura para configurar el pin 

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static ARM_DRIVER_SPI* SPIdrv = &Driver_SPI1;				// asignamos SPI1

static unsigned char buffer[800];										// buffer de char dónde se escribe de tamaño 800
uint8_t posicionL1 = 0;															// posición de la línea 1 (línea superior del lcd)
uint8_t posicionL2 = 0;															// posición de la línea 2 (línea inferior del lcd)

osThreadId_t tid_thlcd;                        			// id del hilo lcd
void ThLCD(void *argument);                   			// función que ejecuta el hilo
 
osMessageQueueId_t IdqueueLCD; 											// id de la cola
infoLCD datosLCD;																		// tipo de datos que se envían por la cola (struct declarado en lcd.h)
 
 
//Inicialización del hilo y llamada a funciones importantes
int Init_display (void) {
 
	clk_enable(); 
  LCD_reset();
  LCD_init();
  
  IdqueueLCD = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(infoLCD), NULL);
  
	//Creamos e iniciamos un nuevo hilo que asignamos al id antes instanciado,
	// le pasamos como parámetros la funcián que ejecutará
  tid_thlcd = osThreadNew(ThLCD, NULL, NULL);
  if (tid_thlcd == NULL) {
    return(-1);
  }
 
  return(0);
}
 

//Función a ejecutar por el hilo
void ThLCD (void *argument) {
  
	  LCD_symbolToLocalBuffer(NULL, NULL, 1, 0); 			// instrucción de escribir en el display

    uint32_t status;
    uint8_t buffer; 
  
    uint16_t offset;
  
    infoLCD info;

    uint8_t posicion;
  
    uint8_t select;

    osStatus_t statusQ; 

    clk_enable();
    LCD_reset();
    LCD_init();

    LCD_update();
  
  while (1) {
    
    status = osThreadFlagsGet();
    if(status == 2){
      
      PINS_reset();
      LCD_reset();
      LCD_init();
      
      LCD_symbolToLocalBuffer(NULL, NULL, 1, 0);
      LCD_update();
      
    }
		statusQ = osMessageQueueGet(IdqueueLCD, &info, NULL, 100);
    
    if(statusQ == osOK){
    
			LCD_symbolToLocalBuffer(NULL, NULL, info.reset, 0);
      
      posicion = LCD_centrar(info.buffer);
			
			if(!posicion){
				LCD_symbolToLocalBuffer(info.linea, NULL, 4, 0);
			}
			
			LCD_symbolToLocalBuffer(info.linea,info.buffer[0], info.linea + 2, posicion);
			
			posicion = posicion + Arial12x12[25*(info.buffer[0] - ' ')];
				
				for (int i = 1; info.buffer[i] != '\0'; i++) {
					posicion = posicion + Arial12x12[25*(info.buffer[i] - ' ')];
					if(posicion + 3 < 128){
						LCD_symbolToLocalBuffer(info.linea,info.buffer[i], 0, 0);
					}
				}
				if(info.select != 0){
    
					select = LCD_buscar(info.buffer, info.select);
      
					offset = 25*(info.buffer[info.select] - ' ');
			
					for(int i = 0; i < 5 ;i++){
						LCD_escribirNormal(select + i, 3, 0x04 + Arial12x12[offset + 4 + i*2]);
					}
				}
			LCD_update();
		}
			; // Insert ThLCD code here...
			osThreadYield();                            // suspend ThLCD
  }
}


//Inicialización del PIN RESET del SPI
void initRESET(void){	
	__HAL_RCC_GPIOA_CLK_ENABLE(); 										// habilitamos el reloj del bus (está en el PA6, es decir, bus A)
	GPIO_InitStructurePIN.Mode = GPIO_MODE_OUTPUT_PP; // configuramos el modo en pull-push salida
	GPIO_InitStructurePIN.Pin = GPIO_PIN_6; 					// asignamos el PIN_6 del GPIO que le digamos después
	
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructurePIN); 		// inicializamos el PIN con la configuración anterior,
																										// y lo asignamos al bus correspondiente (el antes habilitado)
}

//Inicialización del PIN CS del SPI
void initCS(void){
	__HAL_RCC_GPIOD_CLK_ENABLE(); 										// habilitamos el reloj del bus (está en el PD14, es decir, bus D)
	GPIO_InitStructurePIN.Mode = GPIO_MODE_OUTPUT_PP; // configuramos el modo en pull-push salida
	GPIO_InitStructurePIN.Pin = GPIO_PIN_14; 					// asignamos el PIN_14 del GPIO que le digamos después
	
	HAL_GPIO_Init(GPIOD, &GPIO_InitStructurePIN); 		// inicializamos el PIN con la configuración anterior,
																										// y lo asignamos al bus correspondiente (el antes habilitado)
}

//Inicialización del PIN A0 del SPI
void initA0(void){	
	__HAL_RCC_GPIOF_CLK_ENABLE(); 										// habilitamos el reloj del bus (está en el PF13, es decir, bus F)
	GPIO_InitStructurePIN.Mode = GPIO_MODE_OUTPUT_PP; // configuramos el modo en pull-push salida
	GPIO_InitStructurePIN.Pin = GPIO_PIN_13; 					// asignamos el PIN_13 del GPIO que le digamos después

	HAL_GPIO_Init(GPIOF, &GPIO_InitStructurePIN); 		// inicializamos el PIN con la configuración anterior,
																										// y lo asignamos al bus correspondiente (el antes habilitado) 
}

//Función que realiza delays en ms y un previo ajuste de frecuencia
void delay(uint32_t n_microsegundos){
	
	// Configurar y arrancar el timer para generar un evento pasados n_microsegundos
	htim7.Init.Prescaler = 83; 												// 84MHz/83 = (1MHz) 1000000Hz (siendo 84MHz en este caso, el valor de APB1 Timer clocks)
	htim7.Init.Period = n_microsegundos - 1; 					// 1MHz (resultado anterior) / n_microsegundos 
																										// Ej (n_microsegundos = 1000): 1MHz / 999 = 1000 Hz = 1 ms
																										// Ej (n_microsegundos = 10000): 1MHz / 1999 = 500 Hz = 2 ms
  
	HAL_TIM_Base_Init(&htim7);												// inicialización del timer
  HAL_TIM_Base_Start(&htim7);												// orden de START del timer
	
	// Esperar a que se active el flag del registro de Match correspondiente
	__HAL_TIM_CLEAR_FLAG(&htim7, TIM_FLAG_UPDATE); // Borrado preventivo
	while(__HAL_TIM_GET_FLAG(&htim7, TIM_FLAG_UPDATE) == RESET){}

	// Borrar el flag
	__HAL_TIM_CLEAR_FLAG(&htim7, TIM_FLAG_UPDATE);		
		
	// Parar el Timer y ponerlo a 0 para la siguiente llamada a la función
	HAL_TIM_Base_Stop(&htim7);
	__HAL_TIM_SET_COUNTER(&htim7, 0);
}

//Función que resetea el LCD
void LCD_reset(void){
	//Inicialización y configuración del SPI para realizar la gestión del LCD
	SPIdrv -> Initialize(NULL);
	SPIdrv -> PowerControl(ARM_POWER_FULL); //Ponemos el Power en ON
	
	//Configuramos parámetros de control: spi trabajando en modo máster, 
		// con configuración de inicio de transmisión CPOL a 1 y CPHA a 1,
		// organización de la información de most significant bit a least significant 
		// y número de bits por dato a 8.
		// Por último, la frecuencia del sclk a 20MHz
	SPIdrv -> Control(ARM_SPI_MODE_MASTER | ARM_SPI_CPOL1_CPHA1 | ARM_SPI_MSB_LSB | ARM_SPI_DATA_BITS(8), 20000000 );
	
	//En este caso configuramos el control de slaves de este SPI y decimos que está inactivo
	SPIdrv -> Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
	
	//Iniciamos los 3 pines que hemos configurado anteriormente con su valor por defecto:
	initRESET();
	initCS();
	initA0();
	
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, 1);
	
	//Generamos la señal de reset solicitada
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
	delay(1);
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
	delay(1000);
	
}
/**************************** AÑADIDO P3 - EJ 2 ********************************/

//Función que escribe datos en el LCD
void LCD_wr_data(unsigned char data){
	
	// Seleccionar CS = 0;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);
	// Seleccionar A0 = 1;
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, 1);

	// Escribir un dato (data) usando la función SPIDrv->Send(?);
	SPIdrv -> Send(&data, sizeof(data));
	// Esperar a que se libere el bus SPI;
	while(SPIdrv -> GetStatus().busy);
	
	// Seleccionar CS = 1;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);

}

//Función que escribe un comando en el LCD
void LCD_wr_cmd(unsigned char cmd){
	
	// Seleccionar CS = 0;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);
	// Seleccionar A0 = 0;
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, 0);
	
	// Escribir un comando (cmd) usando la función SPIDrv->Send(?);
	SPIdrv -> Send(&cmd, sizeof(cmd));
	// Esperar a que se libere el bus SPI;
	while(SPIdrv -> GetStatus().busy);
	
	// Seleccionar CS = 1;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);
}

/**************************** AÑADIDO P3 - EJ 3 ********************************/

//Función para inicializar el display
void LCD_init(void){

	LCD_wr_cmd(0xAE); //Pone el display a OFF
	
	LCD_wr_cmd(0xA2); //Fija el valor de la relaci?n de la tensi?n de polarizaci?n del LCD a 1/9 

	LCD_wr_cmd(0xA0); //Pone el direccionamiento de la RAM de datos del display en normal

	LCD_wr_cmd(0xC8); //Pone el scan en las salidas COM en normal

	LCD_wr_cmd(0x22); //Fija la relaci?n de resistencias interna a 2

	LCD_wr_cmd(0x2F); //Se activa el power a ON

	LCD_wr_cmd(0x40); //El display empieza en la l?nea 0

	LCD_wr_cmd(0xAF); //El display se pone a ON

	LCD_wr_cmd(0x81); //Se retoca el contraste

	LCD_wr_cmd(0x15); //Se decide el valor del contraste (A ELEGIR)

	LCD_wr_cmd(0xA4); //Se ponen todos los puntos del display en normal

	LCD_wr_cmd(0xA6); //Se pone el LCD de display en normal
	
}

/**************************** AÑADIDO P3 - EJ 4 ********************************/

//Función para actualizar la información que se visualizará en el display
void LCD_update(void){
	int i;
	LCD_wr_cmd(0x00); // 4 bits de la parte baja de la dirección a 0
	LCD_wr_cmd(0x10); // 4 bits de la parte alta de la dirección a 0
	LCD_wr_cmd(0xB0); // Página 0
	for(i=0;i<128;i++){
		LCD_wr_data(buffer[i]);
	}

	LCD_wr_cmd(0x00); // 4 bits de la parte baja de la dirección a 0
	LCD_wr_cmd(0x10); // 4 bits de la parte alta de la dirección a 0
	LCD_wr_cmd(0xB1); // Página 1
	for(i=128;i<256;i++){
		LCD_wr_data(buffer[i]);
	}

	LCD_wr_cmd(0x00);
	LCD_wr_cmd(0x10);
	LCD_wr_cmd(0xB2); //Página 2
	for(i=256;i<384;i++){
		LCD_wr_data(buffer[i]);
	}
	
	LCD_wr_cmd(0x00);
	LCD_wr_cmd(0x10);
	LCD_wr_cmd(0xB3); // Página 3
	for(i=384;i<512;i++){
		LCD_wr_data(buffer[i]);
	}
}

/**************************** AÑADIDO P4 ********************************/

//--------------------LCD_symbolToLocalBuffer---------------------                      ---GONZALO---
//Función que coge un símbolo y lo mete al buffer
void LCD_symbolToLocalBuffer(uint8_t line, uint8_t symbol, uint8_t reset, uint16_t posicionL){
	
  uint8_t i, value1, value2 = 0;
  uint16_t offset = 0;							

  if(symbol == NULL){				// comprobación y ajuste para sustituir símbolos nulos por ' '
    symbol = ' ';
  }
  
  offset = 25*(symbol - ' ');					// genera offset del símbolo (primeras 25 posiciones no son caracteres)
  static uint16_t posicionL1;					// indica posición en la línea superior
  static uint16_t posicionL2;					// indica posición en la línea inferior
	
	if(reset == 4){					// reinicia posición de escritura en la línea especificada
		if(!line){
			posicionL1 = 0;			// reinicia en línea superior
		}else{
			posicionL2 = 0;			// reinicia en línea inferior
		}
	}else{
		if(posicionL != 0){								// si posición de línea no 0
			if(!line){
			posicionL1 = posicionL - 1;			// posición línea superior retrocede una posición
			}else{
			posicionL2 = posicionL - 1;			// posición línea inferior retrocede una posición
			}
		}
		if(reset == 2){												// borra primeros 256 bytes (línea superior)
      for(int j = 0; j < 256; j++){
        buffer[j] = 0x00;
      }
    }else if(reset == 3){									// borra segundos 256 bytes (línea inferior)
      for(int j = 256; j < 512; j++){
        buffer[j] = 0x00;
      }
    }
    if(reset == 1){									// borra todo el buffer y reinicia posiciones
      posicionL1 = 0;
      posicionL2 = 0;
      for(int j = 0; j < 512; j++){
        buffer[j] = 0x00;
      }
    }else{
      if(!line){
        for (i = 0; i < 12; i++){			//trabaja con la línea superior del lcd
          value1 = Arial12x12[offset + i*2 + 1];		// extrae y guarda en value1 caracter correspondiente del arial
          value2 = Arial12x12[offset + i*2 + 2];		// extrae y guarda en value2 caracter correspondiente del arial
          buffer[i + 0 + posicionL1] = value1;			// asigna value1 en posición correspondiente a posiciónL1 de línea 0
          buffer[i + 128 +  posicionL1] = value2;		// asigna value2 en posición correspondiente a posiciónL1 de línea 1
        }
        posicionL1 = posicionL1 + Arial12x12[offset]; 	// reajusta la nueva posición
      }else{
        for (i=0;i<12;i++){						//trabaja con la línea inferior del lcd
          value1 = Arial12x12[offset + i*2 + 1];		// extrae y guarda en value1 caracter correspondiente del arial
          value2 = Arial12x12[offset + i*2 + 2];		// extrae y guarda en value2 caracter correspondiente del arial
          buffer[i + 256 + posicionL2] = value1;		// asigna value1 en posición correspondiente a posiciónL1 de línea 2
          buffer[i + 384 +  posicionL2] = value2;		// asigna value2 en posición correspondiente a posiciónL1 de línea 3
        }
        posicionL2 = posicionL2 + Arial12x12[offset];		// reajusta la nueva posición
      }
    }
	}
}


//Función que centra un valor en el display, devuelve la posición   		---GONZALO---
	//en la que se debe empezar a escribir para tener el texto centrado 
uint8_t LCD_centrar(char text[]){
  
  uint8_t ancho = 0;				// ancho del texto introducido
  uint8_t posicion = 0;			// posicion donde se va a escribir centrado
	uint16_t offset = 0;			
  
  for (int i = 0; (text[i] != '\0') && ((ancho + Arial12x12[offset]) < 128); i++) {		// para cada caracter del texto guarda
      offset = 25*(text[i] - ' ');																											// su ancho y lo añade al ancho total
      ancho = ancho + Arial12x12[offset];
  }
  if(ancho >= 128){					// si ancho supera límite de línea, devuelve 0 (no se puede centrar)
		return 0;
	}
  posicion = (128 - ancho) / 2;				// posicion es = lo que sobra sin caracteres / 2
  
  return posicion;
}
//Busca una posición en el display								---GONZALO---
uint8_t LCD_buscar(char text[], uint8_t sel){
  
  uint8_t posicion;
  uint8_t select;
  uint16_t offset = 0;
  
  for (int i = 0; text[i] != '\0'; i++) {
      offset = 25*(text[i] - ' ');
      posicion = posicion + Arial12x12[offset];
      if(i == (sel - 1)){
        select = posicion;
      }
  }
  
  posicion = (128 - posicion) / 2;
  select = select + posicion;
  
  return select;
}


////Función que vacía el LCD 											---AÑADIR AL CÓDIGO---
//void vaciarLCD(void) {
//	memset(buffer,0x00,512);
//	LCD_update();
//}


//Función para asegurar que todos los relojes están activados
void clk_enable(void){
  
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  
}

//Función que resetea al valor por defecto los 3 pines antes configurados
void PINS_reset(void){
  
  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6, 0);
  HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14, 0);
  HAL_GPIO_WritePin(GPIOF,GPIO_PIN_13, 0);
}


//Función para escribir fácil: columna, página y bit a rellenar
void LCD_escribirNormal(uint8_t column, uint8_t page, uint8_t value){
  
  buffer[column + page * 128] = value;
}

