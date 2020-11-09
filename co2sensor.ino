#include <Wire.h>    // I2C library
#include <ccs811.h>
#Test 
/*
 * IIC address default 0x5A, the address becomes 0x5B if the ADDR_SEL is soldered.
 */
//CCS811 sensor(&Wire, /*IIC_ADDRESS=*/0x5A);
CCS811 sensor(23); // nWAKE on 23;
const int ledPin = 19;

void setup(void)
{   
    Wire.begin(); // Enable I2C

    pinMode (ledPin, OUTPUT);
    Serial.begin(115200);
    Serial.println("Sensor begin");
    bool ok=sensor.begin();
    if (!ok) Serial.println("setup: CCS811 begin FAILED");
    
    ok=sensor.start(CCS811_MODE_1SEC);
    if (!ok)  Serial.println("setup: CCS811 start FAILED");
}

void loop() {
  uint16_t eco2, etvoc, errstat, raw;
  sensor.read(&eco2,&etvoc,&errstat,&raw); 
  
  if( errstat==CCS811_ERRSTAT_OK ) {
    Serial.print("CCS811: ");
    Serial.print("eco2=");  Serial.print(eco2);     Serial.print(" ppm  ");
    Serial.print("etvoc="); Serial.print(etvoc);    Serial.print(" ppb  ");
    Serial.println();

    if (eco2 > 500){
      digitalWrite(ledPin, HIGH);
            //Serial.print ("an");
    } else{
      digitalWrite(ledPin, LOW);
            //Serial.print ("aus");
    }
    
  } else if( errstat==CCS811_ERRSTAT_OK_NODATA ) {
    Serial.println("CCS811: waiting for (new) data");
  } else if( errstat & CCS811_ERRSTAT_I2CFAIL ) { 
    Serial.println("CCS811: I2C error");
  } else {
    Serial.print("CCS811: errstat="); Serial.print(errstat,HEX); 
    Serial.print("="); Serial.println( sensor.errstat_str(errstat) );
  }
  
  // Wait
  delay(1000); 

/*  int co2value; 
  delay(1000);
    if(sensor.checkDataReady() == true){
        Serial.print("CO2: ");
        co2value=sensor.getCO2PPM();
        Serial.println(co2value);
        Serial.print("ppm, TVOC: ");
        Serial.print(sensor.getTVOCPPB());
        Serial.println("ppb");
        if (co2value > 500){
            digitalWrite (ledPin, HIGH);
            //Serial.print ("an");
        }else{
            digitalWrite (ledPin, LOW);
            //Serial.print ("aus");
        }
    } else {
        Serial.println("Data is not ready!");
    }
    /*!
     * @brief Set baseline
     * @param get from getBaseline.ino
     */
/*    sensor.writeBaseLine(0x847B);
    //delay cannot be less than measurement cycle
    //delay(1000);
*/
    
}
