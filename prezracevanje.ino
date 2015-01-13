// Program za krmiljenje priprave zraka z rekuperacijo in 
// dogrevanjem/pohlajevanjem s pomočjo toplotne črpalke 

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
float minB = 12.0;       // minimum °C zraka na vpihu za dogrevanje (tipalo A3) 
float maxA = 25.0;       // maximum °C zraka na vhodu/vpihu za pohlajevanje (tipalo A1)
float maxB = 23.0;       // maximum °C zraka na vhodu/vpihu za pohlajevanje (tipalo A3)
float minGreje = 8.0;    // mejna vrednost dogrevanja pozimi (tipalo A2) 
float maxGreje = 12.0;   // mejna vrednost ohlajanja pozimi (tipalo A2)   
float minVoda = 3.0;     // mejna vrednost delovanja rekuperatorja (tipalo A0) 

        // takt delovanja priprave zraka A.izmnenjevalnika
                         // 0. začetno število sekund inpulsa mešanja
                         // 1. število sekund takta mešanja
                         // 2. število sekund mešanja ob prehodu na poletje 
                         // 3. izračunano število sekund inpulsa mešanja 
int taktValue[] = {6, 20, 210, 0};
                         // 0. števec delovanja mešalnega ventila
                         // 1. števec inpulsa mešanja 
                         // 2. števec prikaza delovanja zapornega ventila na display
                         // 3. števec prikaza delovanja mešalnega ventila
int taktCount[] = {0, 0, 0, 0};     

         // tipla {   A0,    A1,    A2,    A3,    A4}         
float korek[] =   {-0.50, -0.50, -0.50, -0.50, -1.50}; // korekcijski faktor analognih tipal
int uporValue[] = { 1049,  1049,  1049,  1049,  1049}; // pretvornik iz Ohm v °C 
int indexValue = 60;     // število zaporednih branj tipal
int index = 0;           // števec prebranih vhodov
int swVrten = 0;         // števec rotacije obtočne črpalke
int swMix = 0;           // svič pozicije mešalnega ventila
int swMotor = 0;         // svič delovanja mešalnega ventila
int swObtok = 0;         // svič delovanja obtočne črpalke
int swVentil = 2;        // svič prikaza delovanja ventila
int sVentil = 2;         // svič prikaza delovanja ventila - prejšnji
int swUpDown;            // svič padanja/naraščanja temp.
                         // 0. prvič zima
                         // 1. prvič poletje
                         // 2. prvič greje
                         // 3. prvič hladi
int swPrvic[] = {1, 1, 1, 1};  

// ****************************************************************************************
#define LCD_CS A3        // Chip Select goes to Analog 3
#define LCD_CD A2        // Command/Data goes to Analog 2
#define LCD_WR A1        // LCD Write goes to Analog 1
#define LCD_RD A0        // LCD Read goes to Analog 0

#define LCD_RESET A4     // Can alternately just connect to Arduino's reset pin

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
char textAA[7][7] =    {"voda ", "zunaj", "A.izm", "rekup", "B.izm", "dile ", "box  "};
int tRow[] =        {  50,      70,      90,      110,     130,     150,     170};            // vrstica teksta 
int pRow[] =        {  58,      78,      98,      118,     138,     158,     178};            // vrstica kroga 
int tCol[] =        {   8,      25,      95,      165,     235,     312,     315,     318};   // stolpci tabele 

         // določanje analognih vhodnih pinov
                         // A8 tipalo A0   povranta voda na izmenjevalniku A
                         // A9 tipalo A1   zunaj  - zunanji zrak
                         // A10 tipalo A2  zunaj+ - zunanji zrak dogret/pohlajen s TC
                         // A11 tipalo A3  dovod  - dovod zraka iz rekuperatorja
                         // A12 tipalo A4  dovod+ - dovod zraka iz rek. dogret/pohlajen s TC
int sensorPin[] = {A8, A9, A10, A11, A12};   // input pini analognih tipal
int countSensor = 7;                         // število tipal (5 analog. + 2 digit.)

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
                         // 13. 51 rekuperator (mirovni - vklop, delovni - izklop)
