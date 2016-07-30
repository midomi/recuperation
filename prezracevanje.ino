// V 4.1

// Program za krmiljenje priprave zraka z rekuperacijo in 
// dogrevanjem/pohlajevanjem s pomočjo toplotne črpalke 

// knjižnice
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <OneWire.h>
#include <DallasTemperature.h>

// BUS1 priključen na port 41
#define ONE_WIRE_BUS1 41
OneWire oneWire1(ONE_WIRE_BUS1);
DallasTemperature sensors1(&oneWire1);

DeviceAddress sensor0 = { 0x28, 0xC6, 0x7A, 0x78, 0x06, 0x00, 0x00, 0x86 };
DeviceAddress sensor1 = { 0x28, 0x8E, 0x6D, 0x6E, 0x06, 0x00, 0x00, 0x4E };
DeviceAddress sensor2 = { 0x28, 0x81, 0x2D, 0x70, 0x06, 0x00, 0x00, 0xAE };
DeviceAddress sensor3 = { 0x28, 0xA9, 0x60, 0x70, 0x06, 0x00, 0x00, 0xBC };
DeviceAddress sensor4 = { 0x28, 0x39, 0xE1, 0x6E, 0x06, 0x00, 0x00, 0x26 };
DeviceAddress sensor5 = { 0x28, 0x23, 0x05, 0x78, 0x06, 0x00, 0x00, 0x46 };
DeviceAddress sensor6 = { 0x28, 0xD7, 0x64, 0x6F, 0x06, 0x00, 0x00, 0x8A };
DeviceAddress sensor7 = { 0x28, 0x72, 0x43, 0x77, 0x06, 0x00, 0x00, 0xB0 };
DeviceAddress sensor8 = { 0x10, 0x9C, 0x49, 0x77, 0x02, 0x08, 0x00, 0x3A };
// ***************************   nastavljive spremenljivke  ***********xx***
         // vrednosti pogojev delovanja toplotnih izmenjevalnikov A in B 
float minA = 12.0;       // meja zima/greje
float maxB = 23.0;       // meja poleti/hlajenje
float minVoda = 3.0;     // meja ALARM - rekuperator OFF 
float sensorMax = 50.0;  // meja ALARM - rekuperator OFF (sensor ne deluje)
// ************konec *********   nastavljive spremenljivke  **************
                         // korekcijski faktorji tipal
float sensorKore[] = {0.33, -0.53, 0.26, 0.40, -0.59, 0.27, 0.00, 0.00, 0.00}; 
float tempMix = 0;       // temperatura na katero se dela mix 
int impulz = 0;          // impulz motorčka
int poziValue[] = {0, 0, 0};      // pozicija mešalnega ventila
                                  // 0. dejanska pozicija
                                  // 1. preračunana pozicija za prikaz
                                  // 2. preračunana pozicija prejšnja vrednost   
int smer[] = {0, 0};         // 0. smer delovanja mešalnega ventila (1=odpiramo, -1=zapiramo)
                             // 1. smer prejšnja vrednost
int loputa = 0;              // prikaz lopute
float procent[] = {0, 0};         // 0. procent pozicije mešalnega ventila 
                                  // 1. procent pozicije mešalnega ventila prejšnja vrednost
int obrat[] = {1, 1};    // svič za prikaz pretoka vode 0 - črpalka, 1- zaporni ventil
float difTemp[] = {0, 0, 0, 0};  // 0. razlika temp. med mix-vodo in izračunano tempMix  
                                 // 1. razlika odstopanja temp. mix-vode do meje 0,5°C
                                 // 2. stara vrednost 
                                 // 3. vsota diferenc temp. 

int taktValue  = 0;      // števec mirovanja po korekciji
int taktCount[]  = {0, 0, 0};   // števci delovanja 
                         // 0. števec takta 
                         // 1. zaporedno število prebranih za monitor
                         // 2. števec za delay 20 sekund zamika nove korekcije 
int swPrvic[] = {1, 1, 1, 1, 1};  // svič prvič v tem režimu (1-ON, 0-OFF)
                          // 0. program 
                          // 1. < minA (gretje)
                          // 2. > maxB (hlajenje)
                          // 3. > minA, < maxB (bypass)
                          // 4. zmrzal
