// V 3.2.

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
DeviceAddress sensor6 = { 0x28, 0xD7, 0x64, 0x6F, 0x06, 0x00, 0x00, 0x8A };
DeviceAddress sensor7 = { 0x10, 0x15, 0x44, 0x77, 0x02, 0x08, 0x00, 0xAA };
DeviceAddress sensor8 = { 0x10, 0x9C, 0x49, 0x77, 0x02, 0x08, 0x00, 0x3A };
// ***************************   nastavljive spremenljivke  ***********xx***
         // vrednosti pogojev delovanja toplotnih izmenjevalnikov A in B 
float minA = 12.0;       // meja zima/poletje na I. izmenjevalniku
float minB = 12.5;       // meja zima/poletje na II. izmenjevalniku 
float maxB = 23.0;       // meja hlajenje poleti
float minVoda = 3.0;     // meja ALARM - rekuperator OFF 
float sensorMax = 50.0;  // meja ALARM - rekuperator OFF (sensor ne deluje)
float tempKore = 1.0;    // korekcija želene temperature C mix-vode
int poziMax = 5;         // maximalna vrednost spremembe pozicije za 1 izračun
// ************konec *********   nastavljive spremenljivke  **************
                         // korekcijski faktorji
float sensorKore[] = {0.33, -0.53, 0.26, 0.40, -0.59, 0.27, 0.00, 0.00, 0.00}; // tipal

float tempMix = 0;       // temperatura na katero se dela mix 
float razlika = 0;       // temperatura na katero se dela mix 
int impulz = 0;          // impulz motorčka
int impulzOld = 0;       // impulz motorčka - prejšnji
int startVal = 0;        // preračunan vrednost START pozicije
float difer[] = {0, 0};  // odstopanje temp vode od tempMix  
                         // 0. trenutna vrednost
                         // 1. stara vrednost 
int poziValue[] = {0, 0, 0};       // pozicija mešalnega ventila
                         // 0. preračunana vrednost pozicije mešalnega ventila
                         // 1. stara vrednost pozicije mešalnega ventila
                         // 2. procent odprtosti mešalnega ventila 
int iRow = 133;          // izračun vrstice prikaza delovanja mešalnega ventila
int taktValue  = 0;      // števec mirovanja pri visoki temp. povratne vode
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

float sensorValue[9];     // vrednost tipal
float sensorValueOld[9];  // vrednost predhodne temp. tipal za izpis na display
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
//                  {"voda ", "zunaj", "A.izm", "rekup", "B.izm", "TČ   ", "pov  ", "box,   "dile"};
int tCol[] =        {  126,     126,      266,    266,     266,    126,     126,     280,     280};  // stolpci vrednosti 
int tRow[] =        {  125,      56,       56,    126,     196,    175,     150,     160,      95};  // vrstica vrednosti

int countSensor = 9;     // število tipal 
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
     sensors.setResolution(sensor8, 11);  

  // nastavitve display-a
     tft.reset();
     uint16_t identifier = tft.readID();
     tft.begin(identifier);
     uint8_t rotation=1;
     tft.setRotation(rotation);
     tft.fillScreen(BLACK);
     izpisTextdisplay();
}
// --- konec SETUP()-----------------------------------

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
    sensorValue[8] = sensors.getTempC(sensor8);
    for (int k = 0; k < countSensor; k++)  {
         sensorValue[k] = sensorValue[k] + sensorKore[k];
    }     
      // izračun variabilne temp. VODE       
    tempMix = (14.7305 - (0.402528 * sensorValue[1]) + tempKore);     // temp var
    difer[0] = round(tempMix - sensorValue[0]);             // diferenca temp
    if (difer[0] < 0)                             
        difer[0] = difer[0] * -1; 
}