int digiPin[] = {32, 34, 36, 38, 40, 42, 44, 13, 12, 47, 48, 49, 50, 51};  // output digital pini (LED, Beeper, rele)
int countPin = 14;       // število output pinov

float sensorValue[7];    // vrednost tipal
float vrstaValue[7];     // vsota zaporedno prebranih vrednosti tipal
float vrstaAValue[7];    // povprečna vrednost temperatur
float ohmValue[7];       // preračunana vrednost v Ohm
float tempValue[7];      // temperatura 
float MIN[7];            // minimalna temperatura tipal
float MAX[7];            // maximalna temperatura tipal
int indexVal[7];         // index prebranih vrednosti tipal
float sValue[7];         // vrednost predhodne temp. tipal
float stValue[7];        // vrednost predhodne temp. tipal za izpis trenutne temp.
float staValue;          // vrednost predhodne temp. tipala A2 - mix naraščanja/padanja temp.
float zap[7];            // število zaporedno prebranih
float param = 0.0;
float temp;

         // limit prebranih vrednosti tipal
float limitMin = -40;    // minimum limit -40°C 
float limitMax = 50;     // maximun limit +50°C
float trendMin[7][2];    // trend padanja temperature
float trendMax[7][2];    // trend naraščanja temperature

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
     izpisTextdisplay();     // prikaz glave na display
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
       if (sensorValue[k] == sValue[k]) {            //
           zap[k]++;                                 //                                   
           if (zap[k] > 3) {                         //                            
               zap[k] = 3;                           //
           }                                         // 3 x zaporedoma prebrana ista temp.
       }                                             //
       else {                                        //  
           zap[k] = 1;                               //
       }                                             // 
       sValue[k] = sensorValue[k];                   //    
  }

  // beri vrednost digitalnih tipal
  sensors.requestTemperatures();
  sensorValue[5] = sensors.getTempCByIndex(0);
  sensorValue[6] = sensors.getTempCByIndex(1);  
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

