#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
//#include <SoftwareSerial.h>                                // Remove if using HardwareSerial or Arduino package without SoftwareSerial support

// Display Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Original

// Sensor
#include "MHZ19.h"                                        
#define RX_PIN 25                                          // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 26                                          // Tx pin which the MHZ19 Rx pin is attached to
#define BAUDRATE 9600                                      // Device to MH-Z19 Serial baudrate (should not be changed)

// Sound
#define PWM_PIN 12 // Piepser

// RGB-LED
#define PWM_PIN_rd 16 // GPIO 16
#define PWM_PIN_gn 17 // GPIO 17
#define PWM_PIN_bl 27 // GPIO 27
#define RD 0 // Kanäle
#define GN 1
#define BL 2

// Button
#define BTN T4 // GPIO 13 bzw. Touch 4
int sw=0, act=100; // sw: 1 Ein 0 Aus 

// Programmeinstellungen
#define THRESHOLD 1500 // Schwellwert Alarm 
#define TempK 4// Korrekturwert

int freq=200, channel_rd=0, channel_gn=1, channel_bl=2, resolution=8; // Parameter für FarbeLED
int dutyCycle;
int grenze=350; //für Farbübergang mit rot
float rk=255, gk=255, bk=150; //Weißabgleich
int clr[3]; 
int r=0, g=255, b=0; //grün

int freq_ton=2000, chan_ton=3, res_ton=8, duCy_ton=0; // Parameter für "duty cycle"
unsigned char runde=0;
const int BUF=7; // Anzahl gespeicherter Messwerte für AVG-Bildung

MHZ19 myMHZ19;                                             // Constructor for library
int CO2=400;
int8_t Temp=0;
float lval[BUF]={0}, avgCO2=0;

//SoftwareSerial mySerial(RX_PIN, TX_PIN);                   // (Uno example) create device to MH-Z19 serial
HardwareSerial mySerial(1);                              // (ESP32 Example) create device to MH-Z19 serial

unsigned long getDataTimer = 0;

void setup()
{
    Serial.begin(115200);                                     // Device to serial monitor feedback

    //mySerial.begin(BAUDRATE);                               // (Uno example) device to MH-Z19 serial start   
    mySerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN); // (ESP32 Example) device to MH-Z19 serial start   
    myMHZ19.begin(mySerial);                                // *Serial(Stream) refence must be passed to library begin(). 
    myMHZ19.autoCalibration(true);                              // Turn auto calibration ON (OFF autoCalibration(false))

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64 -> 3D funktiomniert nicht !! jpw
      Serial.println(F("SSD1306 allocation failed"));
    }

    display.display(); // Show initial display buffer contents on the screen --
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(2);             // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.println(F("IPL ..."));
    display.display();
    // Piepser
    ledcSetup(chan_ton, freq_ton, res_ton);
    ledcAttachPin(PWM_PIN, chan_ton);
    
    // Alarm-LED
    //pinMode(LED, OUTPUT);

    rk/=255; gk/=255; bk/=255;
    clr[RD]=rk*r; clr[GN]=gk*g; clr[BL]=bk*b;
    
    ledcSetup(channel_rd, freq, resolution);
    ledcAttachPin(PWM_PIN_rd, channel_rd);
    ledcWriteTone(channel_rd, 200); // Frequenz
    ledcWrite(channel_rd, 0);
 
    ledcSetup(channel_gn, freq, resolution);
    ledcAttachPin(PWM_PIN_gn, channel_gn);
    ledcWriteTone(channel_gn, 200); // Frequenz
    ledcWrite(channel_gn, 0);
  
    ledcSetup(channel_bl, freq, resolution);
    ledcAttachPin(PWM_PIN_bl, channel_bl);
    ledcWriteTone(channel_bl, 200); // Frequenz
    ledcWrite(channel_bl, 0);

    CO2=myMHZ19.getCO2();

    int ct=0;
    while ((CO2<400 || CO2>3000) || (CO2>THRESHOLD && ct<20)) {
      Serial.println(CO2);    
      display.print(F("."));display.display();
      delay(2000);
      CO2=myMHZ19.getCO2();
      ct++;
    }
    Serial.println(CO2);  
    
    display.println(F("DONE."));
    display.display();
    delay(2000);
    
    

    
}

