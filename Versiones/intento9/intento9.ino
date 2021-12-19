#include <Wire.h>    //  libreria para interfaz I2C
#include "RTClib.h"   //  libreria para el manejo del modulo RTC
#include <Time.h>
#include <TimeAlarms.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <LCD.h>			// libreria para funciones de LCD
#include <LiquidCrystal_I2C.h>		// libreria para LCD por I2C
#include <DHT.h>    // importa la Librerias DHT
#include <DHT_U.h>


RTC_DS3231 rtc; // crea objeto del tipo RTC_DS3231

char daysOfTheWeek[7][4] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};

//valores para el joystick
#define joyX A2
#define joyY A3
#define pulsa 10//falta definir el pin10 que tiene conectado el SW del joystick con la resistencia de 10k instalada
int bomba;

//indicador de pantalla
int pant = 1;


//creacion objeto pantalla LCD
LiquidCrystal_I2C lcd (0x27, 2, 1, 0, 4, 5, 6, 7); // DIR, E, RW, RS, D4, D5, D6, D7

// Water proof and ambient sensor DS18B20
#define ONE_WIRE_BUS 8
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
// Aquarium status
float aquariumTemp;
float ambientTemp;

uint8_t sensor1[8] = {0x28, 0x20, 0x81, 0x83, 0x29, 0x20, 0x01, 0x7A};
uint8_t sensor2[8] = {0x28, 0xAA, 0x21, 0x57, 0x13, 0x13, 0x02, 0x4D};



// pines digitales para control de reles
#define Lamp_aq 24    // lampara Ciclo
#define filter 26    // Filtro Canister
#define co2 28     //  CO2 selenoide


String momento; // variable de control para el proceso de Apagado de Luces

String CO2= "OFF"; // flag del CO2
         


//***********Variables para la definicion de Horarios del Acuario


//Hora de comienzo del dia
int horaPrender = 11;
int minutoPrender = 30;
int horaco2  = horaPrender-2;
int minutoco2 = minutoPrender;

//Hora de fin del dia
int horaApagar = 20;
int minutoApagar = 30;

//******************Variables del Indoor Growing********************

int cult_sensor = 52; 
DHT dht(cult_sensor, DHT22); 
float Temp_cult;
float Hum_cult;
String estado;
String Vent = "ON";
String air_flag = "OFF";
// pines digitales para control de reles

#define aire 36     // Corriente de los extractores 
#define cult_hz 32   //Humificador /futuro ozono
#define cult_luz 44    // Lampara

//Hora del Sol
int horasol =  8;
int minutosol = 00;

//Hora de la Luna
int horaluna = 22;
int minutoLuna = 00;

int tiempocomida = 2;

// Valores ambientales ideales
float hum_min = 50.0;
float hum_max =65.0;
float Temp_min = 15.5;
float Temp_max = 27.0;



void setup () {
 Serial.begin(9600);    // inicializa comunicacion serie a 9600 bps
 pinMode(co2, OUTPUT);   // pin 22 para las luces secundarias
 pinMode(Lamp_aq, OUTPUT);  //pin 24 lampara ciclo
 pinMode(filter, OUTPUT); 
 pinMode(pulsa, INPUT);		// pulsador como entrada 

 // variables de los reles del indoor
 pinMode(aire, OUTPUT);  // pin 50 como salida
 pinMode(cult_luz, OUTPUT);  // pin 44 como salida
 pinMode(cult_hz, OUTPUT);  // pin 42 como salida
 

//Definiciones RTC
 if (!rtc.begin()) 
  {               // si falla la inicializacion del modulo  Serial.println("Modulo RTC no encontrado !");  // muestra mensaje de error
  while (1);         // bucle infinito que detiene ejecucion del programa
  }

//rtc.adjust(DateTime(__DATE__, __TIME__));   
// Use only without RTC
// setTime(23,31,0,8,8,2021);
// Sync time lib with RTC
setSyncProvider(getRTCTime);
setSyncInterval(60 * 60 * 4);
Serial.println("Modulo RTC Ajustado !"); 

//inicializacion de reles 
digitalWrite(co2, HIGH);
digitalWrite(Lamp_aq, HIGH);
digitalWrite(filter, LOW);
digitalWrite(aire, HIGH);
digitalWrite(cult_luz, HIGH);
digitalWrite(cult_hz, HIGH);
     
//inicializando pantalla
lcd.setBacklightPin(3,POSITIVE);	// puerto P3 de PCF8574 como positivo
lcd.setBacklight(HIGH);		// habilita iluminacion posterior de LCD
lcd.begin(20,4);			// 20 columnas por 4 lineas para LCD 2004A
lcd.clear();			// limpia pantalla

// iniciando modulo de temperatura y humedad del cultivo 
dht.begin(); 

//inicializando modulos de Temperatura de agua y de ambiente
//sensor_t sensor;
sensors.begin();

//inicializacion de funciones para primeras variables

sendstatus();
Statusol();
Statusol_cult();
encendidos();
readSensors();
amb_indoor();
est_indur();
lcdstatus();

}

