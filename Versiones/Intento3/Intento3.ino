#include <Wire.h>    //  libreria para interfaz I2C
#include <RTClib.h>   //  libreria para el manejo del modulo RTC
#include <RBDdimmer.h> // libreria del dimmer AC sadasdsa
#include <Time.h>
#include <TimeAlarms.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

RTC_DS3231 rtc;     // crea objeto del tipo RTC_DS3231
char daysOfTheWeek[7][4] = {"Dom", "Lun", "Mar", "Mier", "Jue", "Vie", "Sab"};

// DHT humidity sensor
int DHTPIN = 9;
// Ambient status
float ambientTemp; 
float ambientHum;// what pin we're connected to
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
int dur = 5000;
int Deltadim = dur/(DimMAX - DimMin);
unsigned long Delta = (unsigned long)Deltadim * (unsigned long)1000; 
unsigned long tim;

//********** Hora de comienzo del amanecer
int horaPrender = 11;
int minutoPrender = 00;

int hrinidia = 13;
int mininidia = 40;

//********** Hora de comienzo del atardecer
int hrfindia = 18;
int minfindia = 00;

//********** Hora de apagado
int horaApagar = 22;
int minutoApagar = 00;


void setup () {
 Serial.begin(9600);    // inicializa comunicacion serie a 9600 bps
 pinMode(RELE, OUTPUT);   // pin 52 para las luces secundarias
 
 dimmer.begin(NORMAL_MODE, OFF);       // si falla la inicializacion del modulo name.begin(MODE, STATE)  
//pinMode(RELE2, OUTPUT);
//Definiciones RTC
if (!rtc.begin()) {       // si falla la inicializacion del modulo
  Serial.println("Modulo RTC no encontrado !");  // muestra mensaje de error
  while (1);         // bucle infinito que detiene ejecucion del programa
 }
//rtc.adjust(DateTime(__DATE__, __TIME__));   
//Serial.println("Modulo RTC Ajustado !"); 
// Use only without RTC
//setTime(10,20,0,14,7,2020);
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
encendidos();
StatusSol();
Alarm.timerRepeat(60, sendStatus);
}

void loop () {       // funcion que devuelve fecha y horario en formato DateTime y asigna a variable fecha

    Alarm.delay(1000);           // demora de 1 segundo
    
}
void StatusSol()
{
  int  hr = hour();
  int  mini = minute();
  Serial.println("Paso a verificar el estdo de luces");
  if(hr >= horaPrender && hr < horaApagar)
      { 
        if(hr == horaPrender && mini > minutoPrender)
         {
          Amanecer();
          return;
         }
        if (hr >= hrinidia && hr <= hrfindia)
          {
            if(hr == hrfindia && mini > minfindia)
            {
              Atardecer();
              return;
            }
            if(hr == hrinidia && mini < mininidia)
            {
              Amanecer();
              return;
            }
            Pleno_dia();
            return;
          }
        if(hr > horaPrender && hr < hrinidia)
        {
          Amanecer();
          return;
        }
        if(hr > hrfindia && hr <= horaApagar)
         { 
           Atardecer();
           return;
         }  
      } 
     
  else Noche(); 
  return;    
}

void sendStatus() {
  readSensors();
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
}

void readSensors() {
  sensors.requestTemperatures();
  aquariumTemp = sensors.getTempCByIndex(0);
  ambientTemp = dht.readTemperature();  // obtencion de valor de temperatura
  ambientHum = dht.readHumidity();   // obtencion de valor de humedad

  }

void Amanecer() 
{
   momento = "Amanecer";
   Serial.println("Amanecer");
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
            if (Dimm == 35){
                digitalWrite(RELE, LOW);       
                Serial.println( "Luces complementarias encendidas" ); 
            }
            Dimm++;
           } 
        }
        
}

void Pleno_dia() 
{
  momento = "Pleno dia";
  dimmer.setState(ON);  
  dimmer.setPower(DimMAX);
  digitalWrite(RELE, LOW);
  Serial.println("Pleno Dia");
}

void Atardecer()
{ 
momento = "Atardecer";
 if(!dimmer.getState()) dimmer.setState(ON);
 Serial.println("Atardecer");
 tim = millis();
 Dimm=DimMAX;
 if(!dimmer.getState()) dimmer.setState(ON);
 dimmer.setPower(Dimm); 
        while(Dimm > DimMin){
           if(millis() >= tim + Delta){
            tim +=Delta;   
            dimmer.setPower(Dimm);                                       // name.setPower(0%-100%)
            Serial.print(dimmer.getPower());
            Serial.println("%");
            if (Dimm == 35){
                digitalWrite(RELE, LOW);       
                Serial.println( "Luces complementarias apagadas" ); 
            }
            Dimm--;
           } 
        }

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