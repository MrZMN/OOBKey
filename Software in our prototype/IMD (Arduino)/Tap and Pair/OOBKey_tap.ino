#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <ArduinoBLE.h>
#include <SPI.h>
#include <SD.h>

#define SDMOUNT   // if SD card is mounted
//#define serial    //whether PC serial port opens

/*
 * BNO055
 */
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);
sensors_event_t accelerometerData;

/*
 * IO
 */
File myFile;
String Filename = "tap.dat";

/*
 * Datalog Control
 * 0 - 1: init datalog
 * 1 - 1: datalogging
 * 1 - 0: deinit datalog
 * 0 - 0: idle
 */
int flag_current = 0;
int flag_last = 0;

/*
 * Debugging
 */
long int t1, t2;            // for code execution time measurement
uint16_t sample_idx = 0;    // to calculate actual ODR

/*
 * BLE
 */
BLEService OOBService("19B10000-E8F2-537E-4F6C-D104768A1214");
// characteristic allows remote device to read and write
BLEUnsignedCharCharacteristic OOBCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

void setup(void) {

  /*
   * LED
   */
  pinMode(22, OUTPUT);  // red LED
  pinMode(24, OUTPUT);  // blue LED
  pinMode(23, OUTPUT);  // green LED
  LED(22, false);
  LED(24, false);
  LED(23, false);

  Serial.begin(115200);
  #ifdef serial
    while (!Serial) delay(10);  
  #endif   

  /*
   * BLE config
   */
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }
  // set the local name peripheral advertises
  BLE.setLocalName("OOBKey");
  // set the UUID for the service this peripheral advertises
  BLE.setAdvertisedService(OOBService);
  // add the characteristic to the service
  OOBService.addCharacteristic(OOBCharacteristic);
  // add service
  BLE.addService(OOBService);
  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
  // assign event handlers for characteristic
  OOBCharacteristic.setEventHandler(BLEWritten, OOBCharacteristicWritten);
  // set an initial value for the characteristic
  OOBCharacteristic.setValue(0);
  // start advertising
  BLE.advertise();

  /*
   * IMU init
   */
  if (!bno.begin(OPERATION_MODE_IMUPLUS))
  {
    Serial.print("No BNO055 detected");
    while (1);
  }
  bno.setAxisRemap();   // the remapped axis follows datasheet page 24 & 25, where the sensor stands up from P1
  bno.setAxisSign();

  /*
   * SD card init
   */
  #ifdef SDMOUNT
    
    if (!SD.begin(4)) {   // init SD card
      Serial.println("Card failed, or not present");
      LED(22, true);  // red LED on
      while (1);
    }
  
    //Remove the former file?
//    SD.remove(Filename);
  #endif  

  delay(1000);
}

void loop() {
  // poll for Bluetooth® Low Energy events
  BLE.poll();

  // init datalog
  if (flag_last == 0 && flag_current == 1) {
    
    #ifdef SDMOUNT
    myFile = SD.open(Filename, FILE_WRITE);   // Open file
    #endif

    flag_last = 1;
    t1 = millis();
    LED(23, true);  // green LED on
  }
  // datalogging
  else if (flag_last == 1 && flag_current == 1) {

    bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER); 
    sample_idx ++; 

    #ifdef SDMOUNT
    myFile.println(String(accelerometerData.acceleration.x) + ", " + String(accelerometerData.acceleration.y) + ", " + String(accelerometerData.acceleration.z));
    #endif     

//    delay(7);  // ODR ≈ 102 Hz
  } 
  // deinit datalog
  else if (flag_last == 1 && flag_current == 0) {
    
    #ifdef SDMOUNT
    myFile.println("END");  // a 'line break'
    myFile.close();         // Close file
    #endif

    flag_last = 0;
    t2 = millis();
    LED(23, false);  // green LED off

//    Serial.println("Number of samples: " + String(sample_idx));
    Serial.println("Actual ODR: " + String(int(sample_idx*1000/(t2-t1))) + " Hz");
    
    sample_idx = 0;
  }
  else {
    // idle, do nothing
  }
}

/*
 * BLE connected
 */
void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  LED(24, true);
}

/* 
 * BLE disconnected
 */
void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
  LED(24, false);
}

/*
 * BLE write
 */
void OOBCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {
  // central wrote new value to characteristic
  if (OOBCharacteristic.value() == 255) {
    Serial.println("Phone: start datalog");
    flag_current = 1;
  } else if (OOBCharacteristic.value() == 254) {
    Serial.println("Phone: stop datalog");
    flag_current = 0;
  }
}

/*
 * LED glow
 * LED_id == 22, red
 * LED_id == 24, blue
 * LED_id == 23, green
 */
void LED(int LED_id, boolean ledswitch) {
  if(ledswitch == true) {
    digitalWrite(LED_id, LOW);  // LED on
  }else {
    digitalWrite(LED_id, HIGH);
  }
}