//*****************************************************************************************
// *** glavna zanka ***********************************************************************
void loop() {
    beriSensor();
    taktCount[0]++;        
    if (taktCount[0] > taktValue[1]) {
        taktCount[0] = 1;                      // začetna vrednost takta
        taktValue[3] = taktValue[3] - 1;       // zmanjšam vrednost inpulsa za vsak takt (20)
    } 
    
    // A.izmenjevalnik ............................VAROVALKA ..................................
    if (sensorValue[0] < minVoda) {                              // voda < 3°C                .  
      digitalWrite(digiPin[13], HIGH);                           // rekuperator OFF           .
      digitalWrite(digiPin[8], HIGH);                            // ALARM piskač              .
      digitalWrite(digiPin[10], HIGH);                           // dodaja TOPLO vodo         .                              
      digitalWrite(digiPin[9], HIGH);                            // mixaj                     .
      obtok(1);                                                  // obtočna črpalka ON        .
      swMix = 1;                                                 // ------------------------- .
      swMotor = 1;                                               //                           .
      swVentil = 1;                                              //                           .
    }                                                            //                           .
    else {              // ........................ZIMA........................................                                                   //                           .      
        digitalWrite(digiPin[13], LOW);                          // rekuperator ON            .
        if (sensorValue[1] < minA) {                             // ------------------------- .
            if (swPrvic[0] == 1) {                               //                           .
                if (zap[1] == 3) {                               //                           .
                    obtok(1);                                    // obtočna črpalka ON        .
                    swPrvic[0] = 0;                              //                           .
                    swPrvic[1] = 1;                              //                           .                    
                    staValue = sensorValue[2];                   //                           .
                    zima();                                      // zima prvič                .
                }                                                //                           .
            }                                                    //                           .
            else {                                               //                           .
                zima();                                          // zima                      .
              }                                                  //                           .
        }                                                        //                           .  
        else {          //.........................POLETJE ....................................
            if (swPrvic[1] == 1) {                               //                           .   
                if (zap[1] == 3) {                               //                           .
                    obtok(0);                                    // obtočna črpalka OFF       .  
                    swPrvic[1] = 0;                              //                           .
                    swPrvic[0] = 1;                              //                           .                    
                    taktCount[1] = 0;                            //                           .
                    staValue = sensorValue[2];                   //                           .                    
                    taktValue[3] = taktValue[2];                 // inpuls postavimo na 210   .                    
                    poleti();                                    // poletje prvič             .
                }                                                //                           .
            }                                                    //                           .
            else {                                               //                           .
                poleti();                                        // poletje                   .
            }                                                    //                           .
        }                                                        //                           .
    }                                                            //                           .
    // konec A.izmenjevalnik ..................................................................                       
   
 // obtočna črpalka
    if (swMix == 1 or swMix == 6 or swPrvic[0] == 0) 
        prikazCrpalka();
          
    // mešalni ventil

    for (int k = 0; k < 3; k++)  {
        if (sensorValue[k] != stValue[k]) {
            narisiKvadrat(tCol[3], tRow[k], 60, 16, BLACK, 1);
            izpisTrenutneTemp(sensorValue[k], k);
            stValue[k] = sensorValue[k];
        }
    }   
    prikazVentil(swVentil);
    
    // zaporni ventil
    izpisZaporniVentilOdprt();
       
    // timer kača
    narisiCrto(28+index, 28, 28+index, 36, CYAN);

    // izpis trenutnih vrednosti na monitor - Serial.print
    izpisMonitor(); 
    
    testSensor();
    index++;
   
    // --- zanka 60 zaporedno prebranih vrednosti .................................................... 
    if (index == indexValue) {
          // preračun povprečne trenutne, MIN in MAX temperature zaporedno prebranih        
          for (int k = 0; k < countSensor; k++)  {
              if (indexVal[k] > (index / 2)) {
                  tempValue[k] = ((vrstaValue[k] / indexVal[k]) );     
                  if (tempValue[k] < MIN[k])
                      MIN[k] = tempValue[k];
                  if (tempValue[k] > MAX[k])
                      MAX[k] = tempValue[k];
                 }
          }
 
          // B. izmenjevalnik ......................................
          if (sensorValue[2] < minB){                            
              digitalWrite(digiPin[12], HIGH);     // ogrevanje
          }
          else{
              if (sensorValue[2] > maxB)
                 digitalWrite(digiPin[12], HIGH);  // hlajenje
              else 
                 digitalWrite(digiPin[12], LOW);   // baypass
          }
          // konec B.izmenjevalnik ................................
          
          
        izpisPovprecje60();          // izpis povprečje 60 zaporedno prebranih na monitor
        izpisVrednostidisplay();     // izpis povprečje 60 zaporedno prebranih na display
        initVrednost();              // inicializacija vrednosti spremenljivk
    }                                         
    // --- konec if zanke 60 prebranih(index == indexValue) .......................................... 

  // turn the ledPin on
  digitalWrite(digiPin[7], HIGH);  
  // stop the program for <sensorValue> milliseconds:
  delay(500);          
  // turn the ledPin off:        
  digitalWrite(digiPin[7], LOW);   
  // stop the program for for <sensorValue> milliseconds:
  delay(500);   
}
// *** konec glavne zanke *****************************************************************

