#include <DHT.h>		// importa la Librerias DHT
#include <DHT_U.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Wire.h>   

int SENSOR = 6;			// pin DATA de DHT22 a pin digital 2
float TEMPERATURA;
float HUMEDAD;
String momento;
String Vent = "OFF";
String air_flag= "OFF";

DHT dht(SENSOR, DHT22);		// sensor de humedad/temp para el inoor
// 7 d0 de sensor de  humedad1
//8 d0 de sensor de humedad2
//a0 a0 de sensor de humedad 1
//a1 a0 de sensor de humedad 2
int cult_Luz = 3;     // Luces del indoor 
int cult_air = 4;    // Ventiladores del indoor
int Ozono = 5;    // Bomba de circulacion
unsigned long ventact = 10000;
unsigned long TiempoAhora = 0;

void setup(){
  Serial.begin(9600);		// inicializacion de monitor serial
  pinMode(cult_Luz, OUTPUT);  // pin 3 como salida
  pinMode(cult_air, OUTPUT);  // pin 4 como salida
  pinMode(Ozono, OUTPUT);  // pin 5 como salida
  digitalWrite(Ozono, HIGH);
  dht.begin();			// inicializacion de sensor

  setTime(16,30,0,7,10,21); // set time to Saturday 8:29:00am Jan 1 2011
  // create the alarms 
  Alarm.timerRepeat(45, air_renew);            // timer for every 15 seconds    
 
}



void loop(){
  readSensors();
  Serial.print(" Temperatura: ");	// escritura en monitor serial de los valores
  Serial.print(TEMPERATURA,1);
  Serial.print(" Humedad: ");
  Serial.println(HUMEDAD,1);
  Serial.print("Ventiladores ");
  Serial.println(Vent);
  Serial.println(air_flag);
  humair(TEMPERATURA,HUMEDAD);
  Alarm.delay(2000);
}


void Dia_cult() 
{
  estado = "Sol";
  digitalWrite(cult_Luz, LOW);
}
       
void Noche_cult()
{
   estado = "Luna";
   digitalWrite(cult_Luz, HIGH);
  }

void airon() 
{
  Vent = "ON";
  digitalWrite(cult_air, LOW);
}
       
void airoff()
{
   Vent = "OFF";
   digitalWrite(cult_air, HIGH);
  }
  
void air_renew_fin()
{
   Vent = "OFF";
   digitalWrite(cult_air, HIGH);
   air_flag="OFF";
  }

void readSensors() 
{
  // obtencion de valor de temperatura
  TEMPERATURA = dht.readTemperature();	// obtencion de valor de temperatura
  HUMEDAD = dht.readHumidity();		// obtencion de valor de humedad
}
void humair(int TEMPERATURA,int HUMEDAD)
{
 if (air_flag == "OFF" && (TEMPERATURA > 26 || HUMEDAD > 58) )
    { 
      Serial.println("bajar temp o hum");
      airon();
    }
  if (TEMPERATURA <= 26 && HUMEDAD <= 58 && air_flag == "OFF" )
    { 
      Serial.println("temp o hum ok");
      airoff();
    }         
}

void air_renew ()
{
    air_flag="ON";
    Serial.println("Aire programado");
    airon();
    Alarm.timerOnce(15, air_renew_fin);
    Serial.println("Aire programado para apagar");
}
