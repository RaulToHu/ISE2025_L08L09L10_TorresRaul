/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network:Service
 * Copyright (c) 2004-2018 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    HTTP_Server_CGI.c
 * Purpose: HTTP Server CGI Module
 * Rev.:    V6.0.0
 *----------------------------------------------------------------------------*/
 
 /**
ISE - P2
Raúl Torres Huete
(LunesTarde)
  */

#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
#include "rl_net.h"                     // Keil.MDK-Pro::Network:CORE

#include "leds.h"         // incluimos nuestro fichero de los leds (P1)
#include "lcd.h"					// incluimos nuestro fichero del lcd (P1)
#include "adc.h"					// incluimos nuestro fichero del adc (P1)
#include "rtc.h"					// incluimos nuestro fichero del rtc (P2)
#include "sntp.h"					// incluimos nuestro fichero del sntp (P2)


#if      defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma  clang diagnostic push
#pragma  clang diagnostic ignored "-Wformat-nonliteral"
#endif

// http_server.c
extern uint16_t AD_in (uint32_t ch);
extern uint8_t  get_button (void);

extern bool LEDrun;			// variable externa (del HTTP_SERVER.c) que activa o desatciva el bucle de los leds

extern char lcd_text[2][20+1];			// almacenamiento de las dos líneas de texto del lcd
extern osThreadId_t TID_Display;		// hilo externo del lcd

char lcd_hora[20+1];			// almacenamiento de la hora
char lcd_fecha[20+1];			// almacenamiento de la fecha

uint8_t dec2bcd(uint8_t val);

// Local variables.
static uint8_t estadoLEDS;										// almacena el estado de los leds
static uint8_t ip_addr[NET_ADDR_IP6_LEN];			// dirección ip procesada
static char    ip_string[40];									// dirección ip en texto

static infoLCD infoDisp;					// tipo que se envía/recibe por la cola del lcd/http_server

// My structure of CGI status variable.
typedef struct {
  uint8_t idx;
  uint8_t unused[3];
} MY_BUF;
#define MYBUF(p)        ((MY_BUF *)p)

// Process query string received by GET request
//Función que procesa las consultas CGI recibidas por un GET request
void netCGI_ProcessQuery (const char *qstr) {
  netIF_Option opt = netIF_OptionMAC_Address;
  int16_t      typ = 0;
  char var[40];

  do {
    // Loop through all the parameters
    qstr = netCGI_GetEnvVar (qstr, var, sizeof (var));
    // Check return string, 'qstr' now points to the next parameter

    switch (var[0]) {
      case 'i': // Local IP address
        if (var[1] == '4') { opt = netIF_OptionIP4_Address;       }
        else               { opt = netIF_OptionIP6_StaticAddress; }
        break;

      case 'm': // Local network mask
        if (var[1] == '4') { opt = netIF_OptionIP4_SubnetMask; }
        break;

      case 'g': // Default gateway IP address
        if (var[1] == '4') { opt = netIF_OptionIP6_DefaultGateway; }
        else               { opt = netIF_OptionIP6_DefaultGateway; }
        break;

      case 'p': // Primary DNS server IP address
        if (var[1] == '4') { opt = netIF_OptionIP4_PrimaryDNS; }
        else               { opt = netIF_OptionIP6_PrimaryDNS; }
        break;

      case 's': // Secondary DNS server IP address
        if (var[1] == '4') { opt = netIF_OptionIP4_SecondaryDNS; }
        else               { opt = netIF_OptionIP6_SecondaryDNS; }
        break;
      
      default: var[0] = '\0'; break;
    }

    switch (var[1]) {
      case '4': typ = NET_ADDR_IP4; break;
      case '6': typ = NET_ADDR_IP6; break;

      default: var[0] = '\0'; break;
    }

    if ((var[0] != '\0') && (var[2] == '=')) {
      netIP_aton (&var[3], typ, ip_addr);
      // Set required option
      netIF_SetOption (NET_IF_CLASS_ETH, opt, ip_addr, sizeof(ip_addr));
    }
  } while (qstr);
}