void loop () 
{       
  int xValue = analogRead(joyX);
  int yValue = analogRead(joyY);
  bomba = digitalRead(pulsa);
  if (yValue>=0 && yValue<=380)
   {
    lcd.clear();
    //lcd.setCursor(0,0);
    scr_aqua();
   }
  if (xValue>=0 && xValue<=380)
  {
    lcd.clear();
    //lcd.setCursor(0,0);
    scr_aqua();
  }
  if (xValue>=630 && xValue<=1023)
   {
     lcd.clear();
     //lcd.setCursor(0,0);
     scr_indur();
   }

  if (yValue>=630 && yValue<=1023)
   {
     lcd.clear(); 
     //lcd.setCursor(0,0);
     scr_indur();
   }
  if (pant == 1) scr_aqua();
  if (pant == 2) scr_indur();
  if (pant==1 && bomba==0) filtro();
  Alarm.delay(500);           // demora de 0.5 segundos
} 
//estado del lcd luego de un reinicio fuera de horario
void lcdstatus()
{
  int  hr = hour();
  int  mini = minute();
  if ((hr>= 23 && mini>= 30) || (hr<= 8 && mini<= 30)) lcdoff();
}

void Statusol()
{
  int  hr = hour();
  int  mini = minute();
  //estas variables internas convierten todas las horas en segundos para evaluar la duracion
  // de los periodos de amanecer y atardecer en base al tiempo disponible antes de la siguiente alarma
  unsigned long int diff;
  unsigned long sec_now = (unsigned long) hr*3600 + (unsigned long) mini*60;
  unsigned long sec_on = (unsigned long)horaPrender*3600 + (unsigned long)minutoPrender*60;
  unsigned long sec_co2 = (unsigned long)horaco2*3600 + (unsigned long)minutoco2*60;
  unsigned long sec_off = (unsigned long)horaApagar*3600 + (unsigned long)minutoApagar*60;
  Serial.println(sec_off);
  Serial.println(sec_now);
  Serial.println(sec_on);
  Serial.println("Paso a verificar el estdo de luces");
  if(sec_now > sec_co2 && sec_now < sec_off)
    { 
      Dia_CO2();
      Serial.println("paso a encender el co2");
      
    }
  if(sec_now > sec_on && sec_now < sec_off)
    { 
      Dia();
      return;
    }
  else Noche(); 
  return;    
}
void Statusol_cult()
{
  int  hr = hour();
  int  mini = minute();
  //estas variables internas convierten todas las horas en segundos para evaluar la duracion
  // de los periodos de amanecer y atardecer en base al tiempo disponible antes de la siguiente alarma
  unsigned long int diff;
  unsigned long sec_now = (unsigned long) hr*3600 + (unsigned long) mini*60;
  unsigned long cult_on = (unsigned long)horasol*3600 + (unsigned long)minutosol*60;
  unsigned long cult_off = (unsigned long)horaluna*3600 + (unsigned long)minutoLuna*60;
  Serial.println("Paso a verificar el estdo de luces del cultivo");
  Serial.print(sec_now);
  Serial.println(" seg ahora");
  Serial.print(cult_on);
  Serial.println("seg ON");
  Serial.print(cult_off);
  Serial.println("seg OFF");
  //if((sec_now > cult_on && sec_now < 86340)||(sec_now < cult_off))
  if(sec_now > cult_on && sec_now < cult_off)
    { 
      Dia_cult();
      Serial.println("Entro en el dia");
      return;
    }
  else Noche_cult(); 
       Serial.println("Entro en la noche");
  return;    
}
void sendstatus() 
{
  DateTime now = rtc.now();
  Serial.print("LOG");
  Serial.print("||");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.print("|Ambiente: ");
  Serial.print(ambientTemp,1);
  Serial.print("C|Acuario: ");
  Serial.print(aquariumTemp,1);
  Serial.println(" C");
  Serial.print("Temp de cultivo: ");
  Serial.print(Temp_cult);
  Serial.print(" C||Humedad de Cultivo: ");
  Serial.println(Hum_cult);
  Serial.print("||Estado luz: ");
  Serial.print(estado);
  Serial.print("||vent: ");
  Serial.println(Vent);  
  //return;
}

