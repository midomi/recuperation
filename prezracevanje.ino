// V 3.0.

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

DeviceAddress sensor0 = { 0x28, 0xC6, 0x7A, 0x78, 0x06, 0x00, 0x00, 0x86 };
DeviceAddress sensor1 = { 0x28, 0x8E, 0x6D, 0x6E, 0x06, 0x00, 0x00, 0x4E };
DeviceAddress sensor2 = { 0x28, 0x81, 0x2D, 0x70, 0x06, 0x00, 0x00, 0xAE };
DeviceAddress sensor3 = { 0x28, 0xA9, 0x60, 0x70, 0x06, 0x00, 0x00, 0xBC };
DeviceAddress sensor4 = { 0x28, 0x39, 0xE1, 0x6E, 0x06, 0x00, 0x00, 0x26 };
DeviceAddress sensor5 = { 0x28, 0x23, 0x05, 0x78, 0x06, 0x00, 0x00, 0x46 };
DeviceAddress sensor6 = { 0x10, 0x15, 0x44, 0x77, 0x02, 0x08, 0x00, 0xAA };
DeviceAddress sensor7 = { 0x28, 0x23, 0x05, 0x78, 0x06, 0x00, 0x00, 0x77 };
// ***************************   nastavljive spremenljivke  ***********xx***
         // vrednosti pogojev delovanja toplotnih izmenjevalnikov A in B 
float minA = 12.0;       // meja zima/poletje na I. izmenjevalniku
float minB = 12.0;       // meja zima/poletje na II. izmenjevalniku 
float maxB = 23.0;       // meja hlajenje poleti
float minVoda = 3.0;     // meja ALARM - rekuperator OFF 
float sensorMax = 50.0;  // meja ALARM - rekuperator OFF (sensor ne deluje)

// int startValue = 210;    // postavitev mešalnega ventila na pozicijo START
// ************konec *********   nastavljive spremenljivke  **************
                         // korekcijski faktorji
float sensorKore[] = {0.33, -0.53, 0.26, 0.40, -0.59, 0.27, 0.00, 0.00}; // tipal

float time = 0;          // čas odpiranja/zapiranja ventila (razlika med staro in novo pozicijo ventila)
float tempMix = 0;       // temperatura na katero se dela mix 
int impulz = 0;          // impulz motorčka
int startVal = 0;        // preračunan vrednost START pozicije
float difer;             // odstopanje od tempMix   
float poziValue[] = {0, 0, 0};       // pozicija mešalnega ventila
                         // 0. preračunana vrednost pozicije mešalnega ventila
                         // 1. stara vrednost pozicije mešalnega ventila
                         // 2. procent odprtosti mešalnega ventila 
int iRow = 133;          // izračun vrstice prikaza delovanja mešalnega ventila

int taktCount[]  = {0, 0, 0, 0, 0, 0};   // števci delovanja 
                         // 0. števec takta 
                         // 1. zaporedno število prebranih za monitor
                         // 2. števec gretja 
                         // 3. števec mirovanja
                         // 4. števec hlajenja
int taktCountView[] = {0, 0, 0, 0, 0}; // števci prikaza (greje, miruje, hladi) 

int countRota[] = {0, 0}; // števec pretoka vode
                          // 0. I. izmenjevalnika
                          // 1. II. izmenjevalnika
int swStat[] = {0, 0};    // status delovanja
                          // 0. I. izmenjevalnika (1-alarm, 2-zima greje, 3-zima, 4-zima hladi
                          //                       5-poleti hladi, 6-poleti prvič, 7-poleti miruje)
                          // 1. II. izmenjevalnika (1-greje, 2-hladi, 3-baypass)
int swStatOld[] = {9, 9}; // status - prejšnji
                          // 0. status prejšnji - določa preskok med enim in drugim režimom
                          // 1. status predprejšnji - določa smer prikaza mirovanja   
