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

// ***************************   nastavljive spremenljivke  ***********xx***
         // vrednosti pogojev delovanja toplotnih izmenjevalnikov A in B 
float minA = 12.0;       // meja zima/poletje na I. izmenjevalniku
float minB = 12.0;       // meja zima/poletje na II. izmenjevalniku 
float maxB = 23.0;       // meja hlajenje poleti
float minVoda = 3.0;     // meja ALARM - rekuperator OFF 
float sensorMax = 50.0;    // meja ALARM - rekuperator OFF (sensor ne deluje)
float tempZrak = 10.0;   // izhodiščna temperatura zraka za mešalni ventil
int taktValue[] = {20, 0,};      // takt delovanja pozimi
                       // 0. takt umirjanja senzorjev
                       // 1. impulz motorčka
int startValue = 20;      // postavitev mešalnega ventila na pozicijo START
int startVal = 0;      // postavitev mešalnega ventila na pozicijo START

// ************konec *********   nastavljive spremenljivke  **************

float difer[7];          // razlika med izhodiščno temp. (tempZrak) in trenutno temp. senzorja   
float diferOld = 0;      // prejšnja vrednost razlike me izhodiščem (tempZrak) in trenutno temp. senzorja
int poziValue = 0;       // pozicija mešalnega ventila
int iRow = 0;            // izračun vrstice prikaza delovanja mešalnega ventila
int zap[7];              // število zaporedno prebranih enakih temp. po senzorjih (3 krat)
int taktCount[]  = {0, 0, 0, 0, 0};   // števci delovanja 
                         // 0. števec takta 
                         // 1. zaporedno število prebranih za monitor
                         // 2. števec gretja 
                         // 3. števec mirovanja
                         // 4. števec hlajenja
int taktCountOld[] = {0, 0, 0, 0, 0}; // števci delovanja - prejšnji 

         // tipla {   A0,    A1,    A2,    A3,    A4}         
float korek[] =   {-0.50, -0.50, -0.50, -0.50, -1.50}; // korekcijski faktor analognih tipal
int uporValue[] = { 1049,  1049,  1049,  1049,  1049}; // pretvornik iz Ohm v °C 

int countRota[] = {0, 0}; // števec pretoka vode
                          // 0. I. izmenjevalnika
                          // 1. II. izmenjevalnika
int swStat[] = {0, 0};    // status delovanja
                          // 0. I. izmenjevalnika (1-alarm, 2-zima greje, 3-zima, 4-zima hladi
                          //                       5-poleti hladi, 6-poleti prvič, 7-poleti miruje)
                          // 1. II. izmenjevalnika (1-greje, 2-hladi, 3-baypass)
int swStatOld = 0;        // status - prejšnji   
int swObtok = 0;          // svič delovanja obtočne črpalke
                          // 0. zima 
                          // 1. poletje
int swPrvic[] = {1, 1,};  // svič prvič v tem režimu
                          // 0. zima (1-ON, 0-OFF)
                          // 1. poletje 

float sensorValue[7];     // vrednost tipal
float sensorValueOld[7];  // vrednost predhodne temp. tipal
float sensorValueOldI[7]; // vrednost predhodne temp. tipal za izpis trenutne temp.
float ohmValue[7];        // preračunana vrednost v Ohm
float param = 0.0;        // parameter za prenost vrednosti za poravnavo decimalk
//****************************************************************************************
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

// pozicije vrednosti temperatur za prikaz na display
char textAA[7][7] = {"voda ", "zunaj", "A.izm", "rekup", "B.izm", "dile ", "box  "};
int tCol[] =        {    0,      40,      180,    180,     180,        0,      0};  // stolpci vrednosti 
int tRow[] =        {  100,      56,       56,    126,     196,      140,    170};  // vrstica vrednosti
int sensorPin[] = {A8, A9, A10, A11, A12};   // input pini analognih tipal
                         // A8 tipalo A0   mix voda za I. izmenevalnik
                         // A9 tipalo A1   zunanji zrak
                         // A10 tipalo A2  po I. izmenjevalniku
                         // A11 tipalo A3  po rekuperaciji
                         // A12 tipalo A4  po II. izmenjevalniku