void loop()
{   
    // Auslesen des Sensors, Mittelwerte der BUF-letzten Messwerte berechnen
    if (millis() - getDataTimer >= 2000)
    {   
        CO2 = myMHZ19.getCO2();                             // Request CO2 (as ppm)
        //Serial.print("CO2 (ppm): ");                      //Serial.println(CO2);                                
        
        Temp = myMHZ19.getTemperature()-TempK;              // Request Temperature (as Celsius)
       // Serial.print("Temperature (C): ");                 
       // Serial.println(Temp);                               
        getDataTimer = millis();
    
        lval[runde%BUF]=CO2; 
        avgCO2=0; 
        for (int i=0; i<BUF; i++){
          avgCO2+=lval[i];
        };
        avgCO2=avgCO2/BUF;
        //avgCO2=1600; //Alarmtest
        runde++;
        
        //Serial.print(runde%BUF);Serial.print(" : ");Serial.print(avgCO2);Serial.print(" : ");Serial.print(CO2);Serial.print("\n");
     }

    btn_switch(); // Button auslesen, umschalten
    
    //Serial.print(act);Serial.print(" : ");Serial.println(sw);
    
    // Auswerten und Anzeigen    
        display.clearDisplay();
        dsp_thres();// eingestellte Alarmgrenze ausgeben
        dsp_avg();// aktuellen Mittelwert der letzten BUF-Messungen ausgeben        
        dsp_co2();// aktuelle CO2-Konzentration ausgeben
        dsp_temp();// aktuelle Temperatur ausgeben
        dsp_snd_state();// Anzeige Sound on/off
        display.display();

        //avgCO2=499; //Farbtest
        grenze=500;
        if (avgCO2<grenze){
          r=(avgCO2-350)/(float)(grenze-350)*255;g=255;b=0; // Start bei gn100% und rd0..100%
        }
        else {
          //r=255; g=190*grenze/(float)avgCO2;
          r=255; g=240*(THRESHOLD-avgCO2)/(float)(THRESHOLD-grenze); //start bei rd⁰100% und gn90%..0% 
        };
        clr[RD]=rk*r; clr[GN]=gk*g; clr[BL]=bk*b;  

        if (avgCO2<THRESHOLD){
          ledcWrite(channel_rd, clr[RD]); ledcWrite(channel_gn, clr[GN]); ledcWrite(channel_bl, clr[BL]);
        }
        else {
          for(int j=0; j<3; j++){
            dsp_blink();

            for(int k=0; k<3; k++){
              //ledcWrite(channel_rd, clr[RD]); ledcWrite(channel_gn, clr[GN]); ledcWrite(channel_bl, clr[BL]);
              ledcWrite(channel_rd, 255); ledcWrite(channel_gn, 0); ledcWrite(channel_bl, 0);
              delay(100);
              //ledcWrite(channel_rd, 0); ledcWrite(channel_gn, 0); ledcWrite(channel_bl, 0);
              //delay(50);
              //ledcWrite(channel_rd, clr[RD]); ledcWrite(channel_gn, clr[GN]); ledcWrite(channel_bl, clr[BL]);
              ledcWrite(channel_rd, 0); ledcWrite(channel_gn, 0); ledcWrite(channel_bl, 255);
              delay(100);
              ledcWrite(channel_rd, 0); ledcWrite(channel_gn, 0); ledcWrite(channel_bl, 0);
              //delay(50);  
            };  
            if (sw==1){
              for(int l=0; l<2; l++){
                //Serial.println(dutyCycle);
                ledcWrite(chan_ton, duCy_ton);
                freq_ton=1046; //Serial.println(freq);
                ledcWriteTone(chan_ton, freq_ton);
                delay(500);
                duCy_ton=0;
                ledcWrite(chan_ton, duCy_ton);
                delay(150);
                /*
                duCy_ton=255;
                ledcWrite(chan_ton, duCy_ton);
                freq_ton=1318; //Serial.println(freq);
                ledcWriteTone(chan_ton, freq_ton);
                delay(100);
                duCy_ton=0;
                ledcWrite(chan_ton, duCy_ton);
                delay(150);
                */
                duCy_ton=255;
                ledcWrite(chan_ton, duCy_ton);
                freq_ton=1567; //Serial.println(freq);
                ledcWriteTone(chan_ton, freq_ton);
                delay(500);
                duCy_ton=0;
                ledcWrite(chan_ton, duCy_ton);
                delay(150);
              }
            }
            /*
            //Serial.println(dutyCycle);
            ledcWrite(chan_ton, duCy_ton);
            freq_ton=2000; //Serial.println(freq);
            ledcWriteTone(chan_ton, freq_ton);
            delay(250);
        
            freq_ton=1318; //Serial.println(freq);
            ledcWriteTone(chan_ton, freq_ton);
            delay(250);

            freq_ton=1567; //Serial.println(freq);
            ledcWriteTone(chan_ton, freq_ton);
            delay(250);

            freq_ton=1318; //Serial.println(freq);
            ledcWriteTone(chan_ton, freq_ton);
            delay(250);

            freq_ton=1046; //Serial.println(freq);
            ledcWriteTone(chan_ton, freq_ton);
            delay(250);
    
            freq_ton=1046; //Serial.println(freq);
            ledcWriteTone(chan_ton, freq_ton);
            delay(250);

            freq_ton=1318; //Serial.println(freq);
            ledcWriteTone(chan_ton, freq_ton);
            delay(250);
            */
            duCy_ton=0;
            //Serial.println(dutyCycle);
            ledcWrite(chan_ton, duCy_ton);
      
            btn_switch();  
            dsp_snd_state();         
          }
         }
}