// Process data received by POST request.
// Type code: - 0 = www-url-encoded form data.
//            - 1 = filename for file upload (null-terminated string).
//            - 2 = file upload raw data.
//            - 3 = end of file upload (file close requested).
//            - 4 = any XML encoded POST data (single or last stream).
//            - 5 = the same as 4, but with more XML data to follow.
//Función que procesa los datos enviados por el cliente al servidor mediante solicitud POST
void netCGI_ProcessData (uint8_t code, const char *data, uint32_t len) {
  char var[40],passw[12]; // var almacena variables y passw la contraseña

  if (code != 0) {			// si formato no url, detiene función
    // Ignore all other codes
    return;
  }

  estadoLEDS = 0;			// resetea el estado de los leds a 0 (todos apagados)
  //LEDrun = true;			// activa de inicio el bucle de los leds
	
  if (len == 0) {			// no se han enviado datos en solicitud POST
    LED_SetOut (estadoLEDS);			// se actualizan leds a todos apagados
    return;
  }
  passw[0] = 1;
  do {
    // Parse all parameters
    data = netCGI_GetEnvVar (data, var, sizeof (var));			// se va extrayendo una a una las variables del cuerpo POST
																															// y se almacenan, se termina cuando no queden más (data sea NULL)
		if (var[0] != 0) {
      // si el primer parámetro no es nulo y coincide con alguno de los strings a comparar debajo
				// entonces enciende el led correspondientes, esto permite encender varios leds simultáneamente
      if (strcmp (var, "led0=on") == 0) {	
        estadoLEDS |= 0x01;
      }
      else if (strcmp (var, "led1=on") == 0) {
        estadoLEDS |= 0x02;
      }
      else if (strcmp (var, "led2=on") == 0) {
        estadoLEDS |= 0x04;
      }
      else if (strcmp (var, "led3=on") == 0) {
        estadoLEDS |= 0x08;
      }
      else if (strcmp (var, "led4=on") == 0) {
        estadoLEDS |= 0x10;
      }
      else if (strcmp (var, "led5=on") == 0) {
        estadoLEDS |= 0x20;
      }
      else if (strcmp (var, "led6=on") == 0) {
        estadoLEDS |= 0x40;
      }
      else if (strcmp (var, "led7=on") == 0) {
        estadoLEDS |= 0x80;
      }
      else if (strcmp (var, "ctrl=Browser") == 0) {		// si recibe esto, desactiva el parpadeo automático
        LEDrun = false;					// se pone a false la variable que permite el bucle de leds de HTTP_SERVER.c
      }
      else if ((strncmp (var, "pw0=", 4) == 0) ||
               (strncmp (var, "pw2=", 4) == 0)) {
        // Change password, retyped password
        if (netHTTPs_LoginActive()) {
          if (passw[0] == 1) {
            strcpy (passw, var+4);
          }
          else if (strcmp (passw, var+4) == 0) {
            // Both strings are equal, change the password
            netHTTPs_SetPassword (passw);
          }
        }
      }
      else if (strncmp (var, "lcd1=", 5) == 0) {
        // LCD Module line 1 text
        strcpy (lcd_text[0], var+5);			// si recibe lcd1=, se almacena el texto en lcd_text[0] (línea superior lcd)
        infoDisp.linea = 0;
// -- ¿ELIMINAR ESTO? -- ^ infoDisp.linea = 0; 

      }
      else if (strncmp (var, "lcd2=", 5) == 0) {
        // LCD Module line 2 text
        strcpy (lcd_text[1], var+5);			// si recibe lcd2=, se almacena el texto en lcd_text[1] (línea inferior lcd)
// -- ¿O AÑADIR ESTO? -- -> infoDisp.linea = 1; 
			}
			
			// Actualización de la hora del RTC
			else if (strncmp(var, "rtc1=", 5) == 0) {
				int hh, mm, ss;
				if (sscanf(var + 5, "%02d:%02d:%02d", &hh, &mm, &ss) == 3) {
					RTC_Time_Config(dec2bcd((uint8_t)hh), dec2bcd((uint8_t)mm), dec2bcd((uint8_t)ss));
				}
			}
			// Actualización de la fecha del RTC
			else if (strncmp(var, "rtc2=", 5) == 0) {
				int dd, ms, yy;
				if (sscanf(var + 5, "%02d-%02d-%d", &dd, &ms, &yy) == 3) {
					if (yy >= 2000) {
						yy -= 2000;
					}
					RTC_Date_Config(dec2bcd((uint8_t)dd), dec2bcd((uint8_t)ms), dec2bcd((uint8_t)yy), RTC_WEEKDAY_MONDAY);
				}
			}
			
    }
  } while (data);
  
  infoDisp.reset = 0;
  
  strcpy (infoDisp.buffer,lcd_text[0]);			// texto primero recibido antes se guarda en tipo para la cola
  infoDisp.linea = 0;												// se asigna a la línea superior del lcd
  osMessageQueuePut(IdqueueLCD, &infoDisp, NULL, 100);		// se envía por la cola
  
  strcpy (infoDisp.buffer,lcd_text[1]);			// texto segundo recibido antes se guarda en tipo para la cola
  infoDisp.linea = 1;												// se asigna a la línea inferior del lcd
  osMessageQueuePut(IdqueueLCD, &infoDisp, NULL, 100);		// se envía por la cola
	
  LED_SetOut (estadoLEDS);					// se actualiza el estado de los leds según lo configurado antes
}