int swObtok = 0;          // svič delovanja obtočne črpalke
                          // 0. zima 
                          // 1. poletje
int swPrvic[] = {1, 1,};  // svič prvič v tem režimu
                          // 0. zima (1-ON, 0-OFF)
                          // 1. poletje 

float sensorValue[8];     // vrednost tipal
float sensorValueOld[8];  // vrednost predhodne temp. tipal za izpis na display
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
//                  {"voda ", "zunaj", "A.izm", "rekup", "B.izm", "TČ   ", "box  ", "dile   "};
int tCol[] =        {  126,     126,      266,    266,     266,    126,     280,     280};  // stolpci vrednosti 
int tRow[] =        {  125,      56,       56,    126,     196,    170,     160,      95};  // vrstica vrednosti

int countSensor = 8;     // število tipal 
int digiPin[] = {13, 12, 47, 48, 49, 50, 51};  // output digital pini (LED, Beeper, rele)

                         // 0. 13 LED D8 Arduino   
                         // 1. 12 alarm (Beeper)
                         // 2. 47 rele mešalni ventil ON/OFF
                         // 3. 48 mešalni ventil (delovni-odpira, mirovni-zaprira)
                         // 4. 49 obtočna črpalka ON/OFF
                         // 5. 50 zaporni ventil ON/OFF  
                         // 6. 51 rekuperator OFF/ON (POZOR delovni-OFF, mirovni-ON) 
int countPin = 7;       // število output pinov


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
     sensors.setResolution(sensor0, 11);
     sensors.setResolution(sensor1, 11);
     sensors.setResolution(sensor2, 11);
     sensors.setResolution(sensor3, 11);
     sensors.setResolution(sensor4, 11);
     sensors.setResolution(sensor5, 11);
     sensors.setResolution(sensor6, 11);
     sensors.setResolution(sensor7, 11); 
     


  // nastavitve display-a
     tft.reset();
     uint16_t identifier = tft.readID();
     tft.begin(identifier);
     uint8_t rotation=1;
     tft.setRotation(rotation);
     tft.fillScreen(BLACK);
     izpisTextdisplay();     // prikaz glave na display
//     startVal = startValue * 1000;
}

// *** inicializacija vrednosti ************************************
void initVrednost() {
  for (int k = 0; k < countSensor; k++)  {
    sensorValue[k] = 0;
  }
}

// *** branje vrednosti tipal **************************************
void beriSensor() {
  // beri vrednost 
    sensors.requestTemperatures();
    sensorValue[0] = sensors.getTempC(sensor0);
    sensorValue[1] = sensors.getTempC(sensor1);
    sensorValue[2] = sensors.getTempC(sensor2);
    sensorValue[3] = sensors.getTempC(sensor3);
    sensorValue[4] = sensors.getTempC(sensor4);
    sensorValue[5] = sensors.getTempC(sensor5);
    sensorValue[6] = sensors.getTempC(sensor6);
    sensorValue[7] = sensors.getTempC(sensor7);
    for (int k = 0; k < countSensor; k++)  {
         sensorValue[k] = sensorValue[k] + sensorKore[k];
    }     
      // izračun variabilne temp. VODE                   
    tempMix = 14.4 - (0.46 * sensorValue[1]);     // temp var
    difer = tempMix - sensorValue[0];             // diferenca var  
    if (difer < 0)                             
        difer = difer * -1;  
}

