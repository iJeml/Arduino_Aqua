#include <Wire.h>    //  libreria para interfaz I2C
#include "RTClib.h"   //  libreria para el manejo del modulo RTC
//#include <RBDdimmer.h> // libreria del dimmer AC 
#include <Time.h>
#include <TimeAlarms.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <LCD.h>			// libreria para funciones de LCD
#include <LiquidCrystal_I2C.h>		// libreria para LCD por I2C
#include <DHT.h>    // importa la Librerias DHT
#include <DHT_U.h>

RTC_DS3231 rtc; // crea objeto del tipo RTC_DS3231


int cult_sensor = 7; 
DHT dht(cult_sensor, DHT11); 

//valores para el joystick
#define joyX A2
#define joyY A3


//declaraiones del sensor DHT para el indoor
int Temp_cult;
int Hum_cult ;
int pant = 1;



char daysOfTheWeek[7][4] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};

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


//Dimmer para las luces blancas
//#define PIN_SALIDA_DIMMER  3        //Pines del dimmer
//#define ZEROCROSS  2

#define Lamp_aq1 22     // donde se encuentra conectado el modulo de rele que corresponde a pin digital 22
#define Lamp_aq2 24  
#define Tom_aq3 26    // donde se encuentra conectado el modulo de rele que corresponde a pin digital 26 bomba

String momento; // variable de control para el proceso de Apagado de Luces
         


// Variables para la definicion de Horarios

//int Dimm;
//int DimMAX = 75;
//int DimMin = 30;
//int dur = 3600;
//unsigned long  dur_temp = 0;
//int Deltadim; //calcula el intervalo de tiempo para el dimmmer
//unsigned long Delta; // convierte Deltadim en milisegundos
//unsigned long tim;  // es el valor de los milis dentro de las funciones de dimmer

//********** Hora de comienzo del amanecer
int horaPrender = 12;
int minutoPrender = 00;

//int hrinidia = 13;
//int mininidia = 00;

//********** Hora de comienzo del atardecer
//int hrfindia = 18;
//int minfindia = 30;

//********** Hora de apagado
int horaApagar = 19;
int minutoApagar = 00;


void setup () {
 Serial.begin(9600);    // inicializa comunicacion serie a 9600 bps
 pinMode(Lamp_aq1, OUTPUT);   // pin 52 para las luces secundarias
//dimmer.begin(NORMAL_MODE, OFF);       // si falla la inicializacion del modulo name.begin(MODE, STATE)  
pinMode(Lamp_aq2, OUTPUT);
pinMode(Tom_aq3, OUTPUT); 
//Definiciones RTC
 if (!rtc.begin()) 
  {       // si falla la inicializacion del modulo  Serial.println("Modulo RTC no encontrado !");  // muestra mensaje de error
  while (1);         // bucle infinito que detiene ejecucion del programa
  }
rtc.adjust(DateTime(__DATE__, __TIME__));   
Serial.println("Modulo RTC Ajustado !"); 
// Use only without RTC
// setTime(13,20,0,27,9,2020);
// Sync time lib with RTC
setSyncProvider(getRTCTime);
setSyncInterval(60 * 60 * 4);
digitalWrite(Lamp_aq1, HIGH);
digitalWrite(Lamp_aq2, HIGH);
digitalWrite(Tom_aq3, LOW);        
//inicializando pantalla
lcd.setBacklightPin(3,POSITIVE);	// puerto P3 de PCF8574 como positivo
lcd.setBacklight(HIGH);		// habilita iluminacion posterior de LCD
lcd.begin(20,4);			// 20 columnas por 4 lineas para LCD 2004A
lcd.clear();			// limpia pantalla

// iniciando modulo de temperatura y humedad del cultivo 
dht.begin(); 

//inicializando modulos de Temperatura de agua y de ambiente/Humedad

//sensor_t sensor;
sensors.begin();

//inicializacion de funciones para primeras variables

sendstatus();
Statusol();
encendidos();
readSensors();
amb_indoor();
//scr_aqua();
//scr_indur();

}