// *** dodatne rutine regulacije **********************************************************
unsigned long zima() {
                                             // ............*** GREJE ***............ 
    if (sensorValue[2] < minGreje) {         
        digitalWrite(digiPin[10], HIGH);     // dodaja TOPLO vodo 
        if (swPrvic[2] == 1) { 
            if (zap[2] == 3) {
                swPrvic[2] = 0;                      // postavi svič na GREJE 
                swPrvic[3] = 1;                      // postavi svič na HLADI                 
                taktCount[0] = 1;                    // postavi takt na začetek
                taktCount[1] = 0;                    // postavi števec inpulsa
                taktValue[3] = taktValue[0];         // postavi čas inpulsa na začetno vrednost  
                swUpDown = 1;                        // status padanja temp. na ON
                staValue = sensorValue[2];           // predhodna vrednost tipala A2
                swMix = 3;                           // status delovanja
                mix();                       // mix prvič
            }                
        }
        else {
            Serial.print(" zima-GREJE ");
           Serial.print(staValue);
          Serial.print("\n"); 
/*            if (sensorValue[2] > staValue)           // ugotavljanje padanja temp.
                swUpDown = 0;                        // status padanja temp. OFF
            staValue = sensorValue[2];               // popravi staro vrednost tipala A2 
                        Serial.print(" zima-GREJE swupdown ");
           Serial.print(swUpDown);
          Serial.print("\n"); 
            if (swUpDown == 1)
*/                mix();                        // mixaj le ob padanju temp.
        }
    }
                                             // ............*** MIRUJE ***.......... 
    if (sensorValue[2] > minGreje and sensorValue[2] < maxGreje) { 
        if (zap[2] == 3) {
            digitalWrite(digiPin[9], LOW);   // motorček OFF
            swMix = 2;
            swMotor = 0;
            swVentil = swMix;

        
        }    
    }          
                                             // ............*** HLADI ***............ 
    if (sensorValue[2] > maxGreje) {
        digitalWrite(digiPin[10], LOW);      // dodaja HLADNO vodo 
        if (swPrvic[3] == 1) { 
            if (zap[2] == 3) {
                swPrvic[3] = 0;                      // postavi svič na HLADI 
                swPrvic[2] = 1;                      // postavi svič na GREJE                 
                taktCount[0] = 1;                    // postavi takt na začetek
                taktCount[1] = 0;                    // postavi števec inpulsa
                taktValue[3] = taktValue[0];         // postavi čas inpulsa na začetno vrednost  
                swUpDown = 1;                        // status padanja temp. na ON
                staValue = sensorValue[2];           // predhodna vrednost tipala A2
                swMix = 4;                           // status delovanja
                mix();                       // mix prvič
            }                
        }
        else {
  /*          if (sensorValue[2] < staValue)           // ugotavljanje naraščanja temp.
              swUpDown = 0;                        // status naraščanja temp. OFF
            staValue = sensorValue[2];               // popravi staro vrednost tipala A2 
            
            if (swUpDown == 1)    
  */              mix();                       // mixaj le ob naraščanju temp.
        }
    }
}
// --- konec zima() ---------------------------------------------------------------

unsigned long poleti() {

  if (taktValue[3] > 0) {                        // inpuls  = 210s
      digitalWrite(digiPin[10], HIGH);           // odpri ventil do konca 
      mix();                                 
      swMix = 5; 
    }
    else {
        digitalWrite(digiPin[9], LOW);           // motorček OFF 
        swMotor = 0;     
        if (sensorValue[1] > maxB) {
            obtok(1);                            // obtočna črpalka ON  
            swMix = 6;
        }
        else {
            obtok(0);                            // obtočna črpalka OFF  
            swMix = 7;
        }
    }     
}
// --- konec poleti() ----------------------------------------------------------------

unsigned long mix() {
    if (taktCount[0] < taktValue[3]) {                  // po padajoči vrednosti inpulsa mix-a 
        digitalWrite(digiPin[9], HIGH);        // motorček ON
        swMotor = 1;
        swVentil = swMix;
        taktCount[3]++;                        // števec delovanja motorčka (od 0 do 5 )
        if (taktCount[3] > 5) 
            taktCount[3] = 0;        
    }    
    else {
        digitalWrite(digiPin[9], LOW);         // motorček OFF
        swMotor = 0;
        taktCount[3] = 0;  
    
    }    
}  
// --- konec mix() ------------------------------------------------------- 