void btn_switch(void){
    act=touchRead(BTN);
    delay(500);
    if (act==0){
      switch (sw){
        case 0: sw=1; break;
        case 1: sw=0; break;
      };
    };
}

void dsp_thres(void){
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(6,0);             // Start at top-left corner
  display.print(F("Treshold  : "));
  //display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.print(THRESHOLD);
  display.println(F(" ppm"));
  //display.display();
}

void dsp_avg(void){
        display.setCursor(6,8); 
        display.print(F("CO2-AVG("));display.print(BUF);display.print(F("): "));
        //display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
        if (avgCO2<100)
          display.print(F("  "));
        else if (avgCO2<1000)
               display.print(F(" "));
        display.print((int)avgCO2);
        display.println(F(" ppm"));
}

void dsp_snd_state(void){
        display.setCursor(38,57);
        display.setTextSize(1);             // Draw 2X-scale text
        display.print(F("Sound "));
        if (sw==1)        
          display.print(F("on"));
        else
          display.print(F("off"));  
}

void dsp_co2(void){
        display.setCursor(0,19);
        display.setTextSize(2);             // Draw 2X-scale text
        display.setTextColor(SSD1306_WHITE);
        display.print(F("CO2 :"));
        if (CO2<1000)
               display.print(F(" "));
        display.print(CO2);
        display.setTextSize(1);display.print(F("ppm"));
}

void dsp_temp(void){
        display.setCursor(0,38);
        display.setTextSize(2);             // Draw 2X-scale text
        display.print(F("Temp:  "));display.print(Temp);
        display.setTextSize(1);display.print(F("C"));
}

void dsp_blink(void){
         display.invertDisplay(true);
         delay(100);
         display.invertDisplay(false);
         delay(100);
         display.invertDisplay(true);
         delay(100);
         display.invertDisplay(false);
         delay(100);
}