int countSensor = 7;     // število tipal (5 analog. + 2 digit.)
int digiPin[] = {32, 34, 36, 38, 40, 42, 44, 13, 12, 47, 48, 49, 50, 51};  // output digital pini (LED, Beeper, rele)

                         // 0. 32 LED D0 kontrolka tipala A0
                         // 1. 34 LED D1 kontrolka tipala A1
                         // 2. 36 LED D2 kontrolka tipala A2
                         // 3. 38 LED D3 kontrolka tipala A3
                         // 4. 40 LED D4 kontrolka tipala A4
                         // 5. 42 LED D5 kontrolka tipala D9
                         // 6. 44 LED D6 kontrolka tipala D10
                         // 7. 13 LED D8 Arduino   
                         // 8. 12 alarm (Beeper)
                         // 9. 47 rele mešalni ventil ON/OFF
                         // 10. 48 mešalni ventil (delovni-odpira, mirovni-zaprira)
                         // 11. 49 obtočna črpalka ON/OFF
                         // 12. 50 zaporni ventil ON/OFF  
                         // 13. 51 rekuperator OFF/ON (POZOR delovni-OFF, mirovni-ON) 
int countPin = 14;       // število output pinov


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

  // nastavitve display-a
     tft.reset();
     uint16_t identifier = tft.readID();
     tft.begin(identifier);
     uint8_t rotation=1;
     tft.setRotation(rotation);
     tft.fillScreen(BLACK);
     izpisTextdisplay();     // prikaz glave na display
     startVal = startValue * 1000;
}

// *** inicializacija vrednosti ************************************
void initVrednost() {
  for (int k = 0; k < countSensor; k++)  {
    sensorValue[k] = 0;
    ohmValue[k] = 0;
  }
}

// *** branje vrednosti tipal **************************************
void beriSensor() {
  // beri vrednost analognih tipal
    for (int k = 0; k < (countSensor - 2); k++)  {
       sensorValue[k] = analogRead(sensorPin[k]);
       ohmValue[k] = (float)uporValue[k] / ((1023.0 / (float)sensorValue[k]) - 1.0); // preračun v Ohm
       sensorValue[k] = ((ohmValue[k] - 1000.0) / 3.9) + korek[k];                   // preračun v °C 
       if (sensorValue[k] == sensorValueOld[k]) {            //
           zap[k]++;                                 //                                   
           if (zap[k] > 3) {                         //                            
               zap[k] = 3;                           //
           }                                         // 3 x zaporedoma prebrana ista temp.
       }                                             //
       else {                                        //  
           zap[k] = 1;                               //
       }                                             //    
       sensorValueOld[k] = sensorValue[k];                   //
       
       difer[k] = tempZrak - sensorValue[k];               // izračun odstopanja temperatur od 
       if (difer[k] < 0)                                  // od izhodiščne temp. za rekuperacijo
           difer[k] = difer[k] * -1;                      //    
  }

  // beri vrednost digitalnih tipal
  sensors.requestTemperatures();
  sensorValue[5] = sensors.getTempCByIndex(0);
  sensorValue[6] = sensors.getTempCByIndex(1); 
}

