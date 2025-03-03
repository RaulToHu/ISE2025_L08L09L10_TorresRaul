/**
ISE - P1
Raúl Torres Huete
(LunesTarde)
  */

#include "lcd.h"
#include "Arial12x12.h"
#include "string.h"
#include "cmsis_os2.h"                          // CMSIS RTOS header file  
#include "stdio.h"

#define MSGQUEUE_OBJECTS 16 //Define el tamaño de la cola

/* Private typedef -----------------------------------------------------------*/
extern ARM_DRIVER_SPI Driver_SPI1;
static TIM_HandleTypeDef htim7; //Creamos la estructura del timer7
static GPIO_InitTypeDef GPIO_InitStructurePIN; //Creamos la estructura para configurar el pin 

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static ARM_DRIVER_SPI* SPIdrv = &Driver_SPI1;

static unsigned char buffer[800];
uint8_t posicionL1 = 0;
uint8_t posicionL2 = 0;

osMessageQueueId_t IdqueueLCD; //Declaración del id de la cola

osThreadId_t tid_thlcd;                        // Thdisplay id
 
void ThLCD(void *argument);                   // Thdisplay function
 

infoLCD datosLCD;
 
int Init_display (void) {
 
	clk_enable(); 
  LCD_reset();
  LCD_init();
  
  IdqueueLCD = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(infoLCD), NULL);
  
	//Creamos e iniciamos un nuevo hilo que asignamos al identificador del Thled1,
	// le pasamos como par?metros la funci?n que ejecutar? el hilo y la informaci?n del gpio del led creado como puntero void
  tid_thlcd = osThreadNew(ThLCD, NULL, NULL);
  if (tid_thlcd == NULL) {
    return(-1);
  }
 
  return(0);
}
 
void ThLCD (void *argument) {
  
    uint32_t status;
    uint8_t buffer; 

    LCD_symbolToLocalBuffer(NULL, NULL, 1, 0); //Instrucci?n de escribir en el display
  
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

//Inicializaci?n del PIN RESET del SPI
void initRESET(void){	
	__HAL_RCC_GPIOA_CLK_ENABLE(); //Habilitamos el pin (est? en el PA6)

	GPIO_InitStructurePIN.Mode = GPIO_MODE_OUTPUT_PP; //Configuramos el modo en pull-push salida
	GPIO_InitStructurePIN.Pin = GPIO_PIN_6; //Asignamos el PIN_11 del GPIO que le digamos despu?s
	
	// relacionamos la configuraci?n anterior con el GPIO correspondiente e inicializamos,
	// resultando en que estamos configurando el PIN_6 del bus A (PB6)
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructurePIN); 
}

//Inicializaci?n del PIN CS del SPI
void initCS(void){
	__HAL_RCC_GPIOD_CLK_ENABLE(); //Habilitamos el pin (est? en el PD14)

	GPIO_InitStructurePIN.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructurePIN.Pin = GPIO_PIN_14; //Asignamos el PIN_14 del GPIO que le digamos despu?s
	
	// relacionamos la configuraci?n anterior con el GPIO correspondiente e inicializamos,
	// resultando en que estamos configurando el PIN_14 del bus D (PD14)
	HAL_GPIO_Init(GPIOD, &GPIO_InitStructurePIN); 
}

//Inicializaci?n del PIN A0 del SPI
void initA0(void){	
	__HAL_RCC_GPIOF_CLK_ENABLE(); //Habilitamos el pin (est? en el PF13)

	GPIO_InitStructurePIN.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructurePIN.Pin = GPIO_PIN_13; //Asignamos el PIN_13 del GPIO que le digamos despu?s
	
	// relacionamos la configuraci?n anterior con el GPIO correspondiente e inicializamos,
	// resultando en que estamos configurando el PIN_13 del bus F (PF13)
	HAL_GPIO_Init(GPIOF, &GPIO_InitStructurePIN); 
}