unsigned long obtok(uint8_t obt) {    
    switch (obt){
        case 1: 
            digitalWrite(digiPin[11], HIGH);   // obtočna črpalka ON  
            swObtok = 1;         
        break;
        default:
            digitalWrite(digiPin[11], LOW);    // obtočna črpalka OFF  
            swObtok = 0;
    }         
}  
// --- konec obtok() ------------------------------------------------------- 
// *** konec dodatne rutine regulacije ****************************************************

// *** prikaz na display ******************************************************************

unsigned long izpisTextdisplay() {
   tft.fillScreen(BLACK);
   unsigned long start = micros();
   tft.setCursor(0, 0);
   tft.setTextColor(YELLOW);  tft.setTextSize(2);
   tft.println(" TEMPERATURE PREZRACEVANJA");
   tft.setTextColor(WHITE);  tft.setTextSize(1);
   tft.println();
   narisiKvadrat(25, 25, 68, 15, YELLOW, 0);
   tft.setTextSize(2);
   tft.setCursor(110, 25);
   tft.print("MINI  TEMP  MAXI");
   // obtočna črpalka
   narisiKrog(160, 212, 21, RED, 0);
   narisiKrog(160, 212, 20, RED, 0);
   narisiKrog(160, 212, 19, RED, 0);
   narisiKrog(160, 212, 4, YELLOW, 1);

     // zaporni ventil
   izpisZaporniVentil();  
       
  tft.println();  
  return micros() - start;
}
// --- konec izpisTextdisplay()-----------------------------------

unsigned long izpisVrednostidisplay() {
   // prikaz temperatur na display
   narisiKvadrat(0, 40, 320, 148, BLACK, 1);
   narisiKvadrat(26, 26, 64, 13, BLACK, 1);
   for (int k = 0; k < (countSensor); k++)  {
      if (tempValue[k] == 99.9) {                                                     
          digitalWrite(digiPin[k], HIGH);
          narisiKrog(tCol[0], pRow[k], 5, GREEN, 0);
      }
      else {        
          digitalWrite(digiPin[k], LOW);             
          narisiKrog(tCol[0], pRow[k], 5, GREEN, 1);
      }
      tft.setTextColor(WHITE); 
      tft.setCursor(tCol[1], tRow[k]);
      for (int n = 0;  n < countSensor; n++) {
           tft.print(textAA[k][n]);
      }
      tft.setCursor(tCol[2], tRow[k]);
      poravnavaDecimalk(MIN[k]);
      tft.setTextColor(GREEN);   
      tft.print(MIN[k],1);
      izpisTrenutneTemp(tempValue[k], k);
      tft.setCursor(tCol[4], tRow[k]);
      poravnavaDecimalk(MAX[k]);
      tft.setTextColor(RED);
      tft.print(MAX[k],1);
      
      // trend gibanja temperatur
      if (tempValue[k] < trendMin[k][0]){
          if (trendMin[k][0] < trendMin[k][1]){
              narisiCrto(tCol[6], tRow[k]+2, tCol[6], tRow[k]+15, WHITE);
              narisiCrto(tCol[6], tRow[k]+2, tCol[5], tRow[k]+5, WHITE);
              narisiCrto(tCol[6], tRow[k]+2, tCol[7], tRow[k]+5, WHITE);   
              trendMin[k][1] = trendMin[k][0]; 
          }
          trendMin[k][0] = tempValue[k];
      } 
      if (tempValue[k] < trendMax[k][0]) {
          if (trendMax[k][0] < trendMax[k][1]) {
              narisiCrto(tCol[6], tRow[k]+15, tCol[6], tRow[k]+2, WHITE);
              narisiCrto(tCol[6], tRow[k]+15, tCol[5], tRow[k]+10, WHITE);
              narisiCrto(tCol[6], tRow[k]+15, tCol[7], tRow[k]+10, WHITE);
              trendMax[k][1] = trendMax[k][0]; 
          }
          trendMax[k][0] = tempValue[k];
      } 
      switch (k){
          case 0: 
              narisiCrto(0, 67, 320, 67, WHITE);
          break;
          case 4:
              narisiCrto(0, 147, 320, 147, WHITE);
          break;
       }
   }
}
// --- konec izpisVrednostidisplay()-----------------------------------

