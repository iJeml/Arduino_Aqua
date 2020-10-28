#include <Wire.h>    //  libreria para interfaz I2C
#include "RTClib.h"   //  libreria para el manejo del modulo RTC
#include <RBDdimmer.h> // libreria del dimmer AC 
#include <Time.h>
#include <TimeAlarms.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

RTC_DS3231 rtc;     // crea objeto del tipo RTC_DS3231
char daysOfTheWeek[7][4] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};

// DHT humidity sensor
int DHTPIN = 10;
// Ambient status
float ambientTemp; 
float ambientHum;    // what pin we're connected to
//#define DHTTYPE    DHT11   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHT11); //// Initialize DHT sensor for normal 16mhz Arduino

// Water proof sensor DS18B20
#define ONE_WIRE_BUS 8
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
// Aquarium status
float aquariumTemp;

//Dimmer para las luces blancas
#define PIN_SALIDA_DIMMER  3        //Pines del dimmer
#define ZEROCROSS  2
#define RELE 24     // donde se encuentra conectado el modulo de rele que corresponde a pin digital 30
//#define RELE2 23    // donde se encuentra conectado el modulo de rele que corresponde a pin digital 31 (desocupado)
dimmerLamp dimmer(PIN_SALIDA_DIMMER);


String momento; // variable de control para el proceso de Apagado de Luces
         


// Variables para la definicion de Horarios

int Dimm;
int DimMAX = 65;
int DimMin = 30;
int dur = 3600;
unsigned long  dur_temp = 0;
int Deltadim; //calcula el intervalo de tiempo para el dimmmer
unsigned long Delta; // convierte Deltadim en milisegundos
unsigned long tim;  // es el valor de los milis dentro de las funciones de dimmer

//********** Hora de comienzo del amanecer
int horaPrender = 11;
int minutoPrender = 00;

int hrinidia = 14;
int mininidia = 10;

//********** Hora de comienzo del atardecer
int hrfindia = 20;
int minfindia = 00;

//********** Hora de apagado
int horaApagar = 22;
int minutoApagar = 30;


void setup () {
 Serial.begin(9600);    // inicializa comunicacion serie a 9600 bps
 pinMode(RELE, OUTPUT);   // pin 52 para las luces secundarias
 
 dimmer.begin(NORMAL_MODE, OFF);       // si falla la inicializacion del modulo name.begin(MODE, STATE)  
//pinMode(RELE2, OUTPUT);
//Definiciones RTC
if (!rtc.begin()) 
 {       // si falla la inicializacion del modulo  Serial.println("Modulo RTC no encontrado !");  // muestra mensaje de error
  while (1);         // bucle infinito que detiene ejecucion del programa
 }
//rtc.adjust(DateTime(__DATE__, __TIME__));   
//Serial.println("Modulo RTC Ajustado !"); 
// Use only without RTC
// setTime(13,20,0,27,9,2020);
// Sync time lib with RTC
setSyncProvider(getRTCTime);
setSyncInterval(60 * 60 * 4);
// should run only once at initial setup
sendStatus();
digitalWrite(RELE, HIGH);    

//inicializando modulos de Temperatura de agua y de ambiente/Humedad
dht.begin();
sensor_t sensor;
sensors.begin();

//inicializacion de funciones para primeras variables
encendidos();
readSensors();
StatusSol();
Alarm.timerRepeat(60, readSensors);
}

void loop () {       

    sendStatus();// funcion que devuelve fecha y horario en formato DateTime y asigna a variable fecha
    Alarm.delay(5000);           // demora de 1 segundo

}
void StatusSol()
{
  int  hr = hour();
  int  mini = minute();

  //estas variables internas convierten todas las horas en segundos para evaluar la duracion
  // de los periodos de amanecer y atardecer en base al tiempo disponible antes de la siguiente alarma
  unsigned long int diff;
  unsigned long sec_now = (unsigned long) hr*3600 + (unsigned long) mini*60;
  unsigned long sec_hp = (unsigned long)horaPrender*3600 + (unsigned long)minutoPrender*60;
  unsigned long sec_ii =  (unsigned long)hrinidia*3600 + (unsigned long) mininidia*60;
  unsigned long sec_fi = (unsigned long)hrfindia*3600 + (unsigned long)minfindia*60;
  unsigned long sec_ap = (unsigned long)horaApagar*3600 + (unsigned long)minutoApagar*60;
  Serial.println(sec_ap);
  Serial.println(sec_now);
  Serial.println(sec_hp);
  Serial.println("Paso a verificar el estdo de luces");
  if(sec_now > sec_hp && sec_now < sec_ap)
      { 
        if(sec_now > sec_hp && sec_now < sec_ii)
         {
          diff = sec_ii - sec_now;   
           if(diff < dur) dur_temp = diff;
          Amanecer();
          return;
         }
        if (sec_now > sec_ii && sec_now < sec_fi)
          {
            Pleno_dia();
            return;
          }
        if(sec_now > sec_fi && sec_now < sec_ap)
         { 
           diff = sec_ap - sec_now;   
           digitalWrite(RELE, LOW);
           if(diff < dur) dur_temp = diff;
           Serial.print("dif: ");
           Serial.println(diff); 
           Serial.print("dur_temp: ");
           Serial.println(dur_temp);   
           Atardecer();
           return;
         }  
      } 
     
  else Noche(); 
  return;    
}