int countPrvic[] = {10, 0, 0, 0};  // števec ponovitev zunanje temperature pred menjavo režima
                          // 0. število ponovitev 
                          // 1. števec (gretje)
                          // 2. števec (hlajenje)
                          // 3. števec (bypass)
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
//                  {  MIX    zunaj     A.izm   rekup    B.izm      TČ   povMIX    povTČ     soba};
int tCol[] =        {  200,     140,      140,    140,     140,    200,     260,     260,     140};  // stolpci vrednosti 
int tRow[] =        {  137,     107,      137,    167,     197,    197,     137,     197,     225};  // vrstica vrednosti

int countSensor = 9;     // Ĺˇtevilo tipal 
int digiPin[] = {13, 12, 47, 48, 49, 50, 51, 53};  // output digital pini (LED, Beeper, rele)

                         // 0. 13 LED D8 Arduino   
                         // 1. 12 alarm (Beeper)
                         // 2. 47 rele mešalni ventil ON/OFF
                         // 3. 48 mešalni ventil (delovni-odpira, mirovni-zaprira)
                         // 4. 49 obtočna črpalka ON/OFF
                         // 5. 50 zaporni ventil ON/OFF  
                         // 6. 51 rekuperator OFF/ON (POZOR delovni-OFF, mirovni-ON)
                         // 7. 53 loputa ON (zima - dol)
                         // 8. 55 loputa OFF (poleti - gor)
int countPin = 9;        // število output pinov


// *** setup *******************************************************
void setup() {
  // postavi digitalne pine na OUTPUT in priredi LOW vrednost:
  for (int k = 0; k < countPin; k++)  {
       pinMode(digiPin[k], OUTPUT);
       digitalWrite(digiPin[k], LOW);   
  }
 
  sensors1.begin();
  Serial.begin(9600); 
  // začetne vrednosti:
     initVrednost();
     sensors1.setResolution(sensor0, 11);
     sensors1.setResolution(sensor1, 11);
     sensors1.setResolution(sensor2, 11);
     sensors1.setResolution(sensor3, 11);
     sensors1.setResolution(sensor4, 11);
     sensors1.setResolution(sensor5, 11);
     sensors1.setResolution(sensor6, 11);
     sensors1.setResolution(sensor7, 11);
     sensors1.setResolution(sensor8, 11);  

  // nastavitve display-a
     tft.reset();
     uint16_t identifier = tft.readID();
     tft.begin(identifier);
     uint8_t rotation=1;
     tft.setRotation(rotation);
     tft.fillScreen(BLACK);
}
// --- konec SETUP()-----------------------------------

// *** inicializacija vrednosti ************************************
void initVrednost() {
  for (int k = 0; k < countSensor; k++)  {
    sensorValue[k] = 0;
  }
  impulz = 0;
}

// *** branje vrednosti tipal **************************************
void beriSensor() {
  // beri vrednost 
    sensors1.requestTemperatures();
    sensorValue[0] = sensors1.getTempC(sensor0);
    sensorValue[1] = sensors1.getTempC(sensor1);
    sensorValue[2] = sensors1.getTempC(sensor2);
    sensorValue[3] = sensors1.getTempC(sensor3);
    sensorValue[4] = sensors1.getTempC(sensor4);    
    sensorValue[5] = sensors1.getTempC(sensor5);
    sensorValue[6] = sensors1.getTempC(sensor6);
    sensorValue[7] = sensors1.getTempC(sensor7);
    sensorValue[8] = sensors1.getTempC(sensor8);
    for (int k = 0; k < countSensor; k++)  {
         sensorValue[k] = sensorValue[k] + sensorKore[k];
    }
}