unsigned long izpisTrenutneTemp(float value, uint8_t k) {
     tft.setCursor(tCol[3], tRow[k]);
      if (k == 2)
          tft.setTextColor(CYAN); 
      else    
          tft.setTextColor(YELLOW);
      poravnavaDecimalk(value);                
      tft.print(value,1);
}      
// --- konec izpisTrenutneTemp() ----------------------------------------      
      
unsigned long poravnavaDecimalk(float param) {
  if (param > -9.9) {
        tft.print(" ");}
  if (param < 10 and param > -0.1)
        tft.print(" ");
}
// --- konec poravnavaDecimalk() ---------------------------------

unsigned long narisiCrto(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    tft.drawLine(x1, y1, x2, y2, color);
}
// --- konec narisiCrto() ----------------------------------------

unsigned long narisiKvadrat(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color, uint8_t fill) {
  if(fill == 1)
     tft.fillRect(x, y, w, h, color);
  else   
     tft.drawRect(x, y, w, h, color);
}
// --- konec narisiKvadrat() ----------------------------------------

unsigned long narisiKrog(uint8_t x, uint8_t y, uint8_t radius, uint16_t color, uint8_t fill) {
  if(fill == 1)
     tft.fillCircle(x, y, radius, color);
  else 
     tft.drawCircle(x, y, radius, color);
}
// --- konec narisiKrog()-----------------------------------------

unsigned long narisiTrikotnik(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,  uint16_t x3, uint16_t y3, uint16_t color, uint8_t fill) {
  if(fill == 1)
    tft.fillTriangle(x1, y1, x2, y2, x3, y3, color);
  else 
    tft.drawTriangle(x1, y1, x2, y2, x3, y3, color);
}
// --- konec narisiTrikotnik() ----------------------------------------

unsigned long prikazCrpalka() {
    swVrten++; 
    switch (swVrten){
        case 1: 
            narisiKrog(160, 212, 18, BLACK, 1);
            narisiCrto(144, 211, 176, 211, CYAN);
            narisiCrto(143, 212, 177, 212, CYAN);
            narisiCrto(144, 213, 176, 213, CYAN);
            narisiCrto(159, 194, 159, 228, CYAN);
            narisiCrto(160, 195, 160, 229, CYAN);
            narisiCrto(159, 194, 159, 228, CYAN);
            narisiKrog(160, 212, 4, YELLOW, 1);
        break;
        case 2: 
            narisiKrog(160, 212, 18, BLACK, 1);
            narisiCrto(149, 200, 172, 223, CYAN);
            narisiCrto(148, 200, 172, 224, CYAN);
            narisiCrto(148, 201, 171, 224, CYAN);
            narisiCrto(148, 223, 171, 200, CYAN);
            narisiCrto(148, 224, 172, 200, CYAN);
            narisiCrto(149, 224, 172, 201, CYAN);
            narisiKrog(160, 212, 4, YELLOW, 1);
        break;
        case 3: 
            narisiKrog(160, 212, 18, BLACK, 1);
            narisiCrto(159, 194, 159, 228, CYAN);
            narisiCrto(160, 195, 160, 229, CYAN);
            narisiCrto(159, 194, 159, 228, CYAN);           
            narisiCrto(144, 211, 176, 211, CYAN);
            narisiCrto(143, 212, 177, 212, CYAN);
            narisiCrto(144, 213, 176, 213, CYAN);
            narisiKrog(160, 212, 4, YELLOW, 1);
        break;
        case 4: 
            narisiKrog(160, 212, 18, BLACK, 1);
            narisiCrto(148, 223, 171, 200, CYAN);
            narisiCrto(148, 224, 172, 200, CYAN);
            narisiCrto(149, 224, 172, 201, CYAN);           
            narisiCrto(149, 200, 172, 223, CYAN);
            narisiCrto(148, 200, 172, 224, CYAN);
            narisiCrto(148, 201, 171, 224, CYAN);
            narisiKrog(160, 212, 4, YELLOW, 1);
            swVrten = 0; 
        break;
    }
}        
// --- konec prikazCrpalka() --------------------------------------------------------

