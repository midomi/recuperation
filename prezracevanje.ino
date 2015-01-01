// knjižnice
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <OneWire.h>
#include <DallasTemperature.h>

// digitalna tipala priključena na port 41
#define ONE_WIRE_BUS 41
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ***************************   nastavljive spremenljivke  **************
         // vrednosti pogojev delovanja toplotnih izmenjevalnikov A in B 
float minA = 12.0;       // minimum °C zraka na vhodu za dogrevanje (tipalo A1)
float minB = 12.0;          // minimum °C zraka na vpihu za dogrevanje (tipalo A3) 
float maxA = 25.0;          // maximum °C zraka na vhodu(/vpihu za pohlajevanje (tipalo A1/A3)
float maxB = 23.0;          // maximum °C zraka na vhodu(/vpihu za pohlajevanje (tipalo A1/A3)
// float miniVoda = 3.0;       // minimum °C vode v izmenjevalniku A za ALARM 
// float minMix = 10.0;    // začetna vrednost zunanje temp.
// float maxMix = 12.0;    // začetna vrednost zunanje temp.

// float tempZraka = 10.0;     // začetna vrednost zunanje temp.
   // 0. trenutna temp. dogretega zraka na vhodu
   // 1. prejšnja temp. dogretega zraka na vhodu
   // 2. pred prejšnja temp. dogretega zraka na vhodu
   // 3. mejna vrednost temp. dogretega zraka na vhodu za mix
float tempZrak[] = {10, 0, 0, 9};

         // tipla {   A0,    A1,    A2,    A3,    A4}         
float korek[] =   {-0.50, -0.50, -0.50, -0.50, -1.50}; // korekcijski faktor analognih tipal
int uporValue[] = { 1049,  1049,  1049,  1049,  1049}; // pretvornik iz Ohm v °C 
int indexValue = 60;    // število zaporednih branj tipal
int indCrpalke = 0;     // števec rotacije obtočne črpalke
int index;              // števec prebranih vhodov
int swPoletje;          // svič prvega prehoda na poletje za zapiranje ogrevanja

// ****************************************************************************************
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

// text in pozicije za prikaz display-a
char textAA[7][7] = {"voda  ", "zunaj ", "A.izm ", "rekup ", "B.izm ", "dile  ", "box   "};
int tRow[] =        {  175,      50,        70,       90,      110,     135,      155}; // vrstica teksta 
int pRow[] =        {  183,      58,        78,       98,      118,     143,      163}; // vrstica kroga 

// določanje analognih vhodnih pinov
   // A8 tipalo A0   povranta voda na izmenjevalniku A
   // A9 tipalo A1   zunaj  - zunanji zrak
   // A10 tipalo A2  zunaj+ - zunanji zrak dogret/pohlajen s TC
   // A11 tipalo A3  dovod  - dovod zraka iz rekuperatorja
   // A12 tipalo A4  dovod+ - dovod zraka iz rek. dogret/pohlajen s TC
int sensorPin[] = {A8, A9, A10, A11, A12};   // input pini analognih tipal
int countSensor = 7;   // število tipal (5 analog. + 2 digit.)

// določanje digitalnih output pinov
   // 0. 32 LED D0 kontrolka tipala A0
   // 1. 34 LED D1 kontrolka tipala A1
   // 2. 36 LED D2 kontrolka tipala A2
   // 3. 38 LED D3 kontrolka tipala A3
   // 4. 40 LED D4 kontrolka tipala A4
   // 5. 42 LED D5 kontrolka tipala D9
   // 6. 44 LED D6 kontrolka tipala D10
   // 7. 13 LED D8 Arduino   
   // 8. 12 alarm (Beeper)
   // 9. 47 rele za vklop mešalnega ventila
   // 10. 48 mešalni ventil (mirovni- zaprta topla voda, delovni- miksa)
   // 11. 49 obtočna črpalka
   // 12. 50 releB elektromotornega ventila za izmenjevalnik B 
   // 13. 51 T-kos tla-strop (ni aktiven!)
int digiPin[] = {32, 34, 36, 38, 40, 42, 44, 13, 12, 47, 48, 49, 50, 51};  // output digital pini (LED, Beeper, rele)
int countPin = 14;    // število output pinov