//*****************************************************************************************
// *** glavna zanka ***********************************************************************
void loop() {
    beriSensor();                               // sensor vrednosti
    taktCount[0]++;                             // takt preverjanja spremembe temp. 
    taktCount[1]++;                             // takt delovanja (števec)    
    if (taktCount[2] > 0)
        taktCount[2]--;                         // takt korekcije (pri spremembi smeri mešalnega ventila 
                                                // počakamo 20 sekund do naslednje korekcije)
    // A. ... VAROVALKA zmrzali vode   ..................................
         if (sensorValue[0] < minVoda or sensorValue[0] > sensorMax or sensorValue[2] < minVoda) { // voda na  vhodu < 3°C  ali >> ALARM
                                                                                                   // zrak na izhodu < 3°C      >> ALARM
                 if (swPrvic[0] == 1) {
                   digitalWrite(digiPin[6], HIGH);                     // rekuperator OFF
                   digitalWrite(digiPin[1], HIGH);                     // ALARM piskač ON
                   digitalWrite(digiPin[4], HIGH);                     // obtočna črpalka ON                    
                   digitalWrite(digiPin[3], HIGH);                     // dodaja TOPLO vodo
                   digitalWrite(digiPin[2], HIGH);                     // mixaj
                   smer[0] = 1;
                   swPrvic[0] = 1; 
                   swPrvic[4] = 0;                                     
                 }  
                 impulz = 1;
                 pozicMesalniVentil();
         }
         else { 
    // B. ... START rekuperatorja .....................
             if (swPrvic[0] == 1) {
                 P_program();
                 swPrvic[0] = 0;
                 swPrvic[1] = 1;        
                 swPrvic[2] = 1;        
                 swPrvic[3] = 1;                                        
                 swPrvic[4] = 1;                     
             }
             else {
     // C. ... ZIMA .................................
                 if (sensorValue[1] < minA) {  
                     if (swPrvic[1] == 1) {
                         countPrvic[1]++;
                         if (countPrvic[1] == countPrvic[0]) {
                             countPrvic[1] = 0;
                             P_zima();
                             swPrvic[1] = 0;        
                             swPrvic[2] = 1;        
                             swPrvic[3] = 1;                                        
                         }
                     }    
                     else {   
                         taktValue++; 
                         izracunDiferenc();
                         if (difTemp[0] > difTemp[2]) {
                             if (taktCount[2] == 0) { 
                                 dolociImpulz();  
                             }    
                         }        
                         else {
                             taktValue = 0;
                             difTemp[3] = 0;
                             }                               
                         difTemp[2] = difTemp[0];        
                         if (impulz > 0) {
                             korekcija();    
                         }    
                     } 
                 }
                 else {       
    // D. ... POLETJE ............................
                     if (sensorValue[1] > maxB) {  
                         if (swPrvic[2] == 1) {
                             countPrvic[2]++;
                             if (countPrvic[2] == countPrvic[0]) {
                                countPrvic[2] = 0;
                                P_poletje();
                                swPrvic[1] = 1;        
                                swPrvic[2] = 0;        
                                swPrvic[3] = 1;                                        
                             }    
                         }
                     }
                     else {      
    // E. ... BYPASS ..............................
                         if (swPrvic[3] == 1) {
                             countPrvic[3]++;
                             if (countPrvic[3] == countPrvic[0]) { 
                                countPrvic[3] = 0;
                                P_bypass();
                                swPrvic[1] = 1;        
                                swPrvic[2] = 1;        
                                swPrvic[3] = 0;                                        
                             }                         
                         }
                     }      
                 }
             }   
         }
     pozicMesalniVentil();
     prikazCrpalka();
     prikazZaporniVentil();
     prikazLoputa();
     prikazTemperatur();

     initVrednost();              // inicializacija vrednosti spremenljivk

  // turn the ledPin on
  digitalWrite(digiPin[0], HIGH);  
  // stop the program for <sensorValue> milliseconds:
  delay(500);          
  // turn the ledPin off:        
  digitalWrite(digiPin[0], LOW);   
  // stop the program for <sensorValue> milliseconds:
  delay(500);   
}     
// *** konec glavne zanke ***
// **********************************************************************************