void loop () 
{       
  int xValue = analogRead(joyX);
  int yValue = analogRead(joyY);
  if (yValue>=0 && yValue<=450)
   {
    lcd.clear();
    //lcd.setCursor(0,0);
    scr_aqua();
   }
  if (xValue>=0 && xValue<=450)
  {
    lcd.clear();
    //lcd.setCursor(0,0);
    scr_aqua();
  }
  if (xValue<=1020 && xValue>=540)
   {
     lcd.clear();
     //lcd.setCursor(0,0);
     scr_indur();
   }

  if (yValue<=1020 && yValue>=540)
   {
     lcd.clear(); 
     //lcd.setCursor(0,0);
     scr_indur();
   }
  if (pant == 1) scr_aqua();
  if (pant == 2) scr_indur();
  Alarm.delay(1000);           // demora de 1 segundo
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
  unsigned long sec_off = (unsigned long)horaApagar*3600 + (unsigned long)minutoApagar*60;
  Serial.println(sec_off);
  Serial.println(sec_now);
  Serial.println(sec_on);
  Serial.println("Paso a verificar el estdo de luces");
  if(sec_now > sec_on && sec_now < sec_off)
    { 
      Dia();
      return;
    }
  else Noche(); 
  return;    
}

void sendstatus() 
{
  DateTime now = rtc.now();
  //readSensors();
  //amb_indoor();
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
  Serial.print(ambientTemp);
  Serial.print("C|Acuario: ");
  Serial.print(aquariumTemp);
  Serial.println(" C");
  Serial.print("Temp de cultivo: ");
  Serial.print(Temp_cult);
  Serial.print(" C||Humedad de Cultivo: ");
  Serial.print(Hum_cult);
  return;
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
  lcd.print("Amb.:");
  lcd.print(ambientTemp);
  lcd.print("C/Cult:");
  lcd.print(Temp_cult);
  lcd.print("C");
  lcd.setCursor(0, 3);
  lcd.print("Hum: ");
  lcd.print(Hum_cult);
  lcd.print(" %");
  lcd.print("Vent:");
//lcd.print("") se debe imprimir el estado de los ventiladores
  lcd.noCursor();			// oculta cursor 
//  return; 
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
  lcd.print(ambientTemp);
  lcd.print("C ");
  lcd.print(momento);
  lcd.setCursor(0, 3);
  lcd.print("Agua:");
  lcd.print(aquariumTemp);
  lcd.print("C");
  lcd.noCursor();			// oculta cursor 
  //return;
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
  momento = "Dia";
  //dimmer.setState(ON);  
  //dimmer.setPower(DimMAX);
  digitalWrite(Lamp_aq1, LOW);
  digitalWrite(Lamp_aq2, LOW);
  Serial.println("Dia");
  return;
}
       
void Noche()
{
   momento = "Noche";
   lcd.setCursor(0, 2);
   lcd.print("                    "); 
   //dimmer.setState(OFF);
   digitalWrite(Lamp_aq1, HIGH);
   digitalWrite(Lamp_aq2, HIGH);
   Serial.println("Noche");
  }

void encendidos() {
  Alarm.alarmRepeat(horaPrender, minutoPrender, 0, Dia);
  Alarm.alarmRepeat(horaApagar, minutoApagar, 0, Noche);
  Alarm.timerRepeat(10, readSensors);
  Alarm.timerRepeat(10, amb_indoor);
  Alarm.timerRepeat(10, sendstatus);
  Serial.println("Paso a setear las alarmas");
}

void amb_indoor(){
  Temp_cult = dht.readTemperature();  // obtencion de valor de temperatura cultivo
  Hum_cult = dht.readHumidity();   // obtencion de valor de humedad cultivo
}

time_t getRTCTime() {
  DateTime now = rtc.now();
  Serial.println("Sync RTC time");
  return now.unixtime();
}
