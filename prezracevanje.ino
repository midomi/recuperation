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
 
float minVoda = 3.0;     // mejna vrednost delovanja rekuperatorja (tipalo A0) 
float tempZrak = 10.0;   // izhodiščna temperatura zraka za rekuperator 
float difer[k];          // razlika med izhodišlno temp. (tempZrak) in trenutno temp. senzorja   

                // takt delovanja priprave zraka A.izmnenjevalnika
                         // 0. takt pozimi 
                         // 1. takt poleti (samo ob prehodu na poletje)  
                         // 2. impulz - število korekcij mešalnega ventila po taktu pozimi
int taktValue[] = {60, 210, 0};

                // števci delovanja 
                         // 0. števec takta
                         // 1. števec delovanja mešalnega ventila
                         // 2. števec prikaza delovanja zapornega ventila na display
int taktCount[] = {0, 0, 0};     

         // tipla {   A0,    A1,    A2,    A3,    A4}         
float korek[] =   {-0.50, -0.50, -0.50, -0.50, -1.50}; // korekcijski faktor analognih tipal
int uporValue[] = { 1049,  1049,  1049,  1049,  1049}; // pretvornik iz Ohm v °C 
int indexValue = 60;     // število zaporednih branj tipal
int index = 0;           // števec prebranih vhodov
int countRota[] = {0, 0};         // števec rotacije obtočne črpalke
                         // 0. status I. izmenjevalnika
                         // 1. status II. izmenjevalnika
int swStat[] = {0, 0};   
int swObtok = 0;         // svič delovanja obtočne črpalke
int swVentil = 2;        // svič prikaza delovanja ventila
int sVentil = 2;         // svič prikaza delovanja ventila - prejšnji
int swI[] = {0, 0};            // svič padanja/naraščanja temp.
                         // 0. prvič zima
                         // 1. prvič poletje
                         // 2. prvič greje
                         // 3. prvič hladi
int swPrvic[] = {1, 1, 1, 1}; 

int jj;
int kk;

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
char textAA[7][7] = {"voda ", "zunaj", "A.izm", "rekup", "B.izm", "dile ", "box  "};
int tCol[] =        {    0,      50,      180,    180,     180,        0,      0};            // stolpci vrednosti 
int tRow[] =        {  100,      56,       56,    126,     196,      120,    140};            // vrstica vrednosti

int pRow[] =        {  58,      78,       98,    118,     138,      158,    178};            // vrstica kroga 