float sensorValue[7];   // vrednost tipal
float vrstaValue[7];    // vsota zaporedno prebranih vrednosti tipal
float vrstaAValue[7];   // povprečna vrednost temperatur
float ohmValue[7];      // preračunana vrednost v Ohm
float tempValue[7];     // temperatura 
float MIN[7];           // minimalna temperatura tipal
float MAX[7];           // maximalna temperatura tipal
int indexVal[7];        // index prebranih vrednosti tipal



// limit prebranih vrednosti tipal
float limitMin = -40;   // minimum limit -40°C 
float limitMax = 50;    // maximun limit +50°C
float trendMin;         // trend padanja temperature
float trendMax;         // trend naraščanja temperature

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
     initVrednost();
     for (int k = 0; k < countSensor; k++)  {
          MIN[k] = 50;
          MAX[k] = 0;
       }
 
  // nastavitve display-a
     tft.reset();
     uint16_t identifier = tft.readID();
     tft.begin(identifier);
     uint8_t rotation=1;
     tft.setRotation(rotation);
       tft.fillScreen(BLACK);

}
 
// *** inicializacija vrednosti ************************************
void initVrednost() {
  for (int k = 0; k < countSensor; k++)  {
    sensorValue[k] = 0;
    vrstaValue[k] = 0;
    vrstaAValue[k] = 0;
    ohmValue[k] = 0;
    indexVal[k] = 0;
    tempValue[k] = 99.9;
  }
  index = 0;
}

// *** branje vrednosti tipal **************************************
void beriSensor() {
  // beri vrednost analognih tipal
  for (int k = 0; k < (countSensor - 2); k++)  {
       sensorValue[k] = analogRead(sensorPin[k]);
       ohmValue[k] = (float)uporValue[k] / ((1023.0 / (float)sensorValue[k]) - 1.0); // preračun v Ohm
       sensorValue[k] = ((ohmValue[k] - 1000.0) / 3.9) + korek[k];                   // preračun v °C 
     }
  // beri vrednost digitalnih tipal
  sensors.requestTemperatures();
  sensorValue[5] = sensors.getTempCByIndex(0);
  sensorValue[6] = sensors.getTempCByIndex(1);
 // trenutna vrednost temp. zraka pred rekuperacijo
  tempZrak[0] = sensorValue[2];
}

// *** test tipal **************************************************
void testSensor(){ 
  // test mejnih vrednosti tipal (-40°C do +50°C)
  for (int k = 0; k < countSensor; k++)  {
     if (sensorValue[k] > limitMin and sensorValue[k] < limitMax){
         vrstaValue[k] = vrstaValue[k] + sensorValue[k];
         indexVal[k]++;
     }
  }
}