void scr_indur() 
{
  pant=2;
  DateTime now = rtc.now();
  readSensors();
  lcd.setCursor(0, 0);
  lcd.print(" Estado del Cultivo");
  lcd.setCursor(0, 1);
  if (now.day() < 10)  lcd.print('0');
  lcd.print(now.day(), DEC); 
  lcd.print('/');
  if (now.month() < 10)  lcd.print('0');
  lcd.print(now.month(), DEC);
  lcd.print('/');
  lcd.print(now.year(), DEC);
  lcd.print("  ");
  if (now.hour() < 10)  lcd.print('0');
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  if (now.minute() < 10)  lcd.print('0');
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  if (now.second() < 10)  lcd.print('0');
  lcd.print(now.second(), DEC);
  lcd.setCursor(0, 2);
  lcd.print("Amb:");
  lcd.print(ambientTemp,1);
  lcd.print("C/Cul:");
  lcd.print(Temp_cult,1);
  lcd.print("C");
  lcd.setCursor(0, 3);
  lcd.print("Hum:");
  lcd.print(Hum_cult,1);
  lcd.print("%");
  lcd.print("Vent:");
  lcd.print(Vent); //se debe imprimir el estado de los ventiladores
  lcd.noCursor();			// oculta cursor 
  //return; 
}

void scr_aqua()
{
  pant=1;
  DateTime now = rtc.now();
  readSensors();
  lcd.setCursor(0, 0);
  lcd.print(" Estado del Acuario");
  lcd.setCursor(0, 1);
  if (now.day() < 10)  lcd.print('0'); 
  lcd.print(now.day(), DEC); 
  lcd.print('/');
  if (now.month() < 10)  lcd.print('0'); 
  lcd.print(now.month(), DEC);
  lcd.print('/');
  lcd.print(now.year(), DEC);
  lcd.print("  ");
  if (now.hour() < 10)  lcd.print('0'); 
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  if (now.minute() < 10)  lcd.print('0'); 
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  if (now.second() < 10)  lcd.print('0'); 
  lcd.print(now.second(), DEC);
  lcd.setCursor(0, 2);
  lcd.print("Amb:");
  lcd.print(ambientTemp,1);
  lcd.print("C ");
  lcd.print(momento);
  lcd.setCursor(0, 3);
  lcd.print("Agua:");
  lcd.print(aquariumTemp,1);
  lcd.print("C");
  lcd.print(" CO2:");
  lcd.print(CO2);
  lcd.noCursor();			// oculta cursor 
  //return;
}
void filtro()
{
  lcd.clear();
  lcd.setCursor(0, 2);
  lcd.print("  Hora de comer!!");
  digitalWrite(filter, HIGH);
  Alarm.delay(tiempocomida*60000);
  lcd.clear();
  lcd.println("   Hora de comer");
  lcd.print("     finaliazda!!");
  digitalWrite(filter, LOW);
}
void readSensors() 
{
  // obtencion de valor de temperatura
  sensors.requestTemperatures();
  ambientTemp = sensors.getTempC(sensor1);
  aquariumTemp = sensors.getTempC(sensor2);
}