unsigned long prikazVentil(uint8_t swVentil) {
    if (swVentil != sVentil) {
        narisiTrikotnik(0, 227, 128, 197, 127, 227, BLACK, 1);
        narisiTrikotnik(0, 227, 66, 197, 130, 227, BLACK, 1);
        narisiTrikotnik(0, 227, 88, 197, 130, 227, BLACK, 1);
        narisiTrikotnik(0, 227, 48, 197, 130, 227, BLACK, 1);
        narisiTrikotnik(0, 227, 128, 197, 127, 227, BLACK, 1);
        sVentil = swVentil;
    }    
    switch (swVentil){
        case 1:
            narisiTrikotnik(5, 225, 125, 225, 125, 198, RED, 1);
            narisiTrikotnik(0, 227, 128, 198, 127, 227, YELLOW, 0);
            izpisGrejeMotor();     
        break;
        case 2:
            narisiTrikotnik(5, 225, 65, 225, 65, 200, RED, 1);             
            narisiTrikotnik(66, 226, 66, 201, 125, 226, BLUE, 1);
            narisiTrikotnik(0, 227, 66, 198, 130, 227, YELLOW, 0);
            narisiTrikotnik(5, 222, 5, 192, 75, 192, BLACK, 1);
            narisiTrikotnik(128, 222, 128, 190, 55, 190, BLACK, 1);            
        break;
        case 3:
            narisiTrikotnik(5, 225, 85, 225, 85, 200, RED, 1);             
            narisiTrikotnik(86, 225, 86, 200, 125, 225, BLUE, 1);
            narisiTrikotnik(0, 227, 88, 198, 130, 227, YELLOW, 0); 
            izpisGrejeMotor();   
        break;
        case 4:
            narisiTrikotnik(5, 225, 45, 225, 45, 200, RED, 1);             
            narisiTrikotnik(46, 225, 46, 200, 125, 225, BLUE, 1);
            narisiTrikotnik(0, 227, 48, 198, 130, 227, YELLOW, 0); 
            izpisHladiMotor();             
        break;
        case 5: 
            narisiTrikotnik(5, 225, 125, 225, 0, 198, BLUE, 1);
            narisiTrikotnik(0, 227, 128, 198, 127, 227, YELLOW, 0);
            izpisHladiMotor();
        break;
    }
}        
// --- konec prikazVentil() --------------------------------------------------------


unsigned long izpisGrejeMotor() {
            switch(taktCount[3]) {
                case 1: 
                    narisiKrog(10, 215, 3, CYAN, 1);
                break;
                case 2: 
                    narisiKrog(20, 211, 3, CYAN, 1);
                break;
                case 3: 
                    narisiKrog(30, 207, 3, CYAN, 1);
                break;
                case 4: 
                    narisiKrog(40, 203, 3, CYAN, 1);
                break;
                case 5: 
                    narisiKrog(50, 199, 3, CYAN, 1);

                break;
                default:
                    narisiTrikotnik(5, 222, 5, 192, 75, 192, BLACK, 1);
            }
}    
// --- konec izpisGrejeMotor() --------------------------------------------------------

unsigned long izpisHladiMotor() {
            switch(taktCount[3]) {
                case 1: 
                    narisiKrog(125, 215, 3, CYAN, 1);
                break;
                case 2: 
                    narisiKrog(115, 211, 3, CYAN, 1);
                break;
                case 3: 
                    narisiKrog(105, 207, 3, CYAN, 1);
                break;
                case 4: 
                    narisiKrog(95, 203, 3, CYAN, 1);
                break;
                case 5: 
                    narisiKrog(85, 199, 3, CYAN, 1);

                break;
                default:
                    narisiTrikotnik(128, 222, 128, 190, 55, 190, BLACK, 1);
              } 
}              
// --- konec izpisHladiMotor() --------------------------------------------------------