// Generate dynamic web data from a script line.
//Función que genera contenido dinámico en el servidor web
uint32_t netCGI_Script (const char *env, char *buf, uint32_t buflen, uint32_t *pcgi) {
  int32_t socket;
  netTCP_State state;
  NET_ADDR r_client;
  const char *lang;
  uint32_t len = 0U;
  uint8_t id;
  static uint32_t adv;
  netIF_Option opt = netIF_OptionMAC_Address;
  int16_t      typ = 0;
  

  switch (env[0]) {
    // Analyze a 'c' script line starting position 2
    case 'a' :
      // Network parameters from 'network.cgi'
      switch (env[3]) {
        case '4': typ = NET_ADDR_IP4; break;
        case '6': typ = NET_ADDR_IP6; break;

        default: return (0);
      }
      
      switch (env[2]) {
        case 'l':
          // Link-local address
          if (env[3] == '4') { return (0);                             }
          else               { opt = netIF_OptionIP6_LinkLocalAddress; }
          break;

        case 'i':
          // Write local IP address (IPv4 or IPv6)
          if (env[3] == '4') { opt = netIF_OptionIP4_Address;       }
          else               { opt = netIF_OptionIP6_StaticAddress; }
          break;

        case 'm':
          // Write local network mask
          if (env[3] == '4') { opt = netIF_OptionIP4_SubnetMask; }
          else               { return (0);                       }
          break;

        case 'g':
          // Write default gateway IP address
          if (env[3] == '4') { opt = netIF_OptionIP4_DefaultGateway; }
          else               { opt = netIF_OptionIP6_DefaultGateway; }
          break;

        case 'p':
          // Write primary DNS server IP address
          if (env[3] == '4') { opt = netIF_OptionIP4_PrimaryDNS; }
          else               { opt = netIF_OptionIP6_PrimaryDNS; }
          break;

        case 's':
          // Write secondary DNS server IP address
          if (env[3] == '4') { opt = netIF_OptionIP4_SecondaryDNS; }
          else               { opt = netIF_OptionIP6_SecondaryDNS; }
          break;
      }

      netIF_GetOption (NET_IF_CLASS_ETH, opt, ip_addr, sizeof(ip_addr));
      netIP_ntoa (typ, ip_addr, ip_string, sizeof(ip_string));
      len = (uint32_t)sprintf (buf, &env[5], ip_string);
      break;

    case 'b':						// -- CONTROL DE LOS LEDs: info en led.cgi --
      // LED control from 'led.cgi'
      if (env[2] == 'c') {
        // Select Control
        len = (uint32_t)sprintf (buf, &env[4], LEDrun ?     ""     : "selected",		// guarda la info de si el led está encendido o apagado
                                               LEDrun ? "selected" :    ""     );
        break;
      }
      // LED CheckBoxes
      id = env[2] - '0';		    // convierte lo leído en env a un número de 0 a 7
      if (id > 7) {
        id = 0;
      }
      id = (uint8_t)(1U << id);        // genera un bitmask que representa a cada led
      len = (uint32_t)sprintf (buf, &env[4], (estadoLEDS & id) ? "checked" : "");   // comprueba el estado del led
      break;                                                                          // si encendido "checked" y si apagado ""

    case 'c':
      // TCP status from 'tcp.cgi'
      while ((uint32_t)(len + 150) < buflen) {
        socket = ++MYBUF(pcgi)->idx;
        state  = netTCP_GetState (socket);

        if (state == netTCP_StateINVALID) {
          /* Invalid socket, we are done */
          return ((uint32_t)len);
        }

        // 'sprintf' format string is defined here
        len += (uint32_t)sprintf (buf+len,   "<tr align=\"center\">");
        if (state <= netTCP_StateCLOSED) {
          len += (uint32_t)sprintf (buf+len, "<td>%d</td><td>%d</td><td>-</td><td>-</td>"
                                             "<td>-</td><td>-</td></tr>\r\n",
                                             socket,
                                             netTCP_StateCLOSED);
        }
        else if (state == netTCP_StateLISTEN) {
          len += (uint32_t)sprintf (buf+len, "<td>%d</td><td>%d</td><td>%d</td><td>-</td>"
                                             "<td>-</td><td>-</td></tr>\r\n",
                                             socket,
                                             netTCP_StateLISTEN,
                                             netTCP_GetLocalPort(socket));
        }
        else {
          netTCP_GetPeer (socket, &r_client, sizeof(r_client));

          netIP_ntoa (r_client.addr_type, r_client.addr, ip_string, sizeof (ip_string));
          
          len += (uint32_t)sprintf (buf+len, "<td>%d</td><td>%d</td><td>%d</td>"
                                             "<td>%d</td><td>%s</td><td>%d</td></tr>\r\n",
                                             socket, netTCP_StateLISTEN, netTCP_GetLocalPort(socket),
                                             netTCP_GetTimer(socket), ip_string, r_client.port);
        }
      }
      /* More sockets to go, set a repeat flag */
      len |= (1u << 31);
      break;

    case 'd':
      // System password from 'system.cgi'
      switch (env[2]) {
        case '1':
          len = (uint32_t)sprintf (buf, &env[4], netHTTPs_LoginActive() ? "Enabled" : "Disabled");
          break;
        case '2':
          len = (uint32_t)sprintf (buf, &env[4], netHTTPs_GetPassword());
          break;
      }
      break;

    case 'e':
      // Browser Language from 'language.cgi'
      lang = netHTTPs_GetLanguage();
      if      (strncmp (lang, "en", 2) == 0) {
        lang = "English";
      }
      else if (strncmp (lang, "de", 2) == 0) {
        lang = "German";
      }
      else if (strncmp (lang, "fr", 2) == 0) {
        lang = "French";
      }
      else if (strncmp (lang, "sl", 2) == 0) {
        lang = "Slovene";
      }
      else {
        lang = "Unknown";
      }
      len = (uint32_t)sprintf (buf, &env[2], lang, netHTTPs_GetLanguage());
      break;

    case 'f':						// -- CONTROL DEL LCD: info en lcd.cgi --
      // LCD Module control from 'lcd.cgi'
      switch (env[2]) {     // analiza env para ver que línea de texto mostrar
        case '1':
          len = (uint32_t)sprintf (buf, &env[4], lcd_text[0]);    // se almacena en buf el texto del lcd_text[0]
          break;
        case '2':
          len = (uint32_t)sprintf (buf, &env[4], lcd_text[1]);    // se almacena en buf el texto del lcd_text[1]
          break;
      }
      break;

    case 'g':           // -- CONTROL DEL ADC: info en adc.cgi --
      // AD Input from 'ad.cgi'
      switch (env[2]) {     // analiza env para ver que tiene que leer
        case '1':     // lee el valor analógico del pin del ADC
          adv = AD_in (0);
          len = (uint32_t)sprintf (buf, &env[4], (uint32_t)adv);
          break;
        case '2':     // convierte adv a voltaje con la fórmula de abajo, depende de la ejecución del case 1
          len = (uint32_t)sprintf (buf, &env[4], (double)((float)adv*3.3f)/4096);
          break;
        case '3':     // convierte adv en porcentaje, depende de la ejecución del case 2
          adv = (adv * 100) / 4096;
          len = (uint32_t)sprintf (buf, &env[4], (uint32_t)adv);
          break;
      }
      break;
      
    case 'h':           // -- CONTROL DEL RTC: info en time.cgi --
      // Input from 'time.cgi'
      switch (env[2]) {
        case '1':       // llama a rtc.show(), que actualiza hora y fecha en el lcd, y escribe lcd_hora en buf
          RTC_Show(lcd_hora, lcd_fecha);
          len = (uint32_t)sprintf (buf, &env[4], lcd_hora);
          break;
        case '2':       // llama a rtc.show(), que actualiza hora y fecha en el lcd, y escribe lcd_fecha en buf
          RTC_Show(lcd_hora, lcd_fecha);
          len = (uint32_t)sprintf (buf, &env[4], lcd_fecha);
          break;
      }
      break;

    case 'x':       // versión reducida del control del ADC
      // AD Input from 'ad.cgx'
      adv = AD_in (0);
      len = (uint32_t)sprintf (buf, &env[1], (uint32_t)adv);
      break;

    case 'y':
      // Button state from 'button.cgx'
      len = (uint32_t)sprintf (buf, "<checkbox><id>button%c</id><on>%s</on></checkbox>",
                               env[1], (get_button () & (1 << (env[1]-'0'))) ? "true" : "false");
      break;
    
    case 'z':
      // Button state from 'time.cgx'
      switch (env[2]) {
        case '1':
          RTC_Show(lcd_hora, lcd_fecha);
          len = (uint32_t)sprintf (buf, &env[4], lcd_hora);
          break;
        case '2':
          RTC_Show(lcd_hora, lcd_fecha);
          len = (uint32_t)sprintf (buf, &env[4], lcd_fecha);
          break;
      }
      break;
  }
  return (len);
}

// Función para convertir de decimal a BCD
uint8_t dec2bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

#if      defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma  clang diagnostic pop
#endif