// ***  PROGRAM prvič ***************************************************************
 unsigned long P_program() {                    
         digitalWrite(digiPin[6], LOW);              // rekuperator ON
         digitalWrite(digiPin[1], LOW);              // ALARM  piskač OFF 
         digitalWrite(digiPin[5], HIGH);             // zaporni ventil ON (omogoči pretok vode na izhodnih izmenjevalnikih)   
         
         tft.fillScreen(BLACK);
         unsigned long start = micros();
         tft.setCursor(0, 0);
         tft.setTextColor(YELLOW);  
         tft.setTextSize(2);
         tft.println("REGULACIJA ZRAKA V4.1");
         tft.setCursor(140, 77);
         tft.println("ZRAK");
         tft.setCursor(230, 77);
         tft.setTextColor(CYAN);         
         tft.println("VODA");
         tft.setCursor(215, 107);
         tft.println("IN"); 
         tft.setTextColor(WHITE);
         tft.setCursor(265, 107);
         tft.println("OUT"); 
         tft.setCursor(3, 107);
         tft.println("zunaj");         
         tft.setCursor(3, 137);
         tft.println("izmen.IN");    
         tft.setCursor(3, 167);   
         tft.println("rekuperator");     
         tft.setCursor(3, 197);   
         tft.println("izmen.OUT");            
         tft.setCursor(3, 225);            
         tft.println("dnevna soba"); 
         narisiCrto(3, 130, 317, 130, WHITE);      
         narisiCrto(3, 220, 317, 220, WHITE);             
           // mešalni ventil
         narisiKvadrat(0, 28, 88, 28, WHITE, 0);  
         narisiKvadrat(1, 29, 86, 26, WHITE, 0);  
         narisiTrikotnik(0, 40, 0, 28, 86, 28, BLACK, 1);
         narisiCrto(0, 40, 86, 29, WHITE);
         narisiCrto(0, 39, 86, 28, WHITE);      
           // obtočna črpalka
         narisiKrog(130, 47, 17, WHITE, 0);
         narisiKrog(130, 47, 16, WHITE, 0); 
           // zaporni ventil 
         narisiKrog(200, 47, 17, WHITE, 1);
         narisiKvadrat(175, 40, 50, 14, WHITE, 0);
         narisiKvadrat(175, 41, 50, 12, WHITE, 0);        
         narisiKvadrat(175, 42, 50, 10, BLACK, 1);
           // loputa
         narisiKvadrat(260, 30, 40, 30, WHITE, 0);        
         narisiKvadrat(261, 31, 38, 28, WHITE, 0);
         if (sensorValue[8] > sensorValue[4]) {
             loputa = 1;
         }
               
         prikazTemperatur();
         if (sensorValue[1] > minA and sensorValue[1] < maxB) {  
             digitalWrite(digiPin[3], LOW);         // mešalni ventil na OFF v celoti ZAPREMO
             smer[0] = -1;
         }
         else {  
              digitalWrite(digiPin[3], HIGH);         // mešalni ventil na ON v celoti ODPREMO
              smer[0] = 1;
         }
         digitalWrite(digiPin[2], HIGH);         // motorček ON

         impulz = 210;
         pozicMesalniVentil;
         return micros() - start;            
 } // --- PROGRAM prvič - KONEC ----- 
// *** ZIMA prvič  ******************************************************************                                                      
 unsigned long P_zima() {                       

         digitalWrite(digiPin[4], HIGH);             // obtočna črpalka ON 
         if (poziValue[0] == 210) { 
             digitalWrite(digiPin[3], LOW);          // mešalni ventil postavimo na 1/4 odprto
             digitalWrite(digiPin[2], HIGH);         // motorček ON
             smer[0] = -1;
             impulz = 160;
             pozicMesalniVentil();
         }
         if (poziValue == 0 ) {
             digitalWrite(digiPin[3], HIGH);     // mešalni ventil postavimo na 1/4 odprto 
             digitalWrite(digiPin[2], HIGH);     // motorček ON
             smer[0] = 1;
             impulz = 50;
             pozicMesalniVentil();
         }  
 } // --- ZIMA prvič - KONEC ---
// *** POLETJE prvič *****************************************************************
 unsigned long P_poletje() {                 
         digitalWrite(digiPin[3], HIGH);         // mešalni ventil na ON v celoti ODPREMO
         digitalWrite(digiPin[2], HIGH);         // motorček ON
         smer[0] = 1;
         impulz = 210;
         pozicMesalniVentil;
 } // --- POLETJE prvič - KONEC ----- 
// *** BYPASS prvič *****************************************************************                                               
 unsigned long P_bypass() {
         digitalWrite(digiPin[4], LOW);              // obtočna črpalka na OFF
         digitalWrite(digiPin[3], LOW);              // mešalni ventil na OFF v celoti ZAPREMO 
         digitalWrite(digiPin[2], HIGH);             // motorček ON
         smer[0] = -1; 
         impulz = 210;
         pozicMesalniVentil();
 } // --- BYPASS prvič - KONEC -----
// ************************************************************************************************