void Dia() 
{
  momento = "Dia  ";
  digitalWrite(Lamp_aq, LOW);
  Serial.println("Dia");
}
void Dia_CO2() 
{
  CO2 = "ON ";
  digitalWrite(co2, LOW);
  Serial.println("CO2 encendido");
}
       
void Noche()
{
   momento = "Noche";
   CO2 = "OFF";
   digitalWrite(co2, HIGH);
   digitalWrite(Lamp_aq, HIGH);
   Serial.println(" Noche");
  }
void lcdoff()
{
  lcd.setBacklight(LOW);
  }
void lcdon()
{
  lcd.setBacklight(HIGH);
  }     

void encendidos() {
  Alarm.alarmRepeat(horaPrender, minutoPrender, 0, Dia);
  Alarm.alarmRepeat(horaco2, minutoco2, 0, Dia_CO2);
  Alarm.alarmRepeat(horaApagar, minutoApagar, 0, Noche);
  Alarm.alarmRepeat(23, 00, 0, lcdoff);
  Alarm.alarmRepeat(8, 00, 0, lcdon);
  Alarm.timerRepeat(10, readSensors);
  Alarm.timerRepeat(10, sendstatus);
  Serial.println("Paso a setear las alarmas");
}
//**************Funciones del Indoor
void amb_indoor(){
  Temp_cult = dht.readTemperature();  // obtencion de valor de temperatura cultivo
  Hum_cult = dht.readHumidity();   // obtencion de valor de humedad cultivo
}
void Dia_cult() 
{
  estado = "Sol";
  digitalWrite(cult_luz, LOW);
  Serial.println("es de dia");

  
}
       
void Noche_cult()
{
   estado = "Luna";
   digitalWrite(cult_luz, HIGH);
   Serial.println("es de noche");
}

/*void airon() 
{
  Vent = "ON ";
  digitalWrite(cult_aire, LOW);
}
       
void airoff()
{
   Vent = "OFF";
   digitalWrite(cult_aire, HIGH);
  }
  
void tempumcont()
{
  Serial.println("entra al control de ambiente");
 if (air_flag == "OFF" && (Temp_cult > Temp_max || Hum_cult > hum_max) )
    { 
      Serial.println("bajar temp o hum");
      airon();
      return;
    }
  if (Temp_cult <= Temp_min && Hum_cult <= hum_min && air_flag == "OFF" )
    { 
      Serial.println("temp o hum ok");
      airoff();
      return;
    }         
}  

void air_renew_fin()
{
   Vent = "OFF";
   digitalWrite(cult_aire, HIGH);
   air_flag="OFF";
   Serial.println("Fin de Renovacion de Aire");
}

void air_renew ()
{
    air_flag="ON";
    Serial.println("Aire programado");
    airon();
    Alarm.timerOnce(180, air_renew_fin);
}*/

void est_indur() {
  Alarm.alarmRepeat(horasol, minutosol, 0, Dia_cult);
  Alarm.alarmRepeat(horaluna, minutoLuna, 0, Noche_cult);
  Alarm.timerRepeat(10, amb_indoor);
  //Alarm.timerRepeat(60, air_renew);
  //Alarm.timerRepeat(60, tempumcont);  
  Serial.println("Paso a setear las alarmas del indoor");
}
// Funcion de Setting del RTC
time_t getRTCTime() {
  DateTime now = rtc.now();
  Serial.println("Sync RTC time");
  return now.unixtime();
}