//*****************************************************************************************
// *** glavna zanka ***********************************************************************
void loop() {
    beriSensor();                               // sensor vrednosti
    taktCount[0]++;                             // takt preverjanja spremembe temp. 
    taktCount[1]++;                             // takt delovanja (števec)    
    taktCount[5]++;                             // takt prikaza "kače"
   
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
                     taktValue++;
                     if (difer[0] > 1) {
                         if (difer[0] == difer[1]) { 
                             if (taktValue > 20) 
                                 izracunPozic();
                         }
                         else {  
                             if (difer[0] > difer[1]) 
                                izracunPozic();
                         }        
                     }
                     difer[1] = difer[0];
                     korekcija();
                  }
             }
             else {          //.........................POLETJE ....................................
                 if (swPrvic[1] == 1) {
                     if (sensorValue[1] > (minB)) {                // Prvič                        . 
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
       if (sensorValue[1] < minA){                            
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
       
     
       if (taktCount[0] == 1) {
            if (swStat[0] == 3) {
                if (swStatOld[0] != 3) 
                    swStatOld[1] = swStatOld[0];
            }   
            swStatOld[0] = swStat[0];
       }
        

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

    narisiKvadrat(99, 20, 220, 10, WHITE, 1);  
    tft.setTextColor(MAGENTA);  
    tft.setTextSize(1);
    tft.setCursor(99, 21);
    tft.print("START pozicija       sekund... ");    
    if (swPrvic[0] == 0) {
        taktCount[0] = 351;     
        impulz = 350;
        poziValue[1] = 70;
        narisiKvadrat(137, 219, 70, 10, BLACK, 1); 
        narisiKrog(162, 224, 12, CYAN, 1); 
        narisiKvadrat(158, 212, 9, 24, BLACK, 1);         
    }    
    else {   
        taktCount[0] = 211;   
        impulz = 210;
        narisiKrog(162, 224, 12, CYAN, 1); 
        narisiKvadrat(132, 219, 85, 10, BLACK, 1);        
    }     
    digitalWrite(digiPin[3], HIGH);    // mešalni ventil v celoti ODPREMO (start pozicija)
    digitalWrite(digiPin[2], HIGH);     // motorček ON

    for (int k = 0; k < impulz; k++)  {
         taktCount[0] = taktCount[0] - 1;
         narisiKvadrat(190, 20, 20, 10, WHITE, 1); 
         tft.setCursor(190, 21);
         tft.print(taktCount[0]); 
         if (k > 210)
             digitalWrite(digiPin[3], LOW);     // mešalni ventil postavimo na 1/3 odprto         
         delay(1000); 
         if (k == 30  or k == 60  or k == 90  or 
             k == 120 or k == 150 or k == 180 or k == 210) 
             izpisMonitor(); 
    }
    digitalWrite(digiPin[2], LOW);             // motorček OFF 
    narisiKvadrat(99, 20, 220, 20, BLACK, 1);
    impulz = 0;
    for (int k = 0; k < countSensor; k++)  {
         taktCount[k] = 0;
    }    
}
// --- konec prvic() ---------------------------------------------------------------
unsigned long izracunPozic() {
    if (sensorValue[6] > tempMix)                  // pretopla povratna voda!
        poziValue[0] = poziValue[1] - round(sensorValue[6] - tempMix);
    else                                           // delež odprtosti ventila
        poziValue[0] = 210 - (round(((sensorValue[5] - tempMix) / (sensorValue[5] - sensorValue[6])) * 210)); 

    if (poziValue[0] > (poziValue[1]  + poziMax)) 
        poziValue[0] = poziValue[1] + poziMax;
    if (poziValue[0] < (poziValue[1]  - poziMax)) 
        poziValue[0] = poziValue[1] - poziMax; 
    if (poziValue[0] < 0)
        poziValue[0] = 0;
   
    taktValue = 0;    
}
// --- konec izracunPozic() ---------------------------------------------------------------
unsigned long korekcija() {

    // korekcija mešalnega ventila pozimi
    // izpis trenutnih vrednosti na monitor - Serial.print

    if (poziValue[0] == poziValue[1]) {
        swStat[0] = 3;        // status delovanja
        impulz = 0;
        izpisMonitor(); 
    }
    else {
        if (poziValue[0] > poziValue[1]) {                // korekcija ON 
            digitalWrite(digiPin[3], HIGH);                // dodaja TOPLO vodo
            impulz = poziValue[0] - poziValue[1];
            swStat[0] = 2;
        }
        else {
            digitalWrite(digiPin[3], LOW);      // dodaja HLADNO vodo pozimi
            impulz = (poziValue[1] - poziValue[0]);
            swStat[0] = 4;                        // status delovanja 
        }
        digitalWrite(digiPin[2], HIGH);           // motorček ON
        izpisMonitor(); 
        for (int k = 0; k < impulz; k++)  {
             delay(1000);
        }     
        taktCount[0] = 1;
    }
    digitalWrite(digiPin[2], LOW);             // motorček OFF 
    poziValue[1] = poziValue[0];
    
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
        impulzOld = impulz;
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


unsigned long izpisTextdisplay() {
// *** prikaz na display ******************************************************************
   tft.fillScreen(BLACK);
   unsigned long start = micros();
   tft.setCursor(0, 0);
   tft.setTextColor(YELLOW);  tft.setTextSize(2);
   tft.println(" REGULACIJA  ZRAKA   V3.0");
   
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
   narisiKvadrat(30, 28, 29, 210, RED, 1);
   narisiKvadrat(60, 29, 28, 210, BLUE, 1); 
   narisiCrto(23, 80, 28, 80, YELLOW);   
   narisiCrto(23, 186, 28, 186, YELLOW);
   tft.setTextSize(1);
   tft.setCursor(5, 20);
   tft.println("210");
   tft.setCursor(5, 75);
   tft.println("157");
   tft.setCursor(5, 128);
   tft.println("105");
   tft.setCursor(9, 180);
   tft.println("52");
   tft.setCursor(13, 250);
   tft.println("0");
   
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
// --- konec izpisTextDisplay()
// --------------------------------------------------------

unsigned long prikazValue() {
 
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
              if (k == 0 or k == 5 or k == 6) tft.setTextColor(CYAN);
              else tft.setTextColor(YELLOW);            
              if (k < 7) tft.setTextSize(2);
                 else tft.setTextSize(1);
              if (sensorValue[k] < 10 and sensorValue[k] > -0.1)
                  tft.print(" ");   
              tft.print(sensorValue[k],1);
          }
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
    // brišem prejšnjo pozicijo ventila
    narisiKvadrat(30, 28, 59, 210, BLACK, 1); 

    // pozicija mešalnega ventila na display-u
    tft.setTextColor(WHITE);  
    tft.setTextSize(1);
    iRow = 27 + (210 - poziValue[0]);
    if (swStat[0] < 5) {
        narisiKvadrat(30, iRow, 59, poziValue[0], RED, 1);
        narisiKvadrat(30, 28, 59, (210 - poziValue[0]), BLUE, 1);
    }
    else
        narisiKvadrat(30, iRow, 59, poziValue[0], BLUE, 1);    
    for (int k = 22; k < 90; k=k+7) {
         narisiCrto(k, 133, k+4, 133, YELLOW);
    }      
    narisiKvadrat(93, iRow-20, 15, 40, BLACK, 1);
    narisiCrto(30, iRow, 103, iRow, GREEN);
    //-------------------------------------------------------
    if (swStat[0] == 2) {
        if (poziValue[0] < 190) {
            narisiTrikotnik(30, iRow-20, 52, iRow-20, 41, iRow-30 , RED, 1); 
            narisiKvadrat(34, iRow-20, 14, 16, RED, 1); 
            narisiTrikotnik(30, iRow-20, 52, iRow-16, 41, iRow-30 , WHITE, 0); 
            narisiKvadrat(34, iRow-20, 14, 16, WHITE, 0); 
            narisiKvadrat(35, iRow-20, 12, 2, RED, 1); 
            tft.setCursor(36, (iRow-14));
            tft.print(impulz);
        }   

        if (poziValue[0] > 20) {
            narisiTrikotnik(68, iRow+20, 89, iRow+20, 78, iRow+30 , WHITE, 0); 
            narisiKvadrat(72, iRow+4, 14, 16, WHITE, 0);
            narisiKvadrat(75, iRow-19, 12, 2, BLACK, 1);  
            tft.setCursor(74, (iRow+6));
            tft.print(impulzOld);
        }   
    }
    if (swStat[0] == 3) {
         if (poziValue[0] > 20) {
            narisiTrikotnik(68, iRow+20, 89, iRow+20, 78, iRow+30 , BLUE, 1); 
            narisiKvadrat(72, iRow+4, 14, 16, BLUE, 1);            
            narisiTrikotnik(68, iRow+20, 89, iRow+16, 78, iRow+30 , WHITE, 0); 
            narisiKvadrat(72, iRow+4, 14, 16, WHITE, 0); 
            narisiKvadrat(72, iRow+19, 12, 2, BLUE, 1); 
            tft.setCursor(74, (iRow+6));
            tft.print(impulz);
        }   
        if (poziValue[0] < 190) {
            narisiTrikotnik(30, iRow-20, 52, iRow-16, 41, iRow-30 , WHITE, 0); 
            narisiKvadrat(34, iRow-20, 14, 16, WHITE, 0); 
            narisiKvadrat(35, iRow-20, 12, 2, RED, 1); 
            tft.setCursor(74, (iRow-6));
            tft.print(impulzOld);
        }    
    }
   //--------------------------------------------------------     
    poziValue[2] = round(((poziValue[0])*100)/210);
    tft.setCursor(95, (iRow-12));
    tft.print(poziValue[0]);
    tft.setCursor(93, (iRow+4));
    tft.print(poziValue[2]);
    tft.print("%");  
    //-------------------------------------------------------
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
        Serial.println("       voda   zunaj  I.izm  rekup  II.izm TC     povrat   box  dile              impulz Status tempMix difer dife takt pozi poziOLD");
    } 
    for (int k = 0; k < countSensor; k++) {
         if (k == 0) {
             if (taktCount[1] < 1000) { 
                 Serial.print(" ");
                 if (taktCount[1] < 1000) { 
                     Serial.print(" ");
                     if (taktCount[1] < 100) {
                         Serial.print(" ");
                         if (taktCount[1] < 10)
                             Serial.print(" ");
                     }        
                 }
             }    
             Serial.print(taktCount[1]);
             Serial.print("  "); 
         }
         Serial.print(sensorValue[k]);
         Serial.print("  "); 
    } 
    switch (swStat[0]) {
        case 0:
            Serial.print("zima PRVIC");
        break;
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
     if (impulz > 0) {
         if (swStat[0] > 3)
             Serial.print("-");
         else
             Serial.print("+");
     }        
     Serial.print(impulz);
     if (swObtok == 1)
         Serial.print(" +C");    
     Serial.print(" [");
     Serial.print(impulz);
     Serial.print("] ");
     Serial.print(swStat[0]);
     Serial.print("=");
     Serial.print(swStatOld[0]);
     Serial.print("=>");
     Serial.print(swStatOld[1]);
     Serial.print(" ");
     Serial.print(tempMix);  
     Serial.print(" ");
     Serial.print(difer[0]); 
     Serial.print(" ");
     Serial.print(difer[1]); 
     Serial.print(" ");     
     Serial.print(taktValue); 
     Serial.print(" ");
     Serial.print(poziValue[0]); 
     Serial.print(" ");
     Serial.print(poziValue[1]); 
     Serial.print("\n");
}
// --- konec izpisMonitor() --------------------------------------------------------
// *** konec prikaz na monitor računalnika ************************************************