// *** pozicMesalniVentil ************************************************************************      
unsigned long pozicMesalniVentil() {
         tft.setTextColor(WHITE);  
         tft.setTextSize(2);
         taktCount[0] = impulz;
         for (int k = impulz; k > 0; k--)  {
              narisiKvadrat(280, 1, 37, 20, BLACK, 1); 
              tft.setCursor(283, 3);
              tft.print(taktCount[0]);
              taktCount[0]--;
              poziValue[0] = poziValue[0] + smer[0];
              if (poziValue[0] < 0) {
                poziValue[0] = 0;
                k = 0;
              }
              if (poziValue[0] > 210) {
                poziValue[0] = 210;
                k = 0;
              }                 
              prikazMesalniVentil();   
              prikazTemperatur();                  
              izpisMonitor(); 
              delay(1000);
         }
         digitalWrite(digiPin[2], LOW);               // motorček OFF              
         narisiKvadrat(280, 1, 37, 20, BLACK, 1);            
         impulz = 0;
         for (int k = 0; k < 3; k++)  {
              taktCount[k] = 0;
         }
 } // --- pozicMesalniVentil - KONEC ---         
// *** prikazMesalniVentil ************************************************************************      
unsigned long prikazMesalniVentil() {
           

     // pozicija mešalnega ventila na display-u
        poziValue[1] = round((poziValue[0] / 10)) * 4;  
        if (poziValue[1] != poziValue[2]) {
            poziValue[2] = poziValue[1]; 
            narisiKvadrat(2, 30, 84, 24, BLACK, 1);                   // brišem prejšnjo pozicijo ventila                                            
            if (sensorValue[0] > sensorValue[1]) {                    // barva   
                narisiKvadrat(2, 30, poziValue[1], 24, RED, 1);
            }
            else {
                narisiKvadrat(2, 30, poziValue[1], 24, BLUE, 1);
            }
            narisiTrikotnik(0, 40, 0, 28, 88, 28, BLACK, 1);
            narisiCrto(0, 40, 86, 29, WHITE);
            narisiCrto(0, 39, 86 , 28, WHITE);
        }    
        procent[0] = (poziValue[0] * 100) / 210;
        if (procent[0] != procent[1]) {
            procent[1] = procent[0]; 
            tft.setTextColor(WHITE); 
            tft.setCursor(7, 58);
            narisiKvadrat(7, 58, 40, 14, BLACK, 1);
            tft.print(procent[0],0);
            tft.setCursor(32, 58); 
            tft.print("%");
        } 
        if (smer[0] != smer[1]) {
            smer[1] = smer[0];
            narisiKvadrat(30, 80, 30, 10, BLACK, 1);
            narisiCrto(30, 85, 60, 85, WHITE);
            if (smer[0] > 0) {
                narisiCrto(56, 81, 60, 85, WHITE);
                narisiCrto(56, 89, 60, 85, WHITE);  
            }
            else {
                narisiCrto(30, 85, 34, 81, WHITE);
                narisiCrto(30, 85, 34, 89, WHITE); 
            }
        }             
        tft.setTextColor(WHITE); 
        tft.setCursor(60, 58);
        narisiKvadrat(60, 58, 40, 14, BLACK, 1);
        tft.print(poziValue[0]);  
 } // --- prikazMesalniVentil - KONEC ---
// *** prikazCrpalka ********************************************************************  
unsigned long prikazCrpalka() {
        if (sensorValue[0] > sensorValue[1]) {
            narisiKrog(130, 47, 15, RED, 1); 
        }
        else {
            narisiKrog(130, 47, 15, CYAN, 1); 
        }   
        if (swPrvic[3] != 0) {
            switch (obrat[0]) {
                  case 1:
                      for (int k = 0; k < 15; k++)  {
                         narisiCrto(130+k, 47, 130+k, 47-(14-k), BLACK);                        
                      }
                     obrat[0] = 2;    
                  break;
                  case 2:
                      for (int k = 0; k < 15; k++)  {
                         narisiCrto(130+k, 47, 130+k, 47+(15-k), BLACK);
                      }
                     obrat[0] = 3;    
                  break;
                  case 3:
                      for (int k = 0; k < 15; k++)  {
                         narisiCrto(130-k, 47, 130-k, 47+(15-k), BLACK);
                      }
                      obrat[0] = 4;
                  break; 
                  case 4:
                      for (int k = 0; k < 15; k++)  {
                         narisiCrto(130-k, 47, 130-k, 47-(14-k), BLACK);
                      }
                      obrat[0] = 1;
                  break;                   
            }
        }
 } // --- prikazCrpalka - KONEC ---   