//*****************************************************************************************
// *** glavna zanka ***********************************************************************
void loop() {
    beriSensor();                               // sensor vrednosti
    taktCount[0]++;                                // takt preverjanja spremembe temp. 
   
    // A.izmenjevalnik ............................VAROVALKA ..................................

         if (sensorValue[0] < minVoda or sensorValue[0] > sensorMax) { // voda < 3°C   ALARM 
                 digitalWrite(digiPin[6], HIGH);                     // rekuperator OFF
                 digitalWrite(digiPin[1], HIGH);                     // ALARM piskač ON
                 digitalWrite(digiPin[3], HIGH);                     // dodaja TOPLO vodo
                 digitalWrite(digiPin[2], HIGH);                     // mixaj
                 obtok(1);                                           // obtočna črpalka ON
                 swStat[0] = 1;                                      // status delovanja
                 poziValue[0] = poziValue[0] + 1;                    // pozicija mešalnega ventila
         }
         else {              // ........................ZIMA........................................
             if (sensorValue[1] < minA) {
                 if (swPrvic[0] == 1) {
                     digitalWrite(digiPin[6], LOW);              // rekuperator ON
                     digitalWrite(digiPin[1], LOW);              // ALARM  piskač OFF
                     swPrvic[0] = 0;                         
                     swPrvic[1] = 1;                          
                     prvic();                                    // Start pozimi postavi ventil na sredino
                     obtok(1);                                   // obtočna črpalka ON
                 }  
                 else {
                     if (difer > 1) {
                         tempMix = 14.4 - (0.46 * sensorValue[1]);           // temp var
                         poziValue[0] = round(((tempMix - sensorValue[7]) / (sensorValue[6] - sensorValue[7])) * 210); 
                     }
                     korekcija;
                 }
             }
             else {          //.........................POLETJE ....................................
                 if (swPrvic[1] == 1) {
                     if (sensorValue[1] > (minA+0.5)) {            // Prvič                        . 
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
           digitalWrite(digiPin[5], HIGH);     // ogrevanje
           swStat[1] = 1;
       }
       else {
           if (sensorValue[1] > maxB) {
               digitalWrite(digiPin[5], HIGH);  // hlajenje
               swStat[1] = 2;
           }   
           else { 
               digitalWrite(digiPin[5], LOW);   // baypass
               swStat[1] = 3;
           }   
       }
    // konec B.izmenjevalnik ................................    

    // trenutne vrednosti temperature zraka 
       prikazValue(); 
       
    // obtočna črpalka
       if (swObtok == 1)
           prikazCrpalka(swObtok);
    
    // zaporni ventil
       prikazZaporniVentil();
       
    // izpis trenutnih vrednosti na monitor - Serial.print
       izpisMonitor();
       
       if (taktCount[0] == 1) {
            if (swStat[0] == 3) {
                if (swStatOld[0] != 3) 
                    swStatOld[1] = swStatOld[0];
            }   
            swStatOld[0] = swStat[0];
       }
        
    taktCount[1]++;
    if (taktCount[1] > 5000)
        taktCount[1] = 1;
    initVrednost();              // inicializacija vrednosti spremenljivk

  // turn the ledPin on
  digitalWrite(digiPin[0], HIGH);  
  // stop the program for <sensorValue> milliseconds:
  delay(500);          
  // turn the ledPin off:        
  digitalWrite(digiPin[0], LOW);   
  // stop the program for for <sensorValue> milliseconds:
  delay(500);   
}

// *** konec glavne zanke *****************************************************************

// *** dodatne rutine regulacije **********************************************************
unsigned long prvic() {

    tft.setCursor(99, 20);
    tft.setTextColor(MAGENTA);  
    tft.setTextSize(1);
    
    tft.print("START pozicija (210 sekund...)"); 
    digitalWrite(digiPin[3], HIGH);    // mešalni ventil v celoti ODPREMO (start pozicija)
    digitalWrite(digiPin[2], HIGH);     // motorček ON
    delay(210000);
    digitalWrite(digiPin[2], LOW);             // motorček OFF 
    narisiKvadrat(99, 20, 220, 20, BLACK, 1);
    poziValue[1] = 210;

    if (swPrvic[0] == 0) {
        narisiKvadrat(137, 219, 70, 10, BLACK, 1); 
        narisiKrog(162, 224, 12, CYAN, 1); 
        narisiKvadrat(158, 212, 9, 24, BLACK, 1); 
    }  
    else {
        narisiKrog(162, 224, 12, CYAN, 1); 
        narisiKvadrat(132, 219, 85, 10, BLACK, 1);
    }

    for (int k = 0; k < countSensor; k++)  {
         taktCount[k] = 0;
    }    
}
// --- konec prvic() ---------------------------------------------------------------
unsigned long korekcija() {

 // korekcija mešalnega ventila pozimi 
    if (poziValue[0] == poziValue[1]) {
        swStat[0] = 3;                             // status delovanja
    }
    else {
        if (poziValue[0] > poziValue[1]) {                // korekcija ON 
            digitalWrite(digiPin[3], HIGH);                // dodaja TOPLO vodo
            impulz = poziValue[0] - poziValue[1];
            time = impulz * 1000;
            swStat[0] = 2;
        }
        else {
            digitalWrite(digiPin[3], LOW);      // dodaja HLADNO vodo pozimi
            impulz = (poziValue[1] - poziValue[0]);
            time = impulz * 1000;
            swStat[0] = 4;                        // status delovanja 
        }
        digitalWrite(digiPin[2], HIGH);           // motorček ON
        delay(time);
        taktCount[0] = 1;
    }
    digitalWrite(digiPin[2], LOW);             // motorček OFF 
    
    if (taktCount[0] == 1) {              // prikazVentil na display
        if (swStat[0] != swStatOld[0]) {
            taktCount[swStat[0]] = 0;
            taktCountView[swStat[0]] = 0; 
        }            
        if (swStat[0] == 3) {
            taktCount[swStat[0]] = taktCount[swStat[0]] + 1;
            taktCountView[swStat[0]] =  taktCountView[swStat[0]] + 1;     
            if (taktCount[swStat[0]] > 5) {
                taktCount[swStat[0]] = taktCount[swStat[0]] - 5;
            }
        }
        else {
            taktCount[swStat[0]] = taktCount[swStat[0]] + impulz;
            taktCountView[swStat[0]] =  taktCountView[swStat[0]] + impulz;     
            if (taktCount[swStat[0]] > 10) {
                taktCount[swStat[0]] = taktCount[swStat[0]] - 10;
            }
        }          
        prikazVentil();
    }    
}
// --- konec korekcija() ---------------------------------------------------------------

unsigned long obtok(uint8_t obt) {    
    switch (obt){
        case 1: 
            digitalWrite(digiPin[4], HIGH);   // obtočna črpalka ON 
            swObtok = 1; 
        break;
        default:
            digitalWrite(digiPin[4], LOW);    // obtočna črpalka OFF 
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
   narisiKvadrat(202, 40, 50, 50, WHITE, 0);
   narisiKvadrat(197, 55, 5, 20, WHITE, 0);
   narisiKvadrat(252, 55, 5, 20, WHITE, 0);
   
   // rekuperator
   narisiKvadrat(197, 100, 60, 70, CYAN, 0);
   narisiKvadrat(198, 101, 58, 68, CYAN, 0);
   narisiKvadrat(203, 96, 8, 4, CYAN, 0);
   narisiKvadrat(215, 96, 8, 4, CYAN, 0);
   narisiKvadrat(230, 96, 8, 4, CYAN, 0);
   narisiKvadrat(244, 96, 8, 4, CYAN, 0);
   narisiCrto(227, 110, 212, 125, CYAN);
   narisiCrto(227, 110, 242, 125, CYAN);
   narisiCrto(212, 125, 212, 145, CYAN);
   narisiCrto(242, 125, 242, 145, CYAN);
   narisiCrto(212, 145, 227, 160, CYAN);
   narisiCrto(242, 145, 227, 160, CYAN);
   
   // II. izmenjevalnik
   narisiKvadrat(202, 180, 50, 50, WHITE, 0);
   narisiKvadrat(197, 195, 5, 20, WHITE, 0);
   narisiKvadrat(252, 195, 5, 20, WHITE, 0);

   // mešalni ventil
   narisiKvadrat(28, 26, 64, 214, YELLOW, 0);
   narisiKvadrat(29, 27, 62, 212, YELLOW, 0);
   narisiKvadrat(30, 28, 18, 210, RED, 1);
   narisiKvadrat(70, 29, 18, 9, BLUE, 1); 
   narisiCrto(23, 80, 28, 80, YELLOW);   
   narisiCrto(23, 186, 28, 186, YELLOW);

   // obtočna črpalka
   narisiKrog(162, 100, 18, RED, 0);
   narisiKrog(162, 100, 19, RED, 0);
   narisiKrog(162, 100, 20, RED, 0);
   narisiKvadrat(137, 79, 70, 10, BLACK, 1);
   narisiKvadrat(137, 77, 52, 2, RED, 1);
   narisiKvadrat(137, 89, 15, 2, RED, 1);
   narisiKvadrat(176, 89, 15, 2, RED, 1);
   narisiCrto(189, 89, 202, 89, WHITE);
   narisiCrto(189, 78, 202, 78, WHITE);
   narisiKvadrat(147, 88, 29, 3, BLACK, 1);
   prikazVeter(0);
  
     // zaporni ventil
   narisiCrto(137, 229, 202, 229, WHITE); 
   narisiCrto(137, 218, 202, 218, WHITE);
   narisiKrog(162, 224, 15, WHITE, 0);
   narisiKrog(162, 224, 14, WHITE, 0); 
   narisiKvadrat(155, 205, 15, 5, WHITE, 1);   
   narisiKvadrat(159, 200, 7, 4, RED, 1);  
   narisiKvadrat(165, 197, 17, 3, RED, 1);
   narisiTrikotnik(165, 197, 162, 200, 165, 200, RED, 1);   
       
  tft.println();  
  return micros() - start;
}
// --- konec izpisTextdisplay()-----------------------------------

unsigned long prikazValue() {
   // mešalni ventil
      if (taktCount[0] > 0) {
          if (taktCount[0] == 1) {
              narisiKvadrat(49, 28, 3, 210, BLACK, 1);
              narisiKvadrat(67, 28, 3, 210, BLACK, 1);
              for (int k = 22; k < 90; k=k+7) {
                   narisiCrto(k, 133, k+4, 133, YELLOW);
              }      
              narisiCrto(30, iRow, 88, iRow, GREEN);
          }    
          int row = 241 - (taktCount[0] * 10);
          narisiKvadrat(49 ,row , 3, 8, CYAN, 1);
          narisiKvadrat(67 ,row , 3, 8, CYAN, 1);
          
      }
 
   // I. izmenjevalnik
      
      for (int k = 44; k < 89; k=k+5) {
           if (swStat[0] == 5) { 
               narisiKvadrat(219, k, 16, 3, CYAN, 1);
           }    
           else {
             if (swStat[0] < 5) 
                 narisiKvadrat(219, k, 16, 3, RED, 1);
             else 
                 narisiKvadrat(219, k, 16, 3, BLUE, 1);    
           }        
      }
      
   // II. izmenjevalnik
      for (int k = 184; k < 228; k=k+5) {
           switch (swStat[1]) {
                case 1: narisiKvadrat(219, k, 16, 3, RED, 1); break;
                case 2: narisiKvadrat(219, k, 16, 3, BLUE, 1); break;
                case 3: narisiKvadrat(219, k, 16, 3, CYAN, 1); break;
           }    
      } 

   // vrednosti temperatur
      tft.setTextColor(YELLOW); 
      for (int k = 0; k < (countSensor); k++)  { 
          if (sensorValue[k] != sensorValueOld[k]) {
              sensorValueOld[k] = sensorValue[k]; 
              narisiKvadrat(tCol[k], tRow[k], 50, 20, BLACK, 1); 
              tft.setCursor(tCol[k], tRow[k]);
              if (k == 0 or k == 5) tft.setTextColor(CYAN);
              else tft.setTextColor(YELLOW);            
              if (k < 6) tft.setTextSize(2);
                 else tft.setTextSize(1);
              if (sensorValue[k] < 10 and sensorValue[k] > -0.1)
                  tft.print(" ");   
              tft.print(sensorValue[k],1);
          }
/*
          if (sensorValue[k] > sensorMax) {                                                     
              digitalWrite(digiPin[k], HIGH);
              narisiKrog((tCol[k]-10), (tRow[k]-7), 5, GREEN, 0);
          }
          else {        
              digitalWrite(digiPin[k], LOW);             
              narisiKrog((tCol[k]-10), (tRow[k]-7), 5, GREEN, 1);
          }    
*/
      }
}
// --- konec prikazValue()-----------------------------------

unsigned long prikazZaporniVentil() {
        narisiKrog(162, 224, 12, CYAN, 1); 
        narisiKvadrat(132, 219, 85, 10, BLACK, 1);
        countRota[1]++;   
        switch (countRota[1]) {
            case 1:
                narisiKrog(138, 224, 3, RED, 1);        
                narisiKrog(150, 224, 3, RED, 1);
                narisiKrog(162, 224, 3, RED, 1);
            break;
            case 2:
                narisiKrog(150, 224, 3, RED, 1);
                narisiKrog(162, 224, 3, RED, 1);
                narisiKrog(174, 224, 3, RED, 1);
            break;
            case 3:
                narisiKrog(162, 224, 3, RED, 1);
                narisiKrog(174, 224, 3, RED, 1);
                narisiKrog(186, 224, 3, RED, 1);
            break;         
            case 4:
                narisiKrog(174, 224, 3, RED, 1);
                narisiKrog(186, 224, 3, RED, 1);
                narisiKrog(198, 224, 3, RED, 1);
            break;         
            case 5:
                narisiKrog(186, 224, 3, RED, 1);
                narisiKrog(198, 224, 3, RED, 1); 
                narisiKrog(210, 224, 3, RED, 1);
                countRota[1] = 0; 
            break;
        }
}        
// --- konec prikazZaporniVentil() --------------------------------------------------------

unsigned long prikazCrpalka(uint8_t swObtok) {
    narisiKrog(162, 100, 18, BLACK, 1);
    narisiKvadrat(134, 79, 82, 10, BLACK, 1);
    prikazVeter(0);
    if (swObtok == 1) { 
        countRota[0]++; 
        switch (countRota[0]) {
            case 1:
                narisiKrog(138, 83, 3, RED, 1);
                narisiKrog(150, 83, 3, RED, 1);
                narisiKrog(162, 83, 3, RED, 1);      
                prikazVeter(1);
            break;
            case 2:
                narisiKrog(162, 83, 3, RED, 1); 
                narisiKrog(174, 83, 3, RED, 1);
                narisiKrog(186, 83, 3, RED, 1);
                prikazVeter(3);
            break; 
            case 3:
                narisiKrog(174, 83, 3, RED, 1);
                narisiKrog(186, 83, 3, RED, 1);
                narisiKrog(198, 83, 3, RED, 1);                 
                prikazVeter(2);
            break;         
            case 4:
                narisiKrog(186, 83, 3, RED, 1);
                narisiKrog(198, 83, 3, RED, 1);
                narisiKrog(210, 83, 3, RED, 1); 
                prikazVeter(4);
                countRota[0] = 0; 
            break;
        }
    }
    narisiKrog(162, 100, 3, YELLOW, 1);

}        
// --- konec prikazCrpalka() --------------------------------------------------------

unsigned long prikazVeter(uint8_t swVeter) {
    switch(swVeter) {
        case 0: 
            narisiCrto(146, 99, 178, 99, CYAN);
            narisiCrto(145, 100, 179, 100, CYAN);
            narisiCrto(146, 101, 178, 101, CYAN);
            narisiCrto(161, 84, 161, 116, CYAN);
            narisiCrto(162, 83, 162, 117, CYAN);
            narisiCrto(163, 84, 163, 116, CYAN);
            narisiCrto(151, 88, 174, 111, CYAN);
            narisiCrto(150, 88, 174, 112, CYAN);
            narisiCrto(150, 89, 173, 112, CYAN);
            narisiCrto(150, 111, 173, 88, CYAN);
            narisiCrto(150, 112, 174, 88, CYAN);
            narisiCrto(151, 112, 174, 89, CYAN);            
        break;

        case 1: 
            narisiCrto(146, 99, 178, 99, BLUE);
            narisiCrto(145, 100, 179, 100, BLUE);
            narisiCrto(146, 101, 178, 101, BLUE);
        break;
        case 2: 
            narisiCrto(161, 84, 161, 116, BLUE);
            narisiCrto(162, 83, 162, 117, BLUE);
            narisiCrto(163, 84, 163, 116, BLUE);
        break;
        case 3: 
            narisiCrto(151, 88, 174, 111, BLUE);
            narisiCrto(150, 88, 174, 112, BLUE);
            narisiCrto(150, 89, 173, 112, BLUE);
        break;
        case 4: 
            narisiCrto(150, 111, 173, 88, BLUE);
            narisiCrto(150, 112, 174, 88, BLUE);
            narisiCrto(151, 112, 174, 89, BLUE);  
        break;
    }
}
// --- konec prikazVeter() --------------------------------------------------------

unsigned long prikazVentil() {
    narisiCrto(30, iRow, 86, iRow, BLACK);
    switch (swStat[0]) { 
        case 2:                  // prikaz impulzov GREJE
            narisiKvadrat(5, 28, 15, 212, BLACK, 1);
            for (int k = 0; k < taktCount[2]; k++) {
                 narisiKrog(10, (231-(15*k)), 4, RED, 1);
                 narisiKrog(10, (231-(15*k)), 5, WHITE, 0);   
            }
        break;
        case 3:                  // prikaz impulzov MIRUJE
           narisiKvadrat(53, 28, 15, 210, BLACK, 1);   
            for (int k = 0; k < taktCount[3]; k++) {
                 if (swStat[0] > swStatOld[1]) iRow = 141-(15*k);
                 if (swStat[0] < swStatOld[1]) iRow = 125+(15*k);  
                 narisiKrog(59, iRow, 4, GREEN, 1);
                 narisiKrog(59, iRow, 5, WHITE, 0); 
            }
        break;    
        case 4:                 // prikaz impulzov HLADI  
            narisiKvadrat(97, 28, 15, 210, BLACK, 1);
            for (int k = 0; k < taktCount[4]; k++) {
                 narisiKrog(104, (33+(15*k)+1), 4, BLUE, 1); 
                 narisiKrog(104, (33+(15*k)), 5, WHITE, 0);   
            }
        break;    
    } 
    if (swStat[0] != swStatOld[0]) {  
        switch (swStatOld[0]) { 
            case 2:                  // prikaz impulzov GREJE
                 for (int k = 0; k < taktCount[2]; k++) {
                     narisiKrog(10, (231-(15*k)), 4, BLACK, 1);     
                }
            break;
            case 3:                  // prikaz impulzov MIRUJE
                for (int k = 0; k < taktCount[3]; k++) {
                     if (swStat[0] > swStatOld[1]) iRow = 141-(15*k);
                     if (swStat[0] < swStatOld[1]) iRow = 125+(15*k);  
                     narisiKrog(59, iRow, 5, BLACK, 1); 
                }  
            break;    
            case 4:                 // prikaz impulzov HLADI  
                 for (int k = 0; k < taktCount[4]; k++) {
                     narisiKrog(104, (33+(15*k)), 4, BLACK, 1); 
                }
            break;    
        }   
    }

    // pozicija mešalnega ventila na display-u
    tft.setTextColor(WHITE);  
    tft.setTextSize(1);
    iRow = 27 + (poziValue[0]);
    narisiKvadrat(30, 28, 19, 210, BLACK, 1); 
    if (swStat[0] < 5)
        narisiKvadrat(30, iRow, 19, (210-poziValue[0]), RED, 1);
    else
        narisiKvadrat(30, iRow, 19, (210-poziValue[0]), BLUE, 1);    
    narisiKvadrat(69, 28, 20, 210, BLACK, 1);
    narisiKvadrat(69, 28, 19, poziValue[0], BLUE, 1); 
    narisiCrto(30, iRow, 88, iRow, GREEN);
    
    narisiKvadrat(95, 180, 50, 30, BLACK, 1); 
    poziValue[3] = ((210-poziValue[0])*100)/210;
    tft.setCursor(35, 210);
    tft.print(taktCountView[2]);
    if (swStat[0] > 3) {
         tft.setCursor(71, (iRow+4));
         tft.print(poziValue[3],0);
         tft.print("%");
         narisiKvadrat(53, 60, 15, 10, BLACK, 1); 
         narisiKvadrat(53, 210, 15, 10, BLACK, 1);     
         tft.setCursor(55, 210);
    }     
    if (swStat[0] < 3) {
         tft.setCursor(32, (iRow-12));
         tft.print(poziValue[3],0);
         tft.print("%");
         narisiKvadrat(53, 60, 15, 10, BLACK, 1); 
         narisiKvadrat(53, 210, 15, 10, BLACK, 1);        
         tft.setCursor(55, 60);
    }        
    tft.print(taktCountView[3]);
    tft.setCursor(74, 60);
    tft.print(taktCountView[4]);
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
    if (taktCount[1] == 1) {
        Serial.println("      voda   zunaj  I.izm  rekup  II.izm TC     box   ");
    } 
    for (int k = 0; k < countSensor; k++) {
         if (k == 0) {
             if (taktCount[1] < 1000) { 
                 Serial.print(" ");
                 if (taktCount[1] < 100) {
                     Serial.print(" ");
                     if (taktCount[1] < 10)
                         Serial.print(" ");
                 }
             }    
             Serial.print(taktCount[1]);
             Serial.print("  "); 
         }
         Serial.print(sensorValue[k]);
         Serial.print("  "); 
    } 
    switch (swStat[0]) {
        case 1:
            Serial.print("zima ALARM ");
        break;
        case 2:
            Serial.print("zima GREJE ");
        break;
        case 3:
            Serial.print("zima MIRUJE ");
        break;
        case 4:
            Serial.print("zima HLADI ");        
        break;
        case 5:
            Serial.print("poletje PRVI ");        
        break;
        case 6:
            Serial.print("poletje HLADI ");
        break;
        case 7:
            Serial.print("poletje MIRUJE ");
        break;
     }
     if (impulz > 0)
         Serial.print("+M");
     if (swObtok == 1)
         Serial.print("+C");    
     Serial.print(" [");
     Serial.print(taktCount[0]);
     Serial.print(" ");
     Serial.print(impulz);
     Serial.print("] ");
     Serial.print(swStat[0]);
     Serial.print("=");
     Serial.print(swStatOld[0]);
     Serial.print("=>");
     Serial.print(swStatOld[1]);
     Serial.print(" ");
     Serial.print(sensorValueOld[2]);
     Serial.print(" ");
     Serial.print(tempMix);  
     Serial.print(" ");
     Serial.print(difer); 
     Serial.print("=");
     Serial.print("\n");
}
// --- konec izpisMonitor() --------------------------------------------------------
// *** konec prikaz na monitor računalnika ************************************************