void delay(uint32_t n_microsegundos){
	// Configurar y arrancar el timer para generar un evento pasados n_microsegundos
	htim7.Init.Prescaler = 83; //84MHz/83 = (1MHz) 1000000Hz (siendo 84MHz en este caso, el valor de APB1 Timer clocks)
	htim7.Init.Period = n_microsegundos - 1; // 1MHz (resultado anterior) / n_microsegundos 
																					 //Ej (n_microsegundos = 1000): 1MHz / 999 = 1000 Hz = 1 ms
																					 //Ej (n_microsegundos = 10000): 1MHz / 1999 = 500 Hz = 2 ms
  
	HAL_TIM_Base_Init(&htim7);
  HAL_TIM_Base_Start(&htim7);
	
	// Esperar a que se active el flag del registro de Match correspondiente
	__HAL_TIM_CLEAR_FLAG(&htim7, TIM_FLAG_UPDATE); //Borrado preventivo
	while(__HAL_TIM_GET_FLAG(&htim7, TIM_FLAG_UPDATE) == RESET){}

	// Borrar el flag
	__HAL_TIM_CLEAR_FLAG(&htim7, TIM_FLAG_UPDATE);
		
	// Parar el Timer y ponerlo a 0 para la siguiente llamada a la funci?n
	HAL_TIM_Base_Stop(&htim7);
	__HAL_TIM_SET_COUNTER(&htim7, 0);
}


void LCD_reset(void){
	//Inicializaci?n y configuraci?n del SPI para realizar la gesti?n del LCD
	SPIdrv -> Initialize(NULL);
	SPIdrv -> PowerControl(ARM_POWER_FULL); //Ponemos el Power en ON
	
	//Configuramos par?metros de control: spi trabajando en modo m?ster, 
	// con configuraci?n de inicio de transmisi?n CPOL a 1 y CPHA a 1,
	// organizaci?n de la informaci?n de most significant bit a least significant 
	// y n?mero de bits por dato a 8.
	// Por ?ltimo, la frecuencia del sclk a 20MHz
	SPIdrv -> Control(ARM_SPI_MODE_MASTER | ARM_SPI_CPOL1_CPHA1 | ARM_SPI_MSB_LSB | ARM_SPI_DATA_BITS(8), 20000000 );
	
	//En este caso configuramos el control de slaves de este SPI y decimos que est? inactivo
	SPIdrv -> Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
	
	//Iniciamos los 3 pines que hemos configurado anteriormente con su valor por defecto:
	initRESET();
	initCS();
	initA0();
	
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, 1);
	
	//Generamos la se?al de reset solicitada
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
	delay(1);
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
	delay(1000);
	
}


void PINS_reset(void){
  
  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6, 0);
  HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14, 0);
  HAL_GPIO_WritePin(GPIOF,GPIO_PIN_13, 0);
}

/**************************** A?ADIDO P3 - EJ 2 ********************************/

//Funci?n que escribe un comando en el LCD
void LCD_wr_data(unsigned char data){
	
	// Seleccionar CS = 0;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);
	// Seleccionar A0 = 1;
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, 1);

	// Escribir un dato (data) usando la funci?n SPIDrv->Send(?);
	SPIdrv -> Send(&data, sizeof(data));
	// Esperar a que se libere el bus SPI;
	while(SPIdrv -> GetStatus().busy);
	
	// Seleccionar CS = 1;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);

}

//Funci?n que escribe un comando en el LCD
void LCD_wr_cmd(unsigned char cmd){
	
	// Seleccionar CS = 0;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 0);
	// Seleccionar A0 = 0;
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, 0);
	
	// Escribir un comando (cmd) usando la funci?n SPIDrv->Send(?);
	SPIdrv -> Send(&cmd, sizeof(cmd));
	// Esperar a que se libere el bus SPI;
	while(SPIdrv -> GetStatus().busy);
	
	// Seleccionar CS = 1;
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, 1);
}

/**************************** A?ADIDO P3 - EJ 3 ********************************/

//Funci?n para inicializar el display
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

	LCD_wr_cmd(0x15); //Se decide el valor del contraste (a elegir)

	LCD_wr_cmd(0xA4); //Se ponen todos los puntos del display en normal

	LCD_wr_cmd(0xA6); //Se pone el LCD de display en normal
	
}

/**************************** A?ADIDO P3 - EJ 4 ********************************/