//*****************************************************************************************
// *** glavna zanka ***********************************************************************
void loop() {
    beriSensor();                               // sensor vrednosti
    taktCount[0]++;                                // takt preverjanja spremembe temp. 
    if (taktCount[0] > taktValue[0])               //
        taktCount[0] = 1;                       // začetna vrednost takta
   
    // A.izmenjevalnik ............................VAROVALKA ..................................

         if (sensorValue[0] < minVoda or sensorValue[0] > sensorMax) { // voda < 3°C   ALARM 
             if (zap[0] == 3) {
                 digitalWrite(digiPin[13], HIGH);                    // rekuperator OFF
                 digitalWrite(digiPin[8], HIGH);                     // ALARM piskač
                 digitalWrite(digiPin[10], HIGH);                    // dodaja TOPLO vodo
                 digitalWrite(digiPin[9], HIGH);                     // mixaj
                 obtok(1);                                           // obtočna črpalka ON
                 swStat[0] = 1;                                      // status delovanja
                 poziValue = poziValue + 1;                          // pozicija mešalnega ventila
                 poziValue++;                                        // števec GREJE             
             }
         }
         else {              // ........................ZIMA........................................                                                   //                           .      
             if (sensorValue[1] < minA) {
               if (swPrvic[0] == 1) {                                // Prvič
                     if (zap[1] == 3) {                              //
                         if (zap[2] == 3) {
                             digitalWrite(digiPin[13], LOW);         // rekuperator ON
                             digitalWrite(digiPin[8], LOW); 
                             swPrvic[0] = 0;                         //
                             swPrvic[1] = 1;                           // postavi ventil v začetno pozicijo
                             prvic();                                  // pozimi ZAPRE, poleti ODPRE
                             obtok(1);                                 // obtočna črpalka ON
                         }                                               
                     }
                 }
                 else {
                     if (difer[2] > 1) {
                         if (difer[2] == diferOld) {
                             if (taktCount[0] == 1) {               // na začetku takta
                                 taktValue[1] = 1;                  // postavim impulz
                           /*      if (difer[2] > 2) 
                                     taktCount[1] = 2;
                                 if (difer[2] > 3)
                                     taktCount[1] = 3;    
                          */   }
                         } 
                         else {    
                             if (zap[2] == 3) {
                                 if (difer[2] > diferOld) {          // če diferenca narašča   
                                     diferOld = difer[2];            // 
                                     taktCount[0] = 1;              // postavim takt 
                                     taktValue[1] = 1;              // postavim impulz 
                                 /*    if (difer[2] > 2) 
                                         taktCount[1] = 2;
                                     if (difer[2] > 3)
                                         taktCount[1] = 3;    
                              */   }
                                 else {
                                     diferOld = difer[2];
                                 }
                             }    
                         }
                     }
                     else {
                         taktCount[1] = 0;                         // postavim impulz   
                     }
                     korekcija();                                 // korekcija temp
                 }                 
             }
             else {          //.........................POLETJE ....................................
                 if (swPrvic[1] == 1) {
                     if  (zap[1] == 3) {                           // Prvič                             . 
                          swPrvic[1] = 0;                          //                              .
                          swPrvic[0] = 1;                          //                              .
                          swStat[0] = 6;                           //                              .
                          obtok(0);                                // obtočna črpalka OFF          .
                          prvic();                                 // 
                     }                                             //  
                 }
                 if (sensorValue[1] > maxB) {                      //                              .
                     obtok(1);                                     // obtočna črpalka ON           .  
                     swStat[0] = 5;                                //                              .
                 }                                                 //                              .     
                 else {                                            //                              .
                     obtok(0);                                     // obtočna črpalka OFF          .  
                     swStat[0] = 7;                                //                              .
                 }                                                 //                              .                                                    //                              .
             }                                                     //                              .
         }
    // konec A.izmenjevalnik ..................................................................  

    // B. izmenjevalnik ......................................
       if (sensorValue[1] < minB){                            
           digitalWrite(digiPin[12], HIGH);     // ogrevanje
           swStat[1] = 1;
       }
       else {
           if (sensorValue[1] > maxB) {
               digitalWrite(digiPin[12], HIGH);  // hlajenje
               swStat[1] = 2;
           }   
           else { 
               digitalWrite(digiPin[12], LOW);   // baypass
               swStat[1] = 3;
           }   
       }
    // konec B.izmenjevalnik ................................    

    // trenutne vrednosti temperature zraka 
       prikazValue(); 
       
    // obtočna črpalka
       if (swObtok == 1)
           prikazCrpalka(swObtok);
    
    // mešalni ventil
       if (swStatOld != swStat[0]) {
           switch (swStat[0]) { 
               case 2:
                   taktCount[2] = 0;
               break;
               case 3:
                   taktCount[3] = 0;
               break;                   
               case 4:
                   taktCount[4] = 0;
               break;     
               default:
                   taktCount[2] = 0;     
                   taktCount[3] = 0;
                   taktCount[4] = 0;
           }                 
       }            
       prikazVentil();
       if (swStatOld != swStat[0]) 
           swStatOld = swStat[0];     
    // zaporni ventil
       prikazZaporniVentil();
       
    // izpis trenutnih vrednosti na monitor - Serial.print
       izpisMonitor(); 
    
    taktCount[1]++;
    if (taktCount[1] > 5000)
        taktCount[1] = 1;
    initVrednost();              // inicializacija vrednosti spremenljivk

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
unsigned long prvic() {

    tft.setCursor(0, 20);
    tft.setTextColor(MAGENTA);  
    tft.setTextSize(1);
    
    tft.print("START pozicija (210 sekund...)"); 
    digitalWrite(digiPin[10], HIGH);    // mešalni ventil v celoti ODPREMO (start pozicija)
    digitalWrite(digiPin[9], HIGH);     // motorček ON
    delay(startVal);

    if (swPrvic[0] == 0) {
        narisiKvadrat(0, 20, 240, 20, BLACK, 1);  
        tft.setCursor(0, 20);
        tft.print("ZIMA pozicija (105 sekund...)");    
        digitalWrite(digiPin[10], LOW); // pozimi postavimo mešalni ventil na POLOVICO
        delay(105000);
        poziValue = 105;                             // trenutna pozicija delovanja motorčka
        swStat[0] = 2;                               // status delovanja ZIMA 
        narisiKvadrat(55, 219, 70, 10, BLACK, 1); 
        narisiKrog(80, 224, 12, CYAN, 1); 
        narisiKvadrat(76, 212, 9, 24, BLACK, 1); 
    }  
    else {
        swStat[0] = 6;                             // status delovanja POLETJE
        narisiKrog(80, 224, 12, CYAN, 1); 
        narisiKvadrat(50, 219, 85, 10, BLACK, 1);
    }
            
    digitalWrite(digiPin[9], LOW);             // motorček OFF 
    narisiKvadrat(0, 20, 240, 20, BLACK, 1);
    for (int k = 0; k < countSensor; k++)  {
         taktCount[k] = 0;
    }    
}
// --- konec prvic() ---------------------------------------------------------------

unsigned long korekcija() {
 // korekcija mešalnega ventila vsakič na začetku takta-zima 
    if (taktCount[0] <= taktValue[1]) {   // korekcija ON 
        if (sensorValue[2] < tempZrak) {
            digitalWrite(digiPin[10], HIGH);      // dodaja TOPLO vodo pozimi
            poziValue = poziValue - 1;            // pozicija mešalnega ventila 
            taktCount[2]++;                       // števec GREJE 
            if (taktCount[2] > 17) {
//                narisiKvadrat(230, 28, 8, 210, BLACK, 1);
                taktCount[2] = 1;
                swStat[0] = 2;                 // status delovanja
            }
        }
        else {
            digitalWrite(digiPin[10], LOW);       // dodaja HLADNO vodo
            poziValue = poziValue + 1;            // pozicija mešalnega ventila
            taktCount[4]++;                       // števec HLADI
            if (taktCount[4] > 17) {
                narisiKvadrat(312, 28, 8, 210, BLACK, 1);
                taktCount[4] = 1;
                swStat[0] = 4;                   // status delovanja
            }
        }     
        digitalWrite(digiPin[9], HIGH);          // motorček ON
    }
    else {                                // korekcija OFF
       digitalWrite(digiPin[9], LOW);             // motorček OFF 
       taktCount[3]++;                            // števec HLADI
       if (taktCount[3] > 8) {
           narisiKvadrat(272, 28, 8, 201, BLACK, 1); 
           taktCount[3] = 1;       
           swStat[0] = 3;                        // status delovanja
       }
    }
}
// --- konec korekcija() ---------------------------------------------------------------

unsigned long obtok(uint8_t obt) {    
    switch (obt){
        case 1: 
            digitalWrite(digiPin[11], HIGH);   // obtočna črpalka ON 
            swObtok = 1; 
        break;
        default:
            digitalWrite(digiPin[11], LOW);    // obtočna črpalka OFF 
            swObtok = 0;   
            prikazCrpalka(0);
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
   tft.println(" REGULACIJA  PREZRACEVANJA");
   
   // I. izmenjevalnik
   narisiKvadrat(120, 40, 50, 50, WHITE, 0);
   narisiKvadrat(115, 55, 5, 20, WHITE, 0);
   narisiKvadrat(170, 55, 5, 20, WHITE, 0);
   
   // rekuperator
   narisiKvadrat(115, 100, 60, 70, CYAN, 0);
   narisiKvadrat(116, 101, 58, 68, CYAN, 0);
   narisiKvadrat(121, 96, 8, 4, CYAN, 0);
   narisiKvadrat(133, 96, 8, 4, CYAN, 0);
   narisiKvadrat(148, 96, 8, 4, CYAN, 0);
   narisiKvadrat(162, 96, 8, 4, CYAN, 0);
   narisiCrto(145, 110, 130, 125, CYAN);
   narisiCrto(145, 110, 160, 125, CYAN);
   narisiCrto(130, 125, 130, 145, CYAN);
   narisiCrto(160, 125, 160, 145, CYAN);
   narisiCrto(130, 145, 145, 160, CYAN);
   narisiCrto(160, 145, 145, 160, CYAN);
   
   // II. izmenjevalnik
   narisiKvadrat(120, 180, 50, 50, WHITE, 0);
   narisiKvadrat(115, 195, 5, 20, WHITE, 0);
   narisiKvadrat(170, 195, 5, 20, WHITE, 0);

   // mešalni ventil
   narisiKvadrat(246, 26, 58, 214, YELLOW, 0);
   narisiKvadrat(247, 27, 56, 212, YELLOW, 0);
   narisiKvadrat(249, 28, 18, 210, RED, 1);
   narisiKvadrat(281, 28, 18, 210, BLUE, 1); 
   narisiCrto(241, 80, 245, 80, YELLOW);   
   narisiCrto(241, 133, 245, 133, YELLOW);
   narisiCrto(241, 186, 245, 186, YELLOW);

   // obtočna črpalka
   narisiKrog(80, 100, 18, RED, 0);
   narisiKrog(80, 100, 19, RED, 0);
   narisiKrog(80, 100, 20, RED, 0);
   narisiKvadrat(55, 79, 70, 10, BLACK, 1);
   narisiKvadrat(55, 77, 52, 2, RED, 1);
   narisiKvadrat(55, 89, 15, 2, RED, 1);
   narisiKvadrat(94, 89, 15, 2, RED, 1);
   narisiCrto(107, 89, 120, 89, WHITE);
   narisiCrto(107, 78, 120, 78, WHITE);
   narisiKvadrat(65, 88, 29, 3, BLACK, 1);
   prikazVeter(0);
  
     // zaporni ventil
   narisiCrto(55, 229, 120, 229, WHITE); 
   narisiCrto(55, 218, 120, 218, WHITE);
   narisiKrog(80, 224, 15, WHITE, 0);
   narisiKrog(80, 224, 14, WHITE, 0); 
   narisiKvadrat(73, 205, 15, 5, WHITE, 1);   
   narisiKvadrat(77, 200, 7, 4, RED, 1);  
   narisiKvadrat(60, 197, 20, 3, RED, 1);
   narisiTrikotnik(80, 197, 80, 200, 83, 200, RED, 1);   
       
  tft.println();  
  return micros() - start;
}
// --- konec izpisTextdisplay()-----------------------------------

unsigned long prikazValue() {
   // mešalni ventil
      if (taktCount[0] > 0) {
          if (taktCount[0] == 1)
              narisiKvadrat(267, 28, 3, 210, BLACK, 1);
              narisiKvadrat(278, 28, 3, 210, BLACK, 1);
          int row = 241 - (taktCount[0] * 10);
          narisiKvadrat(267 ,row , 3, 8, CYAN, 1);
          narisiKvadrat(278 ,row , 3, 8, CYAN, 1);
          
      }
      
   // I. izmenjevalnik
      
      for (int k = 44; k < 89; k=k+5) {
           if (swStat[0] == 5) { 
               narisiKvadrat(137, k, 16, 3, CYAN, 1);
           }    
           else {
             if (swStat[0] < 5) 
                 narisiKvadrat(137, k, 16, 3, RED, 1);
             else 
                 narisiKvadrat(137, k, 16, 3, BLUE, 1);    
           }        
      }
      
   // II. izmenjevalnik
      for (int k = 184; k < 228; k=k+5) {
           switch (swStat[1]) {
                case 1: narisiKvadrat(137, k, 16, 3, RED, 1); break;
                case 2: narisiKvadrat(137, k, 16, 3, BLUE, 1); break;
                case 3: narisiKvadrat(137, k, 16, 3, CYAN, 1); break;
           }    
      } 

   // vrednosti temperatur
      tft.setTextColor(YELLOW); 
      for (int k = 0; k < (countSensor); k++)  { 
          if (sensorValue[k] != sensorValueOldI[k]) {
              sensorValueOldI[k] = sensorValue[k]; 
              narisiKvadrat(tCol[k], tRow[k], 50, 20, BLACK, 1); 
              tft.setCursor(tCol[k], tRow[k]);
              poravnavaDecimalk(sensorValue[k]);
              if (k == 0) tft.setTextColor(CYAN);
              else tft.setTextColor(YELLOW);            
              if (k < 5) tft.setTextSize(2);
                 else tft.setTextSize(1);
              tft.print(sensorValue[k],1);
          }
          if (sensorValue[k] > sensorMax) {                                                     
              digitalWrite(digiPin[k], HIGH);
              narisiKrog((tCol[k]+7), (tRow[k]-7), 5, GREEN, 0);
          }
          else {        
              digitalWrite(digiPin[k], LOW);             
              narisiKrog((tCol[k]+7), (tRow[k]-7), 5, GREEN, 0);
          }    
      }
}
// --- konec prikazValue()-----------------------------------

unsigned long poravnavaDecimalk(float param) {
//  if (param > -9.9) {
//      tft.print(" ");}
  if (param < 10 and param > -0.1)
        tft.print(" ");
}
// --- konec poravnavaDecimalk() ---------------------------------

unsigned long prikazZaporniVentil() {
        narisiKrog(80, 224, 12, CYAN, 1); 
        narisiKvadrat(50, 219, 85, 10, BLACK, 1);
        countRota[1]++;   
        switch (countRota[1]) {
            case 1:
                narisiKrog(56, 224, 3, RED, 1);        
                narisiKrog(68, 224, 3, RED, 1);
                narisiKrog(80, 224, 3, RED, 1);
            break;
            case 2:
                narisiKrog(68, 224, 3, RED, 1);
                narisiKrog(80, 224, 3, RED, 1);
                narisiKrog(92, 224, 3, RED, 1);
            break;
            case 3:
                narisiKrog(80, 224, 3, RED, 1);
                narisiKrog(92, 224, 3, RED, 1);
                narisiKrog(104, 224, 3, RED, 1);
            break;         
            case 4:
                narisiKrog(92, 224, 3, RED, 1);
                narisiKrog(104, 224, 3, RED, 1);
                narisiKrog(116, 224, 3, RED, 1);
            break;         
            case 5:
                narisiKrog(104, 224, 3, RED, 1);
                narisiKrog(116, 224, 3, RED, 1); 
                narisiKrog(128, 224, 3, RED, 1);
                countRota[1] = 0; 
            break;
        }
}        
// --- konec prikazZaporniVentil() --------------------------------------------------------

unsigned long prikazCrpalka(uint8_t swObtok) {
    narisiKrog(80, 100, 18, BLACK, 1);
    narisiKvadrat(50, 79, 82, 10, BLACK, 1);
    prikazVeter(0);
    if (swObtok == 1) { 
        countRota[0]++; 
        switch (countRota[0]) {
            case 1:
                narisiKrog(56, 83, 3, RED, 1);
                narisiKrog(68, 83, 3, RED, 1);
                narisiKrog(80, 83, 3, RED, 1);      
                prikazVeter(1);
            break;
            case 2:
                narisiKrog(56, 83, 3, RED, 1);
                narisiKrog(68, 83, 3, RED, 1);
                narisiKrog(92, 83, 3, RED, 1);
                prikazVeter(2);
            break; 
            case 3:
                narisiKrog(68, 83, 3, RED, 1); 
                narisiKrog(80, 83, 3, RED, 1); 
                narisiKrog(92, 83, 3, RED, 1);
                prikazVeter(3);
            break;         
            case 4:
                narisiKrog(92, 83, 3, RED, 1);
                narisiKrog(104, 83, 3, RED, 1);
                narisiKrog(116, 83, 3, RED, 1); 
                prikazVeter(4);
            break; 
            case 5:
                narisiKrog(104, 83, 3, RED, 1);
                narisiKrog(116, 83, 3, RED, 1);  
                narisiKrog(128, 83, 3, RED, 1);
                prikazVeter(1); 
            break; 
            case 6:
                narisiKrog(116, 83, 3, RED, 1);  
                narisiKrog(128, 83, 3, RED, 1);
                prikazVeter(2);                    
                countRota[0] = 0; 
            break;
        }
    }
    narisiKrog(80, 100, 3, YELLOW, 1);

}        
// --- konec prikazCrpalka() --------------------------------------------------------

unsigned long prikazVeter(uint8_t swVeter) {
    switch(swVeter) {
        case 0: 
            narisiCrto(64, 99, 96, 99, CYAN);
            narisiCrto(63, 100, 97, 100, CYAN);
            narisiCrto(64, 101, 96, 101, CYAN);
            narisiCrto(79, 84, 79, 116, CYAN);
            narisiCrto(80, 83, 80, 117, CYAN);
            narisiCrto(81, 84, 81, 116, CYAN);
            narisiCrto(69, 88, 92, 111, CYAN);
            narisiCrto(68, 88, 92, 112, CYAN);
            narisiCrto(68, 89, 91, 112, CYAN);
            narisiCrto(68, 111, 91, 88, CYAN);
            narisiCrto(68, 112, 92, 88, CYAN);
            narisiCrto(69, 112, 92, 89, CYAN);            
        break;

        case 1: 
            narisiCrto(64, 99, 96, 99, BLUE);
            narisiCrto(63, 100, 97, 100, BLUE);
            narisiCrto(64, 101, 96, 101, BLUE);
        break;
        case 2: 
            narisiCrto(79, 84, 79, 116, BLUE);
            narisiCrto(80, 83, 80, 117, BLUE);
            narisiCrto(81, 84, 81, 116, BLUE);
        break;
        case 3: 
            narisiCrto(69, 88, 92, 111, BLUE);
            narisiCrto(68, 88, 92, 112, BLUE);
            narisiCrto(68, 89, 91, 112, BLUE);
        break;
        case 4: 
            narisiCrto(68, 111, 91, 88, BLUE);
            narisiCrto(68, 112, 92, 88, BLUE);
            narisiCrto(69, 112, 92, 89, BLUE);  
        break;
    }
}
// --- konec prikazVeter() --------------------------------------------------------

unsigned long prikazVentil() {
    // prikaz impulzov GREJE 
    if (taktCountOld[2] != taktCount[2]) { 
        narisiKvadrat(227, 28, 12, 212, BLACK, 1);
        for (int k = 1; k < taktCount[2]; k++) {
             narisiKrog(233, (246-(12*k)), 4, YELLOW, 0);     
             if (swStat[0] == 2) 
                 narisiKrog(233, (246-(12*k)), 3, RED, 1);  
        }
    } 
    // prikaz impulzov MIRUJE
    if (taktCountOld[3] != taktCount[3]) { 
        narisiKvadrat(260, 28, 4, 210, BLACK, 1);
        for (int k = 1; k < taktCount[3]; k++) {
              if (swStatOld > swStat[0]) iRow = 125+(12*k);
             if (swStatOld < swStat[0]) iRow = 141-(12*k);             
             narisiKrog(260, iRow, 4, YELLOW, 0);     
             if (swStat[0] == 3) 
                 narisiKrog(260, iRow, 3, GREEN, 1);
        }
    }    
    // prikaz impulzov HLADI
    if (taktCountOld[4] != taktCount[4]) { 
        narisiKvadrat(306, 28, 12, 210, BLACK, 1);
        for (int k = 1; k < taktCount[4]; k++) {
              narisiKvadrat(306, (20+(12*k)), 4, 4, YELLOW, 0);     
             if (swStat[0] == 4) 
                 narisiKvadrat(307, (20+(12*k)+1), 2, 2, BLUE, 1);  
        }
    }   

    // pozicija mešalnega ventila na display-u
    iRow = 27 + (poziValue);
    narisiKvadrat(248, 28, 19, 210, BLACK, 1); 
    narisiKvadrat(248, iRow, 19, (210-poziValue), RED, 1);
    narisiKvadrat(281, 28, 19, 210, BLACK, 1);
    narisiKvadrat(281, 28, 19, poziValue, BLUE, 1); 
    narisiCrto(248, iRow, 302, iRow, GREEN);
}        
// --- konec prikazVentil() --------------------------------------------------------

// ***********************************************************************************************
// ***********************************************************************************************

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
// *** konec izpis na display *************************************************************

// *** prikaz na monitor računalnika ******************************************************
unsigned long izpisMonitor() {
    if (taktCount[0] == 1) {
        Serial.print("\n");
        Serial.println("     A0      A1      A2      A3      A4      A5      A6       Mix ventil  ");
    } 
    for (int k = 0; k < countSensor; k++) {
         if (k == 0) {
             if (taktCount[1] < 10) Serial.print("   ");
             if (taktCount[1] < 100) Serial.print("  ");
             if (taktCount[1] < 1000) Serial.print(" ");
             Serial.print(taktCount[1]);
             Serial.print(".  ");
         }
         Serial.print(sensorValue[k]);
         Serial.print("   "); 
    } 
    switch (swStat[0]) {
        case 1:
            Serial.print("  zima ALARM");
        break;
        case 2:
            Serial.print("  zima GREJE");
        break;
        case 3:
            Serial.print("  zima MIRUJE");
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
     if (taktCount[2] > 0)
         Serial.print("  + MIX");
     if (swObtok == 1)
         Serial.print("  + OBTOK");    
     Serial.print("  [");
     Serial.print(taktCount[0]);
     Serial.print(" ");
     Serial.print(taktValue[1]);
     Serial.print(" ");
     Serial.print("] ");
     Serial.print(swStat[0]);
     Serial.print("  (");
     Serial.print(zap[1]);
     Serial.print(" "); 
     Serial.print(zap[2]);          
     Serial.print(" ) "); 
     Serial.print(taktCount[2]);
     Serial.print(" ");
     Serial.print(taktCount[3]);
     Serial.print(" ");
     Serial.print(taktCount[4]);
     Serial.print("\n");
}
// --- konec izpisMonitor() --------------------------------------------------------
// *** konec prikaz na monitor računalnika ************************************************