unsigned long izpisZaporniVentil() {
    narisiKvadrat(202, 213, 15, 2, YELLOW, 1);
    narisiKvadrat(202, 228, 15, 2, YELLOW, 1);
    narisiKvadrat(243, 213, 17, 2, YELLOW, 1);
    narisiKvadrat(243, 228, 17, 2, YELLOW, 1);
    narisiKvadrat(222, 201, 17, 5, YELLOW, 1);
    narisiKvadrat(226, 197, 6, 4, YELLOW, 1);
    narisiKvadrat(227, 194, 21, 3, YELLOW, 1);
    narisiKrog(230, 220, 15, YELLOW, 0);
    narisiKrog(230, 220, 16, YELLOW, 0);
    narisiKrog(230, 220, 12, CYAN, 1);
    narisiKvadrat(202, 215, 76, 13, BLACK, 1);
}
// --- konec izpisZaporniVentil() ------------------------------------------------------

unsigned long izpisZaporniVentilOdprt() {
    taktCount[2]++;
    switch(taktCount[2]) {
                case 1: 
                    narisiKrog(206, 221, 4, RED, 1);
                    narisiKrog(254, 221, 4, BLACK, 1);
                break;
                case 2: 
                    narisiKrog(218, 221, 4, RED, 1);
                break;
                case 3: 
                    narisiKrog(230, 221, 4, RED, 1);
                break;
                case 4: 
                    narisiKrog(242, 221, 4, RED, 1);
                break;
                case 5: 
                    narisiKrog(254, 221, 4, RED, 1);
                    narisiKrog(206, 221, 4, BLACK, 1);
                break;
                case 6: 
                    narisiKrog(218, 221, 4, BLACK, 1);                 
                break;
                case 7: 
                    narisiKrog(230, 221, 4, BLACK, 1);
                break;
                case 8: 
                    narisiKrog(242, 221, 4, BLACK, 1);
                    taktCount[2]= 0;                 
                break;                
              } 
}
// --- konec izpisZaporniVentil --------------------------------------------------------
// *** konec izpis na display *************************************************************

// *** prikaz na monitor računalnika ******************************************************
unsigned long izpisMonitor() {
    if (index == 0) {
        Serial.print("\n");
        Serial.println("     A0      A1      A2      A3      A4      A5      A6       Mix ventil  ");
    } 
    for (int k = 0; k < countSensor; k++) {
         if (k == 0) {
             Serial.print(index+1);
             Serial.print(".  ");
         }
         Serial.print(sensorValue[k]);
         Serial.print("   "); 
    } 
    switch (swMix) {
        case 1:
            Serial.print("  zima ALARM");
        break;
        case 2:
            Serial.print("  zima MIRUJE");
        break;
        case 3:
            Serial.print("  zima GREJE");
        break;
        case 4:
            Serial.print("  zima HLADI");        
        break;
        case 5:
            Serial.print("  poletje PRVI");        
        break;
        case 6:
            Serial.print("  poletje HLADI");
        break;
        case 7:
            Serial.print("  poletje MIRUJE");
        break;
     }
     if (swMotor == 1)
         Serial.print("  + MIX");
     if (swObtok == 1)
         Serial.print("  + OBTOK");    
     Serial.print("  [");
     Serial.print(taktCount[0]);
     Serial.print(" ");
     Serial.print(taktValue[3]);
     Serial.print(" ");
     Serial.print("] ");
     Serial.print(swMix);
     Serial.print(" ");
     Serial.print(swMotor);
     Serial.print(" ");     
     Serial.print("\n");
}
// --- konec izpisMonitor() --------------------------------------------------------

unsigned long izpisPovprecje60() {
    Serial.print("           voda  zunaj   A.izm   rekup    B.izm  dile    box    ");
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
}
// --- konec izpisPovprecje60() --------------------------------------------------- 
// *** konec prikaz na monitor računalnika ************************************************

    
      
  