// *** glavna zanka ************************************************
void loop() 
{
  beriSensor();
    if (index == 0) {
        Serial.print("\n");
        Serial.println("     A0      A1      A2      A3      A4      A5      A6      TEMP.VODE   ");
       } 
  for (int k = 0; k < countSensor; k++) {
         if (k == 0) {
             Serial.print(index+1);
             Serial.print(".  ");
            }      
         Serial.print(sensorValue[k]);
         Serial.print("   ");
     }
                         Serial.print(" [0.=");
                    Serial.print(tempZrak[0],1); 
                    Serial.print("]  [1.=");
                    Serial.print(tempZrak[1],1); 
                    Serial.print("]  [2.=");
                    Serial.print(tempZrak[2],1);
                    Serial.print("]");
     Serial.print("\n");
    testSensor();
    
    index = index + 1;
//    indCrpalke = indCrpalke + 1;
    // timer kača
    narisiCrto(26+index, 25, 26+index, 34, CYAN);
   
    // obtočna črpalka
    narisiKrog(255, 210, 10, RED, 0);
    if (digitalRead(digiPin[11]) == HIGH){
        switch (indCrpalke){
            case 1: 
               narisiCrto(252, 205, 252, 206, CYAN);
               narisiCrto(253, 203, 253, 208, CYAN);
               narisiCrto(254, 202, 254, 209, CYAN);
               narisiCrto(255, 201, 255, 210, CYAN);
               narisiCrto(256, 203, 256, 209, CYAN);
               narisiCrto(257, 202, 257, 208, CYAN);
               narisiCrto(258, 205, 258, 206, CYAN);
                  narisiCrto(249, 204, 249, 207, BLACK);
                  narisiCrto(250, 204, 250, 209, BLACK);
                  narisiCrto(251, 204, 251, 209, BLACK);
                  narisiCrto(252, 204, 252, 210, BLACK);
                  narisiCrto(253, 205, 246, 210, BLACK);
                  narisiCrto(254, 205, 246, 210, BLACK);
                  narisiCrto(255, 207, 246, 210, BLACK);
            break:
            case 2:              
               narisiCrto(261, 204, 261, 207, CYAN);
               narisiCrto(260, 204, 260, 209, CYAN);
               narisiCrto(259, 204, 259, 209, CYAN);
               narisiCrto(258, 204, 258, 210, CYAN);
               narisiCrto(257, 205, 257, 210, CYAN);
               narisiCrto(256, 205, 256, 210, CYAN);
               narisiCrto(255, 207, 255, 210, CYAN);
                  narisiCrto(252, 205, 253, 206, BLACK);
                  narisiCrto(253, 203, 253, 208, BLACK);
                  narisiCrto(254, 202, 254, 209, BLACK);
                  narisiCrto(255, 201, 255, 210, BLACK);
                  narisiCrto(256, 203, 256, 209, BLACK);
                  narisiCrto(257, 202, 257, 208, BLACK);
                  narisiCrto(258, 205, 258, 206, BLACK);
            break:
            case 3: 
               narisiCrto(255, 210, 255, 210, CYAN);
               narisiCrto(256, 209, 256, 211, CYAN);
               narisiCrto(257, 208, 257, 212, CYAN);
               narisiCrto(258, 208, 258, 212, CYAN);
               narisiCrto(259, 207, 259, 213, CYAN);
               narisiCrto(260, 207, 260, 213, CYAN);
               narisiCrto(261, 208, 261, 212, CYAN);
               narisiCrto(262, 208, 262, 212, CYAN);
               narisiCrto(263, 209, 263, 211, CYAN);
               narisiCrto(264, 210, 264, 210, CYAN);
                  narisiCrto(261, 204, 261, 207, BLACK);
                  narisiCrto(260, 204, 260, 209, BLACK);
                  narisiCrto(259, 204, 259, 209, BLACK);
                  narisiCrto(258, 204, 258, 210, BLACK);
                  narisiCrto(257, 205, 257, 210, BLACK);
                  narisiCrto(256, 205, 256, 210, BLACK);
                  narisiCrto(255, 207, 255, 210, BLACK);
            break:
            case 4:
               narisiCrto(261, 213, 261, 216, CYAN);
               narisiCrto(260, 211, 260, 216, CYAN);
               narisiCrto(259, 211, 259, 216, CYAN);
               narisiCrto(258, 210, 258, 216, CYAN);
               narisiCrto(257, 210, 257, 215, CYAN);
               narisiCrto(256, 210, 256, 215, CYAN);
               narisiCrto(255, 210, 255, 213, CYAN);
                  narisiCrto(255, 210, 255, 210, BLACK);
                  narisiCrto(256, 209, 256, 211, BLACK);
                  narisiCrto(257, 208, 257, 212, BLACK);
                  narisiCrto(258, 208, 258, 212, BLACK);
                  narisiCrto(259, 207, 259, 213, BLACK);
                  narisiCrto(260, 207, 260, 213, BLACK);
                  narisiCrto(261, 208, 261, 212, BLACK);
                  narisiCrto(262, 208, 262, 212, BLACK);
                  narisiCrto(263, 209, 263, 211, BLACK);
                  narisiCrto(264, 210, 264, 210, BLACK);
            break:
            case 5:
               narisiCrto(252, 214, 252, 215, CYAN);
               narisiCrto(253, 212, 253, 217, CYAN);
               narisiCrto(254, 211, 254, 218, CYAN);
               narisiCrto(255, 210, 255, 219, CYAN);
               narisiCrto(256, 211, 256, 218, CYAN);
               narisiCrto(257, 212, 257, 217, CYAN);
               narisiCrto(258, 214, 258, 215, CYAN);
                  narisiCrto(261, 213, 261, 216, BLACK);
                  narisiCrto(260, 211, 260, 216, BLACK);
                  narisiCrto(259, 211, 259, 216, BLACK);
                  narisiCrto(258, 210, 258, 216, BLACK);
                  narisiCrto(257, 210, 257, 215, BLACK);
                  narisiCrto(256, 210, 256, 215, BLACK);
                  narisiCrto(255, 210, 255, 213, BLACK);
            break:
            case 6:  
               narisiCrto(252, 213, 252, 216, CYAN);
               narisiCrto(253, 214, 253, 216, CYAN);
               narisiCrto(254, 214, 254, 216, CYAN);
               narisiCrto(255, 210, 255, 216, CYAN);
               narisiCrto(256, 210, 256, 215, CYAN);
               narisiCrto(257, 210, 257, 215, CYAN);
               narisiCrto(258, 210, 258, 213, CYAN);
                  narisiCrto(252, 214, 252, 215, BLACK);
                  narisiCrto(253, 212, 253, 217, BLACK);
                  narisiCrto(254, 211, 254, 218, BLACK);
                  narisiCrto(255, 210, 255, 219, BLACK);
                  narisiCrto(256, 211, 256, 218, BLACK);
                  narisiCrto(257, 212, 257, 217, BLACK);
                  narisiCrto(258, 214, 258, 215, BLACK);
            break:
            case 7:  
               narisiCrto(255, 210, 255, 210, CYAN);
               narisiCrto(254, 209, 254, 211, CYAN);
               narisiCrto(253, 208, 253, 212, CYAN);
               narisiCrto(252, 208, 252, 212, CYAN);
               narisiCrto(251, 207, 251, 213, CYAN);
               narisiCrto(250, 207, 250, 213, CYAN);
               narisiCrto(249, 208, 249, 212, CYAN);
               narisiCrto(248, 208, 248, 212, CYAN);
               narisiCrto(247, 209, 247, 211, CYAN);
               narisiCrto(246, 210, 246, 210, CYAN);
                  narisiCrto(252, 213, 252, 216, BLACK);
                  narisiCrto(253, 214, 253, 216, BLACK);
                  narisiCrto(254, 214, 254, 216, BLACK);
                  narisiCrto(255, 210, 255, 216, BLACK);
                  narisiCrto(256, 210, 256, 215, BLACK);
                  narisiCrto(257, 210, 257, 215, BLACK);
                  narisiCrto(258, 210, 258, 213, BLACK);
            break:
            case 8:          
               narisiCrto(249, 204, 249, 207, CYAN);
               narisiCrto(250, 204, 250, 209, CYAN);
               narisiCrto(251, 204, 251, 209, CYAN);
               narisiCrto(252, 204, 252, 210, CYAN);
               narisiCrto(253, 205, 246, 210, CYAN);
               narisiCrto(254, 205, 246, 210, CYAN);
               narisiCrto(255, 207, 246, 210, CYAN);
                  narisiCrto(255, 210, 255, 210, CYAN);
                  narisiCrto(254, 209, 254, 211, CYAN);
                  narisiCrto(253, 208, 253, 212, CYAN);
                  narisiCrto(252, 208, 252, 212, CYAN);
                  narisiCrto(251, 207, 251, 213, CYAN);
                  narisiCrto(250, 207, 250, 213, CYAN);
                  narisiCrto(249, 208, 249, 212, CYAN);
                  narisiCrto(248, 208, 248, 212, CYAN);
                  narisiCrto(247, 209, 247, 211, CYAN);
                  narisiCrto(246, 210, 246, 210, CYAN);
               indCrpalke = 0;    
         break:
        }      
    }    

   // --- zanka zaporedno prebranih vrednosti --- 
    if (index == indexValue) {
          // preračun povprečne trenutne temperature         
          // preračun MIN MAX temperature
          for (int k = 0; k < countSensor; k++)  {
              if (indexVal[k] > (index / 2)) {
                  tempValue[k] = ((vrstaValue[k] / indexVal[k]) );     // preračun povprečne vrednosti zaporedno prebranih
                  if (tempValue[k] < MIN[k])
                      MIN[k] = tempValue[k];
                  if (tempValue[k] > MAX[k])
                      MAX[k] = tempValue[k];
                 }
          }
 
          // ALARM - kontrola zmrzovanja vode  na vhodu zraka (A izmenjevalnik)  
          //if (tempValue[0] < miniVoda)                                                   
          //    digitalWrite(digiPin[8], HIGH);                                                  
          //else                                                                    
          //    digitalWrite(digiPin[8], LOW);
              
          // vklop - izklop dogrevanja/pohlajevanja zraka na vpihu (B izmenjevalnik)
          if (tempZrak[0] < minB){                            
              digitalWrite(digiPin[12], HIGH);     // ogrevanje
          }
          else{
              if (tempZrak[0] > maxB)
                 digitalWrite(digiPin[12], HIGH);  // hlajenje
              else 
                 digitalWrite(digiPin[12], LOW);   // baypass
          }
         
          // usmerjanje zraka pozimi-poleti v tla-strop        
 //         if (tempZraka < minA)                      
 //             digitalWrite(digiPin[13], HIGH);     // usmeri zrak v tla 
 //         else
 //             digitalWrite(digiPin[13], LOW);      // usmeri zrak v strop 
    
   //izpis vrednosti
      // glava zapisa
        Serial.print("           voda ");
        Serial.print(" zunaj  ");
        Serial.print(" tempVZ ");
        Serial.print(" zunaj+ ");
        Serial.print(" dovod  "); 
        Serial.print(" dovod+ "); 
        Serial.print(" dile   "); 
        Serial.print(" box    "); 
        Serial.print("\n"); 

     // minimalna temperatura 
        Serial.print("MIN =      ");
        for (int k = 0; k < countSensor; k++)  {
             Serial.print(MIN[k],1); 
             Serial.print("   ");
        }        
        Serial.print("\n"); 
     // trenutna temperatura
        Serial.print("trenutna = ");
        for (int k = 0; k < countSensor; k++)  {
             Serial.print(tempValue[k],1); 
             Serial.print("   ");
        }        
        Serial.print("\n");
     // maximalna temperatura
        Serial.print("MAX =      ");
        for (int k = 0; k < countSensor; k++)  {
             Serial.print(MAX[k],1); 
             Serial.print("   ");
        }
        Serial.print("\n");
        
     // klic rutine izpisTextdisplay
        izpisTextdisplay();
     
     // inicializacija vrednosti spremenljivk
        initVrednost(); 
    }                                         
    // --- konec if zanke (index == indexValue) --- 
    
    // vklop - izklop dogrevanja/pohlajevanja zraka na vhodu (A izmenjevalnik)

    if (tempZrak[0] < minA){      // ZIMA 
        swPoletje = 0;
        digitalWrite(digiPin[11], HIGH);          // obtočna črpalka dela  
        digitalWrite(digiPin[9], HIGH);           // miksaj 
        // varovalka pred zmrzaljo - dogret zrak na A.izmenjevalniku pod 4°C! 
        if (tempZrak[0] < 4.0){
             digitalWrite(digiPin[10], HIGH);  // dodaja toplo vodo
             Serial.print(" mix GREJE alarm  ");
        }  
        // konec varovalke pred zmrzaljo  
        else{
             mixVode(); 
        }        
    }
    else{                                         // POLETJE
        digitalWrite(digiPin[10], LOW);
        if (swPoletje == 0){                   
           digitalWrite(digiPin[9], HIGH);        // ogrevanje zaprto
           delay(210000);
           swPoletje = 1;
    }
        if (tempZrak[0] > maxA)                  // hlajenje
            digitalWrite(digiPin[11], HIGH);      // obtočna črpalka dela
        else
            digitalWrite(digiPin[11], LOW);
    }
                     Serial.print(tempZrak[0],1); 
                     Serial.print("    ");
                     Serial.print(tempZrak[3],1); 
                     Serial.print("   ");
                     Serial.print(sensorValue[2],1);
                     Serial.print("\n"); 


  // turn the ledPin on
  digitalWrite(digiPin[7], HIGH);  
  // stop the program for <sensorValue> milliseconds:
  delay(500);          
  // turn the ledPin off:        
  digitalWrite(digiPin[7], LOW);   
  // stop the program for for <sensorValue> milliseconds:
  delay(500);   

}
// *** konec glavne zanke ******************************************