// *** prikazZaporniVentil ********************************************************************   
unsigned long prikazZaporniVentil() {
        if (sensorValue[0] > sensorValue[1]) {
            narisiKvadrat(175, 42, 50, 10, RED, 1);
        }
        else {
            narisiKvadrat(175, 42, 50, 10, CYAN, 1);
        }   
        if (swPrvic[3] == 0) {
            narisiKvadrat(200, 42, 25, 10, WHITE, 0);
            narisiKrog(200, 47, 17, WHITE, 1);
        } 
        else { 
            switch (obrat[1]) {
                  case 1:
                      narisiKvadrat(191, 42, 34, 10, BLACK, 1);
                      obrat[1] = 2;    
                  break;
                  case 2:
                      narisiKvadrat(175, 42, 17, 10, BLACK, 1);
                      narisiKvadrat(208, 42, 17, 10, BLACK, 1);
                      obrat[1] = 3;
                  break; 
                  case 3:
                      narisiKvadrat(175, 42, 34, 10, BLACK, 1);
                      obrat[1] = 1;
                  break; 
            }
        }
 } // --- prikazZaporniVentil - KONEC ---   
// *** prikazLoputa ********************************************************************   
unsigned long prikazLoputa() {
        if (sensorValue[8] > sensorValue[4]) {
            if (loputa == 1) {
                narisiKvadrat(260, 30, 40, 30, BLACK, 1); 
                narisiKvadrat(260, 30, 40, 30, WHITE, 0);        
                narisiKvadrat(261, 31, 38, 28, WHITE, 0);
                narisiTrikotnik(262, 33, 262, 58, 297, 58, RED, 1);
                narisiCrto(262, 32, 297, 58, YELLOW);
                narisiCrto(263, 32, 298, 58, YELLOW);
                loputa = 0;
            }    
        }
        else {
            if (loputa == 0) {
                narisiKvadrat(260, 30, 40, 30, BLACK, 1); 
                narisiKvadrat(260, 30, 40, 30, WHITE, 0);        
                narisiKvadrat(261, 31, 38, 28, WHITE, 0);
                narisiTrikotnik(263, 32, 298, 32, 298, 57, CYAN, 1);
                narisiCrto(262, 32, 297, 58, MAGENTA);
                narisiCrto(263, 32, 298, 58, MAGENTA);
                loputa = 1;
            }   
        }   
 } // --- prikazLoputa - KONEC ---   
// *** prikazTemperatur ********************************************************************   
unsigned long prikazTemperatur() {
    // vrednosti temperatur
      tft.setTextColor(YELLOW); 
      for (int j = 0; j < (countSensor); j++)  { 
          if (sensorValue[j] != sensorValueOld[j]) {
              sensorValueOld[j] = sensorValue[j]; 
              narisiKvadrat(tCol[j], tRow[j], 60, 20, BLACK, 1);     
              tft.setCursor(tCol[j], tRow[j]);
              switch (j) {
                    case 0:
                        tft.setTextColor(CYAN);    
                    break;
                    case 5:
                        tft.setTextColor(CYAN);
                    break;               
                    case 6:
                        tft.setTextColor(WHITE);    
                    break;
                    case 7:
                        tft.setTextColor(WHITE);
                    break;  
                    default:
                        tft.setTextColor(YELLOW);
              }         
              if (sensorValue[j] < 10 and sensorValue[j] > -0.1)
                  tft.print(" ");   
              tft.print(sensorValue[j],1);
          }
      }
 } // --- prikazTemperatur - KONEC ---
// ****************************************************************************************

// *** izracunDiferenc ********************************************************************   
unsigned long izracunDiferenc() {    
      // izračun variabilne temp. VODE       
    tempMix = 14.7305 - (0.402528 * sensorValue[1]);     // temp var
    difTemp[0] = tempMix - sensorValue[0];             // diferenca temp vode
    if (difTemp[0] < 0)                             
        difTemp[0] = difTemp[0] * -1; 
       
       // izračun odstopanja povprečja diferenc temp. od meje 0,5°C
    if (difTemp[0] > 0.5) {
        difTemp[3] = difTemp[3] + difTemp[0]; 
        difTemp[1] = (difTemp[3] / taktValue ) - 0.5;
    }
    else {
        difTemp[1] = 0; 
        difTemp[3] = 0;
        taktValue = 0;
    }    
 } // --- izracunDiferenc - KONEC --- 