void sendStatus() {
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
  Serial.print("||");
  Serial.print(ambientTemp);
  Serial.print("||");
  Serial.print(ambientHum);
  Serial.print("||");
  Serial.print(aquariumTemp);
  Serial.print("||");
  Serial.println(momento);
  return;
}

void readSensors() {
  sensors.requestTemperatures();
  aquariumTemp = sensors.getTempCByIndex(0);
  ambientTemp = dht.readTemperature();  // obtencion de valor de temperatura
  Alarm.delay(2000);
  ambientHum = dht.readHumidity();   // obtencion de valor de humedad

  }

void Amanecer() 
{
   momento = "Amanecer";
   Serial.println("Amanecer");
   if (dur_temp != 0) 
      {
        Deltadim = dur_temp/(DimMAX - DimMin); //calcula el intervalo del tiempo para el dimmer
        Delta = (unsigned long)Deltadim * (unsigned long)1000; // convierte Deltadim en milisegundos  
        Serial.print(" reduccion del amanecer por encendido. duracion:");
        Serial.print(dur_temp);
        Serial.print(" milisegundos");
      }
    else
      {
        Deltadim = dur/(DimMAX - DimMin); //calcula el intervalo del tiempo para el dimmer
        Delta = (unsigned long)Deltadim * (unsigned long)1000; // convierte Deltadim en milisegundos
      };      
   tim = millis();
   Dimm=DimMin;
   if(!dimmer.getState()) dimmer.setState(ON);
   Dimm=DimMin;
   dimmer.setPower(Dimm); 
        while(Dimm < DimMAX){
           if(millis() >= tim + Delta){
            tim +=Delta;   
            dimmer.setPower(Dimm);                                       // name.setPower(0%-100%)
            Serial.print(dimmer.getPower());
            Serial.println("%");
            if (Dimm == 45){
                digitalWrite(RELE, LOW);       
                Serial.println( "Luces complementarias encendidas" ); 
            }
            Dimm++;
           } 
        }
  dur_temp = 0;
}

void Pleno_dia() 
{
  momento = "Pleno dia";
  dimmer.setState(ON);  
  dimmer.setPower(DimMAX);
  digitalWrite(RELE, LOW);
  Serial.println("Pleno Dia");
  return;
}

void Atardecer()
{ 
 momento = "Atardecer";
 Serial.println(dur_temp);
  if (dur_temp != 0) 
      {
        Deltadim = dur_temp/(DimMAX - DimMin); //calcula el intervalo del tiempo para el dimmer
        Delta = (unsigned long)Deltadim * (unsigned long)1000; // convierte Deltadim en milisegundos  
        Serial.println(" reduccion del atardecer por encendido. duracion:");
        Serial.print(dur_temp);
        Serial.print(" segundos");
      }
    else
      {
        Deltadim = dur/(DimMAX - DimMin); //calcula el intervalo del tiempo para el dimmer
        Delta = (unsigned long)Deltadim * (unsigned long)1000; // convierte Deltadim en milisegundos
        Serial.print("dur temp es cero");
      };  
 if(!dimmer.getState()) dimmer.setState(ON);
 Serial.println("Atardecer");
 tim = millis();
 Dimm=DimMAX;
 if(!dimmer.getState()) dimmer.setState(ON);
 dimmer.setPower(Dimm); 
while(Dimm > DimMin)
  {
  if(millis() >= tim + Delta)
    {
      tim +=Delta;   
      dimmer.setPower(Dimm);          // name.setPower(0%-100%)
      Serial.print(dimmer.getPower());
      Serial.println("%");
      if (Dimm == 45)
        {
         digitalWrite(RELE, HIGH);       
         Serial.println( "Luces complementarias apagadas" ); 
         }
      Dimm--;
     } 
   }
  dur_temp = 0;

}        
void Noche()
{
   momento = "Noche";
   dimmer.setState(OFF);
   digitalWrite(RELE, HIGH);
   Serial.println("Noche");
  }

void encendidos() {
  Alarm.alarmRepeat(horaPrender, minutoPrender, 0, Amanecer);
  Alarm.alarmRepeat(hrinidia, mininidia, 0, Pleno_dia);
  Alarm.alarmRepeat(hrfindia, minfindia, 0, Atardecer);
  Alarm.alarmRepeat(horaApagar, minutoApagar, 0, Noche);
  Serial.println("Paso a setear las alarmas");
}

time_t getRTCTime() {
  DateTime now = rtc.now();
  Serial.println("Sync RTC time");
  return now.unixtime();
}
