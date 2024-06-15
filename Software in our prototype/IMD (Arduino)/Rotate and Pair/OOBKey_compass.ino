#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <ArduinoBLE.h>
#include <SPI.h>
#include <SD.h>

#define SDMOUNT   //whether SD card is mounted
//#define serial    //whether PC serial port opens

/*
 * BNO055
 */
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);
sensors_event_t orientationData;

/*
 * IO
 */
File myFile;
String Filename = "compass.dat";
  
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
//    pinMode(4, OUTPUT);
    pinMode(10, OUTPUT);
    // init SD card
    if (!SD.begin(4)) {
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
//  readYaw();  // for testing continuous orientation data
}

void readYaw() {

  bno.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);

  Serial.println(orientationData.orientation.x);

  /*
   * TODO: add timestamp
   */
  #ifdef SDMOUNT
    // Open file
    myFile = SD.open(Filename, FILE_WRITE);
    if (myFile) {
      // write compass heading into sd
      myFile.println(orientationData.orientation.x);  
      // Close file
      myFile.close();      
    }else {
      LED(22, true);  // red LED on
    }
  #endif

//  delay(50);
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
  Serial.print("Characteristic event, written: ");
  Serial.println(OOBCharacteristic.value());

  if (OOBCharacteristic.value() == 255) {
    LED(23, true);  // green LED on
    readYaw();
    LED(23, false);  // green LED off
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