// *** dolociImpulz ********************************************************************  
unsigned long dolociImpulz() {
      // določi impulz z mikronastavitvijo za vsake 0,15°C 
      if (difTemp[1] > 0) {
          if (difTemp[1] < 0.15) 
              impulz = 1;
          else {
              if (difTemp[1] < 0.30) {
                  impulz = 2;
              }
              else {
                  if (difTemp[1] < 0.45) {
                      impulz = 3;
                  }
                  else {
                      if (difTemp[1] < 0,60) {
                          impulz = 4;
                      }
                      else {
                          if (difTemp[1] < 0,75) {
                              impulz = 5;
                          }
                          else {
                              if (difTemp[1] < 1) 
                                  impulz = 6;
                              else 
                                  impulz = 10;
                          }
                      }
                  }    
              }
          }
      }   
 } // --- dolociImpulz - KONEC --- 
// *** korekcija ********************************************************************  
unsigned long korekcija() {
    // korekcija mešalnega ventila pozimi
        if (tempMix > sensorValue[0]) {                
            digitalWrite(digiPin[3], HIGH);            // korekcija z odpiranjem ventila (dodaja TOPLO vodo)
            digitalWrite(digiPin[2], HIGH);            // motorček ON
            if (smer[0] < 0) {                            // pri menjavi smeri mešalnega ventila  
                impulz = 8;                            // postavimo impulz na 8  
                taktCount[2] = 20;                     // in 20 sekund delaya  da se čimprej približalo idealni temp. 
            } 
            smer[0] = 1;    
         }
        else {
            digitalWrite(digiPin[3], LOW);             // korekcija z zapiranjem ventila (dodaja HLADNO vodo)
            digitalWrite(digiPin[2], HIGH);            // motorček ON
            if (smer[0] > 0) {
                impulz = 8;
                taktCount[2] = 20;
            }     
            smer[0] = -1;
        }
        izpisMonitor(); 
        pozicMesalniVentil();
 } // --- korekcija - KONEC --- 
// ***********************************************************************************************

// *** narisiCrto ********************************************************* 
unsigned long narisiCrto(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    tft.drawLine(x1, y1, x2, y2, color);
} // --- narisiCrto - KONEC ---
// *** narisiKvadrat ******************************************************
unsigned long narisiKvadrat(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color, uint8_t fill) {
  if(fill == 1)
     tft.fillRect(x, y, w, h, color);
  else   
     tft.drawRect(x, y, w, h, color);
} // --- narisiKvadrat - KONEC ---
// *** narisiKrog *********************************************************
unsigned long narisiKrog(uint8_t x, uint8_t y, uint8_t radius, uint16_t color, uint8_t fill) {
  if(fill == 1)
     tft.fillCircle(x, y, radius, color);
  else 
     tft.drawCircle(x, y, radius, color);
} // --- narisiKrog - KONEC --- 
// *** narisiTrikotnik ***************************************************
unsigned long narisiTrikotnik(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,  uint16_t x3, uint16_t y3, uint16_t color, uint8_t fill) {
  if(fill == 1)
    tft.fillTriangle(x1, y1, x2, y2, x3, y3, color);
  else 
    tft.drawTriangle(x1, y1, x2, y2, x3, y3, color);
} // --- narisiTrikotnik - KONEC ---

// *** izpisMonitor ******************************************************
unsigned long izpisMonitor() {
    if (taktCount[1] == 1) {
        Serial.println("       voda   zunaj  I.izm  rekup  II.izm TC     povrat   TCpov dnevna          impulz tempMix difer dife takt pozi");
    } 
    for (int i = 0; i < countSensor; i++) {
         if (i == 0) {
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
         Serial.print(sensorValue[i]);
         Serial.print("  "); 
    } 
    if (swPrvic[0] == 0)
        Serial.print("START ");
    if (swPrvic[1] == 0)
        Serial.print("OGREVA");
    if (swPrvic[2] == 0)
        Serial.print("HLADI ");
    if (swPrvic[3] == 0)
        Serial.print("BYPASS");        

     Serial.print(" ");
     Serial.print(impulz);
     Serial.print(" ");
     Serial.print(tempMix);  
     Serial.print(" ");
     Serial.print(difTemp[0]); 
     Serial.print(" ");
     Serial.print(taktValue); 
     Serial.print(" ");
     Serial.print(poziValue[0]); 
     Serial.print("\n");
}
// --- izpisMonitor - konec ---
// ********************************************************************************

