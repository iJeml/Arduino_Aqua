#include <Wire.h>    //  libreria para interfaz I2C
#include <RTClib.h>   //  libreria para el manejo del modulo RTC
#include <RBDdimmer.h> // libreria del dimmer AC 

RTC_DS3231 rtc;     // crea objeto del tipo RTC_DS3231

bool Amanecer = true;  // variable de control para inicio de evento amanecer FadeIN
bool pleno_dia = true;   // variable de control para activar todas las luces
bool Atardecer = true; // variable de control para el proceso de Apagado de Luces
int valor = 25 ;

#define PIN_SALIDA_DIMMER  3                                    //Pines del dimmer
#define ZEROCROSS  2
#define use_serial Serial 
# define RELE 52     // donde se encuentra conectado el modulo de rele que corresponde a pin digital 30
# define RELE2 23    // donde se encuentra conectado el modulo de rele que corresponde a pin digital 31 (desocupado)
dimmerLamp dimmer(PIN_SALIDA_DIMMER);
        
void setup () {
 use_serial.begin(9600);    // inicializa comunicacion serie a 9600 bps
 pinMode(RELE, OUTPUT);   // pin 30 como salida
 
 dimmer.begin(NORMAL_MODE, OFF);       // si falla la inicializacion del modulo name.begin(MODE, STATE)  
//pinMode(RELE2, OUTPUT);
if (! rtc.begin()) {       // si falla la inicializacion del modulo
  use_serial.println("Modulo RTC no encontrado !");  // muestra mensaje de error
  while (1);         // bucle infinito que detiene ejecucion del programa
 }
//rtc.adjust(DateTime(__DATE__, __TIME__));   
//use_serial.println("Modulo RTC Ajustado !"); 
digitalWrite(RELE, HIGH);                          
}

void loop () {
 DateTime fecha = rtc.now();        // funcion que devuelve fecha y horario en formato DateTime y asigna a variable fecha
 if ( fecha.hour() == 19 && fecha.minute() == 10 )
    { 
        for(valor;valor < 75;valor++)
          {   
              dimmer.setState(ON);
              dimmer.setPower(valor);                                       // name.setPower(0%-100%)
              use_serial.print(dimmer.getPower());
              use_serial.println("%");
              delay(1000);
          if (valor == 35){     // si evento_inicio es verdadero
                digitalWrite(RELE, LOW);       
                use_serial.println( "Rele encendido" ); 
                Amanecer = false; 
             } 
          }             
    }
  if ( fecha.hour() == 19 && fecha.minute() == 14){   //si hora = 15 y minutos = 30
          
          for(valor;valor > 25;valor --)
          {     
              dimmer.setPower(valor);                                       // name.setPower(0%-100%)
              use_serial.print(dimmer.getPower());
              use_serial.println("%");
              delay(1000);
          if (valor == 35){     // si evento_inicio es verdadero
                digitalWrite(RELE, HIGH);       
                use_serial.println( "Rele apagado" );  
                Atardecer = false;
             } 
   
          }
       dimmer.setState(OFF);
  }

 use_serial.print(fecha.day());       // funcion que obtiene el dia de la fecha completa
 use_serial.print("/");         // caracter barra como separador
 use_serial.print(fecha.month());       // funcion que obtiene el mes de la fecha completa
 use_serial.print("/");         // caracter barra como separador
 use_serial.print(fecha.year());        // funcion que obtiene el año de la fecha completa
 use_serial.print(" ");         // caracter espacio en blanco como separador
 use_serial.print(fecha.hour());        // funcion que obtiene la hora de la fecha completa
 use_serial.print(":");         // caracter dos puntos como separador
 use_serial.print(fecha.minute());        // funcion que obtiene los minutos de la fecha completa
 use_serial.print(":");         // caracter dos puntos como separador
 use_serial.println(fecha.second());      // funcion que obtiene los segundos de la fecha completa
 
 delay(1000);           // demora de 1 segundo

}