int pCol[] =        {  25,     105,      185,     265};           // kolona kroga 
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
int zap[7];            // število zaporedno prebranih
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
       
       difer[k] = tempZrak - sensorValue[k]               // izračun odstopanja temperatur od 
       if (difer[k] < 0)                                  // od izhodiščne temp. za rekuperacijo
           difer[k] = difer[k] * -1;                      //    
  }

  // beri vrednost digitalnih tipal
  sensors.requestTemperatures();
  sensorValue[5] = sensors.getTempCByIndex(0);
  sensorValue[6] = sensors.getTempCByIndex(1); 
  
  // izračun diference
      difer = tempZrak - sensorValue[2];                                                
    //abs(difer);
    if (difer < 0)
        difer = difer * -1; 
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
    beriSensor();              // sensor vrednosti
    taktCount[0]++;                                // takt preverjanja spremembe temp.
    if (taktCount[0] > taktValue[0])               //
        taktCount[0] = 1;                          // začetna vrednost takta
   
    // A.izmenjevalnik ............................VAROVALKA ..................................

         if (sensorValue[0] < minVoda) {                           // voda < 3°C                   .  
             digitalWrite(digiPin[13], HIGH);                        // rekuperator OFF              .
             digitalWrite(digiPin[8], HIGH);                         // ALARM piskač                 .
             digitalWrite(digiPin[10], HIGH);                        // dodaja TOPLO vodo            .                              
             digitalWrite(digiPin[9], HIGH);                         // mixaj                        .
             obtok(1);                                               // obtočna črpalka ON           .
             swStat[0] = 1;                                              // -------------------------    .
             taktCount[1]++;                                         //                              .
             swVentil = 1;                                           //                              .
         }                                                         //                              .
         else {              // ........................ZIMA........................................                                                   //                           .      
             if (sensorValue[1] < minA) {
                 if (swPrvic[0] == 1) {                           // -------------------------    .
                     if (zap[1] == 3) {                          //                              .
                         if (zap[2] ==3) { 
                             digitalWrite(digiPin[13], LOW);                       // rekuperator ON               .
                             swPrvic[0] = 0;                           //                              .
                             swPrvic[1] = 1;                           //                              .
                             obtok(1);                                 // obtočna črpalka ON           .
                             dolociImpulz();                           // impulz dobi vrednost
                             korekcija();                                // korekcija temp.
                         }
                     }
                 }
                 else {
                 if (difer[2] != stdifer)
                     if (taktCount[0] > difer > 5)
                             
                     if (taktCount[0] ==  1) {                    // začetek takta
                         if (zap[2] == 3) { 
                             dolociImpulz();                      // določi trajanje impulza
                         }
                         else {
                             taktCount[0] = 0;
                             taktValue[2] = 0;
                         }             
                     }
                     korekcija();                                 // korekcija temp
                 }
             }
             else {          //.........................POLETJE ....................................
                 if (swPrvic[1] == 1) {
                     if  (zap[1] == 3) {                           //                              . 
                          swPrvic[1] = 0;                               //                              .
                          swPrvic[0] = 1;                               //                              .
                          taktCount[1] = 1;  
                          swStat[0] = 5;                                    //                              .
                          obtok(0);                                     // obtočna črpalka OFF          .  
                     }                                                  //    na HLADNA voda 
                 }
                 if (taktCount[1] < taktValue[1]) {                    // odpri mešalni ventil do konca
                     digitalWrite(digiPin[10], HIGH);                  // za HLAJENJE poleti
                     digitalWrite(digiPin[9], HIGH);                   // motorček ON
                     taktCount[1]++;                                       //                              .
                 }
                 else {
                      digitalWrite(digiPin[9], LOW);                   // motorček OFF
                 }     
                 if (sensorValue[1] > maxB) {                      //                              .
                     obtok(1);                                     // obtočna črpalka ON           .  
                     swStat[0] = 6;                                    //                              .
                 }                                                 //                              .     
                 else {                                            //                              .
                     obtok(0);                                     // obtočna črpalka OFF          .  
                     swStat[0] = 7;                                    //                              .
                 }                                                 //                              .                                                    //                              .
             }                                                     //                              .
         }
    
    // konec A.izmenjevalnik ..................................................................                       
   
 // obtočna črpalka
    if (swStat[0] == 1 or swStat[0] == 5 or swPrvic[0] == 0) 
        prikazCrpalka();
        
    // trenutne vrednosti temperature zraka 
    izpisValue();  
    
    // mešalni ventil

    prikazVentil(swVentil);
    
    // zaporni ventil
    izpisZaporniVentilOdprt();
       
    // izpis trenutnih vrednosti na monitor - Serial.print
    izpisMonitor(); 
    
    testSensor();
    index++;
   
 
 
          // B. izmenjevalnik ......................................
          if (sensorValue[1] < minB){                            
              digitalWrite(digiPin[12], HIGH);     // ogrevanje
              swStat[1] = 1;
          }
          else{
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
          
          
 //       izpisPovprecje60();          // izpis povprečje 60 zaporedno prebranih na monitor
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
unsigned long dolociImpulz() {
    taktValue[2] = 0;                                   // default vrednost impulza OFF 

 
    for (int k = 1; k < 10; k++)  {                     // določanje dolžine impulza    
         if (difer > k)                                                         
             taktValue[2] = k+1;
             
    }        
        Serial.print("Diferenca = ");
    Serial.print(difer);
    Serial.print(" impulz = ");
    Serial.print(taktValue[2]);
    Serial.print("\n");   
}
// --- konec dolociImpulz ---------------------------------------------------------------    

unsigned long korekcija() {
 // korekcija mešalnega ventila vsakič na začetku takta-zima 
    if (taktCount[0] < taktValue[2]) {   // korekcija ON 
        if(sensorValue[2] < tempZrak) {
           digitalWrite(digiPin[10], HIGH);      // dodaja TOPLO vodo pozimi
           swStat[0] = 3;                            // status delovanja
        }
        else {
            digitalWrite(digiPin[10], LOW);      // dodaja HLADNO vodo
            swStat[0] = 4;                           // status delovanja
        }    
        digitalWrite(digiPin[9], HIGH);          // motorček ON
        taktCount[1]++;                          // števec delovanja motorčka 
    }
    else {                                // korekcija OFF
       digitalWrite(digiPin[9], LOW);           // motorček OFF 
       taktCount[1] = 0;                        // števec delovanja motorčka 
       swStat[0] = 2;                               // status delovanja
       

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
   
   tft.println();
  
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
   narisiKvadrat(250, 44, 60, 188, YELLOW, 0);
   narisiKvadrat(251, 45, 58, 186, YELLOW, 0);
   narisiKvadrat(252, 46, 56, 184, YELLOW, 0);
   narisiKvadrat(254, 48, 20, 180, RED, 1);
   narisiKvadrat(286, 48, 20, 180, BLUE, 1); 
   narisiCrto(245, 87, 250, 87, YELLOW);   
   narisiCrto(245, 138, 250, 138, YELLOW);
   narisiCrto(245, 184, 250, 184, YELLOW);

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
  
     // zaporni ventil
//   izpisZaporniVentil();  
       
  tft.println();  
  return micros() - start;
}
// --- konec izpisTextdisplay()-----------------------------------

unsigned long izpisValue() {
   // mešalni ventil
      if (taktCount[0] > 0) {
          if (taktCount[0] == 1)
              narisiKvadrat(275, 48, 10, 180, BLACK, 1);
          int row = 229 - (taktCount[0] * 9);
          narisiKvadrat(275 ,row , 10, 7, CYAN, 1);
      }
      
   // I. izmenjevalnik
      
      for (int k = 44; k < 89; k=k+5) {
           if (swStat[0] == 5) {
               narisiKvadrat(137, k, 16, 3, CYAN, 1);
          }
          else {
             if (swStat[0] < 5) {
                 narisiKvadrat(137, k, 16, 3, RED, 1);
              }    
              else {
                   narisiKvadrat(137, k, 16, 3, BLUE, 1);
              }    
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

/*          swI[0]++;
          switch(swI[0]) {
              case 1:
                  if (swStat[0] == 5)
                      narisiKrog(145, 77, 4, CYAN, 1);
                  else {
                      if (swStat[0] < 5)
                          narisiKrog(145, 77, 4, RED, 1);
                      else
                          narisiKrog(145, 77, 4, BLUE, 1);
                  }        
              break; 
              case 2:
                  if (swStat[0] == 5)
                      narisiKrog(145, 62, 4, CYAN, 1);
                  else {
                      if (swStat[0] < 5)
                          narisiKrog(145, 62, 4, RED, 1);
                      else
                          narisiKrog(145, 62, 4, BLUE, 1);
                  }
              break;
              case 3:
                  if (swStat[0] == 5)
                      narisiKrog(145, 47, 4, CYAN, 1);
                  else {
                      if (swStat[0] < 5)
                          narisiKrog(145, 47, 4, RED, 1);
                      else
                          narisiKrog(145, 47, 4, BLUE, 1);
                  }        
              break; 
              case 4:
                  narisiKrog(145, 47, 4, BLACK, 1);
                  narisiKrog(145, 62, 4, BLACK, 1);
                  narisiKrog(145, 77, 4, BLACK, 1);
                  swI[0] = 0;
              break;               
         }    
*/
      
      
      if (sensorValue[1] == 99.9) {                                                     
          digitalWrite(digiPin[1], HIGH);
          narisiKrog(pCol[0], 64, 5, GREEN, 0);
      }
      else {        
          digitalWrite(digiPin[1], LOW);             
          narisiKrog(pCol[0], 64, 5, GREEN, 1);
      }

    
     tft.setTextColor(YELLOW); 
     for (int k = 0; k < (countSensor); k++)  { 
          if (sensorValue[k] != stValue[k]) {
              stValue[k] = sensorValue[k]; 
              narisiKvadrat(tCol[k], tRow[k], 60, 20, BLACK, 1); 
              tft.setCursor(tCol[k], tRow[k]);
              poravnavaDecimalk(sensorValue[k]);
              if (k == 0) tft.setTextColor(CYAN);
              else tft.setTextColor(YELLOW);            
              if (k < 5) tft.setTextSize(2);
                 else tft.setTextSize(1);
              tft.print(sensorValue[k],1);
          }    
     }
}
// --- konec izpisVrednostidisplay()-----------------------------------

unsigned long izpisTrenutneTemp() {
    
}      
// --- konec izpisTrenutneTemp() ----------------------------------------      
      
unsigned long poravnavaDecimalk(float param) {
//  if (param > -9.9) {
//      tft.print(" ");}
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
    countRota[0]++; 
    narisiKrog(80, 100, 18, BLACK, 1);
    switch (countRota[0]) {
        case 1:
            narisiKrog(80, 83, 3, RED, 1);      
            narisiKrog(68, 83, 3, RED, 1);
            narisiKrog(56, 83, 3, BLACK, 1);
            narisiKrog(104, 83, 3, BLACK, 1);
            narisiKrog(92, 83, 3, BLACK, 1);
            prikazVeter(2);
        break;
        case 2:
            narisiKrog(104, 83, 3, RED, 1);
            narisiKrog(92, 83, 3, RED, 1);
            narisiKrog(80, 83, 3, BLACK, 1);      
            narisiKrog(68, 83, 3, BLACK, 1);
            narisiKrog(116, 83, 3, BLACK, 1); 
            narisiKrog(128, 83, 3, BLACK, 1);
            prikazVeter(1);
        break; 
        case 3:
            narisiKrog(116, 83, 3, RED, 1); 
            narisiKrog(128, 83, 3, RED, 1);
            narisiKrog(92, 83, 3, BLACK, 1);
            narisiKrog(104, 83, 3, BLACK, 1);
            prikazVeter(2);
        break;         
        case 4:
            narisiKrog(56, 83, 3, RED, 1);
            narisiKrog(68, 83, 3, RED, 1);
            narisiKrog(116, 83, 3, BLACK, 1); 
            narisiKrog(128, 83, 3, BLACK, 1);
            prikazVeter(1);
            countRota[0] = 0; 
        break;
    }
//            narisiKrog(66, 83, 3, BLACK, 1);
//            narisiKrog(92, 97, 3, RED, 1); 
                
    //            narisiKrog(78, 83, 3, BLACK, 1);           
//            narisiKrog(114, 83, 3, RED, 1);

 //            narisiKrog(87, 90, 3, RED, 1); 
               
//           narisiKrog(78, 83, 3, RED, 1);    
    
    narisiKrog(80, 100, 3, YELLOW, 1);

}        
// --- konec prikazCrpalka() --------------------------------------------------------

unsigned long prikazVeter(uint8_t swVeter) {
    switch(swVeter) {
        case 1: 
            narisiCrto(64, 99, 96, 99, CYAN);
            narisiCrto(63, 100, 97, 100, CYAN);  
            narisiCrto(64, 101, 96, 101, CYAN); 
            narisiCrto(79, 84, 79, 116, CYAN);
            narisiCrto(80, 83, 80, 117, CYAN);
            narisiCrto(81, 84, 81, 116, CYAN);
        break;
        case 2: 
            narisiCrto(68, 89, 92, 111, CYAN);
            narisiCrto(68, 88, 92, 112, CYAN);
            narisiCrto(69, 88, 91, 112, CYAN);
            narisiCrto(68, 111, 91, 88, CYAN);
            narisiCrto(68, 112, 92, 88, CYAN);
            narisiCrto(69, 112, 92, 89, CYAN);
        break;
    }
}
// --- konec prikazVeter() --------------------------------------------------------

unsigned long prikazVentil(uint8_t swVentil) {
/*    if (swVentil != sVentil) {
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
            narisiTrikotnik(5, 225, 125, 225, 5, 198, BLUE, 1);
            narisiTrikotnik(0, 227, 128, 198, 0, 195, YELLOW, 0);
            izpisHladiMotor();
        break;
    }*/
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
                    narisiKrog(20, 221, 3, RED, 1);
                    narisiKrog(25, 221, 3, BLACK, 1);
                break;
                case 2: 
                    narisiKrog(21, 221, 3, RED, 1);
                break;
                case 3: 
                    narisiKrog(23, 221, 3, RED, 1);
                break;
                case 4: 
                    narisiKrog(24, 221, 3, RED, 1);
                break;
                case 5: 
                    narisiKrog(25, 221, 3, RED, 1);
                    narisiKrog(20, 221, 3, BLACK, 1);
                break;
                case 6: 
                    narisiKrog(21, 221, 3, BLACK, 1);                 
                break;
                case 7: 
                    narisiKrog(23, 221, 3, BLACK, 1);
                break;
                case 8: 
                    narisiKrog(24, 221, 3, BLACK, 1);
                    taktCount[2]= 0;                 
                break;                
              } 
}
// --- konec izpisZaporniVentil --------------------------------------------------------
// *** konec izpis na display *************************************************************

// *** prikaz na monitor računalnika ******************************************************
unsigned long izpisMonitor() {
    if (taktCount[0] == 1) {
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
    switch (swStat[0]) {
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
     if (taktCount[1] > 0)
         Serial.print("  + MIX");
     if (swObtok == 1)
         Serial.print("  + OBTOK");    
     Serial.print("  [");
     Serial.print(taktCount[0]);
     Serial.print(" ");
     Serial.print(taktValue[2]);
     Serial.print(" ");
     Serial.print("] ");
     Serial.print(swStat[0]);
     Serial.print(" ");
     Serial.print(taktCount[1]);
     Serial.print("  (");
     Serial.print(zap[1]);
     Serial.print(" "); 
     Serial.print(zap[2]);          
     Serial.print(" )");  

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

