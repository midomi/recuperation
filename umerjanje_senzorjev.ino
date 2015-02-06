

// Program za krmiljenje priprave zraka z rekuperacijo in 
// dogrevanjem/pohlajevanjem s pomočjo toplotne črpalke 

// knjižnice

#include <OneWire.h>
#include <DallasTemperature.h>

// digitalna tipala priključena na port 41                                                           
#define ONE_WIRE_BUS 41
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress sensor0 = { 0x28, 0xC6, 0x7A, 0x78, 0x06, 0x00, 0x00, 0x86 };
DeviceAddress sensor1 = { 0x28, 0x8E, 0x6D, 0x6E, 0x06, 0x00, 0x00, 0x4E };
DeviceAddress sensor2 = { 0x28, 0x81, 0x2D, 0x70, 0x06, 0x00, 0x00, 0xAE };
DeviceAddress sensor3 = { 0x28, 0xA9, 0x60, 0x70, 0x06, 0x00, 0x00, 0xBC };
DeviceAddress sensor4 = { 0x28, 0x39, 0xE1, 0x6E, 0x06, 0x00, 0x00, 0x26 };
DeviceAddress sensor5 = { 0x28, 0x23, 0x05, 0x78, 0x06, 0x00, 0x00, 0x46 };

float sensorValue[6];     // vrednost tipal
float sensorKore[] = {0.33, -0.53, 0.26, 0.40,	-0.59,	0.27};

int countSensor = 6;     // število tipal (5 analog. + 2 digit.)
int digiPin[] = {32, 34, 36, 38, 40, 42, 13, 12};  // output digital pini (LED, Beeper, rele)

                         // 0. 32 LED D0 kontrolka tipala D0
                         // 1. 34 LED D1 kontrolka tipala D1
                         // 2. 36 LED D2 kontrolka tipala D2
                         // 3. 38 LED D3 kontrolka tipala D3
                         // 4. 40 LED D4 kontrolka tipala D4
                         // 5. 42 LED D5 kontrolka tipala D5                         // 6. 44 LED D6 kontrolka tipala D10
                         // 7. 13 LED D8 Arduino   
                         // 8. 12 alarm (Beeper)
        
int countPin = 6;       // število output pinov
int taktCount = 0;

// *** setup *******************************************************
void setup() {
  // postavi digitalne pine na OUTPUT in priredi LOW vrednost:
  for (int k = 0; k < countPin; k++)  {
       pinMode(digiPin[k], OUTPUT);
       digitalWrite(digiPin[k], LOW);   
  }
 
  sensors.begin();
  Serial.begin(9600); 
  // začetne vrednosti:
       sensors.setResolution(sensor0, 12);
       sensors.setResolution(sensor1, 12);
       sensors.setResolution(sensor2, 12);
       sensors.setResolution(sensor3, 12);
       sensors.setResolution(sensor4, 12);
       sensors.setResolution(sensor5, 12);
  
}

void printTemperature(DeviceAddress deviceAddress)
{

}

void loop(void)
{ 
  delay(1000);
   sensors.requestTemperatures();
   
  taktCount++;
  Serial.print(taktCount);
  Serial.print("   ");

  sensorValue[0] = sensors.getTempC(sensor0);
  sensorValue[1] = sensors.getTempC(sensor1);
  sensorValue[2] = sensors.getTempC(sensor2);
  sensorValue[3] = sensors.getTempC(sensor3);
  sensorValue[4] = sensors.getTempC(sensor4);
  sensorValue[5] = sensors.getTempC(sensor5);
  for (int k = 0; k < countPin; k++)  {
       sensorValue[k] = sensorValue[k] + sensorKore[k];
     
      if (sensorValue[k] == -127.00) {
          Serial.print("Error getting temperature");
       } else {
//    Serial.print("C: ");
    
       Serial.print(sensorValue[k]);
       Serial.print("   ");
       }
  }
  Serial.print("\n\r");
}