// *** mixanje vode *******************************************
unsigned long mixVode() {
      if (tempZrak[0] < tempZrak[1]){
          if (tempZrak[1] < tempZrak[2]){
              if (tempZrak[0] < tempZrak[3]){      
                  Serial.print(" mix GREJE   ");
                  digitalWrite(digiPin[10], HIGH);  // dodaja toplo vodo
              }
              tempZrak[2] = tempZrak[1]; 
          }
          tempZrak[1] = tempZrak[0];
      }
      if (tempZrak[0] > tempZrak[1]){
          if (tempZrak[1] > tempZrak[2]){
         //   if (tempVode[0] > tempVode[4]){      
              Serial.print(" mix HLADI   ");
              digitalWrite(digiPin[10], LOW);  // dodaja hladno vodo
         // }
              tempZrak[2] = tempZrak[1];             
          }
          tempZrak[1] = tempZrak[0];
      }
}              

// *** prikaz na display *******************************************
unsigned long izpisTextdisplay() {
  tft.fillScreen(BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(YELLOW);  tft.setTextSize(2);
  tft.println(" TEMPERATURE PREZRACEVANJA");
  tft.setTextColor(WHITE);  tft.setTextSize(1);
  tft.println();
  narisiCrto(24, 22, 90, 22, WHITE);
  narisiCrto(24, 37, 90, 37, WHITE);
  narisiCrto(24, 22, 24, 37, WHITE);
  narisiCrto(90, 22, 90, 37, WHITE);
  tft.setTextSize(2);
  tft.setCursor(110, 25);
  tft.print("MINI  TEMP  MAXI");
  narisiCrto(0, 47, 320, 47, WHITE);
  
    // prikaz temperatur na display
       for (int k = 1; k < (countSensor); k++)  {
            if (k == 5)
                  narisiCrto(0, 132, 320, 132, WHITE);
            if (tempValue[k] == 99.9) {                                                     
                digitalWrite(digiPin[k], HIGH);
                narisiKrog(8, pRow[k], 5, GREEN, 0);
               }
            else {        
                digitalWrite(digiPin[1], LOW);             
                narisiKrog(8, pRow[k], 5, GREEN, 1);
            }
            tft.setTextColor(WHITE); 
            tft.setCursor(25, tRow[k]);
            for ( int n = 0;  n < 6; n++) {
                  tft.print(textAA[k][n]);
               }
            if (MIN[k] > -10){
               tft.print(" ");}
            if (MIN[k] < 10){
               if(MIN[k] > -0.1)
                 tft.print(" ");} 
            tft.setTextColor(GREEN);   
            tft.print(MIN[k],1);
            tft.print(" ");
            if (k == 1 or k == 3)
                tft.setTextColor(CYAN); 
            else    
                tft.setTextColor(YELLOW);                 
            if (tempValue[k] > -10){
               tft.print(" ");}
            if (tempValue[k] < 10){
               if(tempValue[k] > -0.1)
                 tft.print(" ");} 
            tft.print(tempValue[k],1);
            tft.print(" ");
            if (MAX[k] > -10){
               tft.print(" ");}
            if (MAX[k] < 10){
               if(MAX[k] > -0.1)
                 tft.print(" ");} 
            tft.setTextColor(RED);
            tft.print(MAX[k],1);
/*         // trend gibanja temperatur
            trendMax = MAX[k] - tempValue[k];
            trendMin = tempValue[k] - MIN[k];
            if (trendMax < 0)
                trendMax = trendMax * (-1);
            if (trendMin < 0)
                trendMin = trendMin * (-1);
             Serial.print(" MIN =   ");                
             Serial.print(MIN[k]); 
             Serial.print("Trend MIN =   ");                
             Serial.print(trendMin); 
             Serial.print("  Temp VALUE =   ");
             Serial.print(tempValue[k]); 
             Serial.print(" MAX =   ");                
             Serial.print(MAX[k]); 
             Serial.print("  Trend MAX =  ");
             Serial.print(trendMax); 
            Serial.println("   ");
            if (trendMax == trendMin) {
                tft.setTextColor(WHITE);
                tft.setCursor(312, tRow[k]);
                tft.print("=");
               }
            else {
                if (trendMax < trendMin) {
                    narisiCrto(315, tRow[k]+2, 315, tRow[k]+15, WHITE);
                    narisiCrto(315, tRow[k]+2, 312, tRow[k]+5, WHITE);
                    narisiCrto(315, tRow[k]+2, 318, tRow[k]+5, WHITE);
                   }
                else { 
                    narisiCrto(315, tRow[k]+15, 315, tRow[k]+2, WHITE);
                    narisiCrto(315, tRow[k]+15, 312, tRow[k]+10, WHITE);
                    narisiCrto(315, tRow[k]+15, 318, tRow[k]+10, WHITE);
               }
          }
*/     }  

    // prikaz VODA temperature
    if (tempValue[0] == 99.9) {                                                    
        digitalWrite(digiPin[0], HIGH);
        narisiKrog(8, pRow[0], 5, GREEN, 0);
       }
    else {        
        digitalWrite(digiPin[0], LOW);
        narisiKrog(8, pRow[0], 5, GREEN, 1);
       }
  tft.setTextColor(WHITE);
  tft.setCursor(25, tRow[0]);
  tft.print("voda  ");
  tft.setTextColor(GREEN);
  if (MIN[0] > -10){
      tft.print(" ");}
  if (MIN[0] < 10){
     if(MIN[0] > -0.1)
       tft.print(" ");}     
  tft.print(MIN[0],1);
  tft.print(" "); 
  tft.setTextColor(YELLOW);
  if (tempValue[0] > -10){
      tft.print(" ");}
  if (tempValue[0] < 10){
      if(tempValue[0] > -0.1)
         tft.print(" ");}
  tft.print(tempValue[0],1);
  tft.print(" "); 
  tft.setTextColor(RED);
  if (MAX[0] > -10){
      tft.print(" ");}
  if (MAX[0] < 10){
      if(MAX[0] > -0.1)
         tft.print(" ");} 
  tft.print(MAX[0],1);
           // trend gibanja temperatur
            trendMax = MAX[0] - tempValue[0];
            trendMin = tempValue[0] - MIN[0];
            if (trendMax < 0)
                trendMax = trendMax * (-1);
            if (trendMin < 0)
                trendMin = trendMin * (-1);
                
            if (trendMax == trendMin) {
                tft.setTextColor(WHITE);
                tft.setCursor(312, tRow[0]);
                tft.print("=");
               }
            else {
                if (trendMax < trendMin) {
                    narisiCrto(315, tRow[0]+2, 315, tRow[0]+15, WHITE);
                    narisiCrto(315, tRow[0]+2, 312, tRow[0]+5, WHITE);
                    narisiCrto(315, tRow[0]+2, 318, tRow[0]+5, WHITE);
                   }
                else { 
                    narisiCrto(315, tRow[0]+15, 315, tRow[0]+2, WHITE);
                    narisiCrto(315, tRow[0]+15, 312, tRow[0]+10, WHITE);
                    narisiCrto(315, tRow[0]+15, 318, tRow[0]+10, WHITE);
               }
            }
  tft.println();
  narisiCrto(0, 198, 320, 198, WHITE);

  // prikaz pogojev A
  tft.setCursor(25, 205);
  tft.setTextColor(WHITE); 
  tft.print("A ");
  tft.print(minA, 1);
  tft.print(">");
  tft.setTextColor(CYAN); 
  tft.print(tempValue[1], 1);
  tft.setTextColor(WHITE);
  tft.print(">");
  tft.print(maxA, 1);
  if (digitalRead(digiPin[9]) == HIGH) {
       narisiKrog(8, 210, 8, RED, 1);
     }
  else {
      tft.print("        ");
      narisiKrog(8, 210, 8, RED, 0);
     }      

  // prikaz pogojev B
  tft.setCursor(25, 225);  
  tft.print("B ");
  tft.print(minB,1);
  tft.print(">");
  tft.setTextColor(CYAN);  
  tft.print(tempValue[1],1);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.print(" ali ");
  tft.setTextColor(CYAN); 
  tft.setTextSize(2);
  tft.print(tempValue[3],1);
  tft.setTextColor(WHITE);
  tft.print(">");
  tft.print(maxB,1);
  if (digitalRead(digiPin[12]) == HIGH){
      narisiKrog(8, 230, 8, RED, 1);
     }
  else {
      narisiKrog(8, 230, 8, RED, 0);
     }
  return micros() - start;
// konec izpisTextdisplay()
}

unsigned long narisiKrog(uint8_t x, uint8_t y, uint8_t radius, uint16_t color, uint8_t fill) {
  if(fill == 1){
     tft.drawCircle(x, y, radius, color);
     tft.fillCircle(x, y, radius, color);
    }
  else {
     tft.drawCircle(x, y, radius, color);
    }
// konec narisiKrog()
}
  unsigned long narisiCrto(uint16_t x, uint16_t y, uint16_t x1, uint16_t y1, uint16_t color) {
    tft.drawLine(x, y, x1, y1, color);
// konec nariciCrto()
}