//Funci?n para ctualizar la informaci?n que se visualizar? en el display
void LCD_update(void){
	int i;
	LCD_wr_cmd(0x00); // 4 bits de la parte baja de la direcci?n a 0
	LCD_wr_cmd(0x10); // 4 bits de la parte alta de la direcci?n a 0
	LCD_wr_cmd(0xB0); // P?gina 0
	for(i=0;i<128;i++){
		LCD_wr_data(buffer[i]);
	}

	LCD_wr_cmd(0x00); // 4 bits de la parte baja de la direcci?n a 0
	LCD_wr_cmd(0x10); // 4 bits de la parte alta de la direcci?n a 0
	LCD_wr_cmd(0xB1); // P?gina 1
	for(i=128;i<256;i++){
		LCD_wr_data(buffer[i]);
	}

	LCD_wr_cmd(0x00);
	LCD_wr_cmd(0x10);
	LCD_wr_cmd(0xB2); //P?gina 2
	for(i=256;i<384;i++){
		LCD_wr_data(buffer[i]);
	}
	
	LCD_wr_cmd(0x00);
	LCD_wr_cmd(0x10);
	LCD_wr_cmd(0xB3); // Pagina 3
	for(i=384;i<512;i++){
		LCD_wr_data(buffer[i]);
	}
}

/**************************** A?ADIDO P4 ********************************/

//--------------------LCD_symbolToLocalBuffer---------------------
//Coge un simbolo y lo mete al buffer
void LCD_symbolToLocalBuffer(uint8_t line, uint8_t symbol, uint8_t reset, uint16_t posicionL){
  uint8_t i, value1, value2 = 0;
  uint16_t offset = 0;

  if(symbol == NULL){
    symbol = ' ';
  }
  
  offset = 25*(symbol - ' ');
  static uint16_t posicionL1;
  static uint16_t posicionL2;
	
	if(reset == 4){
		if(!line){
			posicionL1 = 0;
		}else{
			posicionL2 = 0;
		}
	}else{
		if(posicionL != 0){
			if(!line){
			posicionL1= posicionL - 1;
			}else{
			posicionL2= posicionL - 1;
			}
		}
		if(reset == 2){
		
      for(int j=0;j<256;j++){
        buffer[j] = 0x00;
      }
    }else if(reset == 3){
      for(int j=256;j<512;j++){
        buffer[j] = 0x00;
      }
    }
    
    
    if(reset == 1){
      posicionL1 = 0;
      posicionL2 = 0;
      for(int j=0;j<512;j++){
        buffer[j] = 0x00;
      }
    }else{
      if(!line){
        for (i=0;i<12;i++){
          value1 = Arial12x12[offset+i*2+1];
          value2 = Arial12x12[offset+i*2+2];
          buffer[i + 0 + posicionL1] = value1;
          buffer[i + 128 +  posicionL1] = value2;
        }
        posicionL1 = posicionL1 + Arial12x12[offset];
      }else{
        for (i=0;i<12;i++){
          value1 = Arial12x12[offset+i*2+1];
          value2 = Arial12x12[offset+i*2+2];
          buffer[i + 256 + posicionL2]=value1;
          buffer[i + 384 +  posicionL2]=value2;
        }
        posicionL2 = posicionL2 + Arial12x12[offset];
      }
    }
	}
}

//Centra un valor en el display

uint8_t LCD_centrar(char text[]){
  
  uint8_t posicion = 0;
  uint16_t offset = 0;
  
  for (int i = 0; (text[i] != '\0') && ((posicion + Arial12x12[offset]) < 128); i++) {
      offset = 25*(text[i] - ' ');
      posicion = posicion + Arial12x12[offset];
  }
  if(posicion >= 128){
		return 0;
	}
  posicion = (128 - posicion) / 2;
  
  return posicion;
}
//Busca una posición en el display

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


//Función que vacía el LCD
void vaciarLCD(void)
{
	memset(buffer,0x00,512);
	LCD_update();
}


//Funci?n para asegurar que todos los relojes est?n activados
void clk_enable(void){
  
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  
}

void LCD_escribirNormal(uint8_t column, uint8_t page, uint8_t value){
  
  buffer[column + page * 128] = value;
}

