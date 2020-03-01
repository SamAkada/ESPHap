#include <M5StickC.h>
#include "DHT12.h"
#include <Wire.h> //The DHT12 uses I2C comunication.
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>

const char* ssid     = "myssid";
const char* password = "mypassword";

const int identity_led=10;
const int led_gpio = 10;

extern "C"{
#include "homeintegration.h"
}
homekit_service_t* hapservice={0};
String pair_file_name="/pair.dat";

void Format(){
  // Next lines have to be done ONLY ONCE!!!!!When SPIFFS is formatted ONCE you can comment these lines out!!
  Serial.println("Please wait 30 secs for SPIFFS to be formatted");
  SPIFFS.format();
  Serial.println("Spiffs formatted");
}

void init_hap_storage(){

  Serial.print("init_hap_storage");
  if (!SPIFFS.begin(true)) {
       Serial.print("SPIFFS Mount failed");
    }
    
  //HomeKitアクセサリーが認識しない場合は、SPIFFSをFormatしてください。  
  //Format();
  
    File fsDAT=SPIFFS.open(pair_file_name, "r");
  if(!fsDAT){
      Serial.println("Failed to read pair.dat");
      return;
  }
  int size=hap_get_storage_size_ex();
  char* buf=new char[size];
  memset(buf,0xff,size);
  int readed=fsDAT.readBytes(buf,size);
  Serial.print("Readed bytes ->");
  Serial.println(readed);
  hap_init_storage_ex(buf,size);
  fsDAT.close();
  delete []buf;
}

void storage_changed(char * szstorage,int size){
  SPIFFS.remove(pair_file_name);
  File fsDAT=SPIFFS.open(pair_file_name, "w+");
  if(!fsDAT){
    Serial.println("Failed to open pair.dat");
    return;
  }
  fsDAT.write((uint8_t*)szstorage, size);

  fsDAT.close();
}

DHT12 dht12;
homekit_service_t* temperature;
homekit_service_t* humidity;
void setup() {
    M5.begin();
    Wire.begin(32, 33, 100000);
    M5.Lcd.setRotation(3);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

   pinMode(led_gpio,OUTPUT);
    hap_set_identity_gpio(identity_led);

    init_hap_storage();
    set_callback_storage_change(storage_changed);

    hap_setbase_accessorytype(homekit_accessory_category_thermostat);
    hap_initbase_accessory_service("Sensor","A-TEAM","0","HAP-ENV","1.0");

    //hapservice= hap_add_lightbulb_service("Led",led_callback,(void*)&led_gpio);
    temperature = hap_add_temperature_service("Temperature Sensor");
    humidity = hap_add_humidity_service("Humidity Sensor");
    //hapservice = hap_add_temp_hum_as_accessory(homekit_accessory_category_thermostat,"Sensor",&temperature,&humidity);
        
   //and finally init HAP
    hap_init_homekit_server();

}

void loop() {
 
    float tmp = dht12.readTemperature();
    float hum = dht12.readHumidity();

    M5.Lcd.setCursor(0, 6, 2);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("Temp: %2.1fC\n", tmp);
    M5.Lcd.printf("Humi: %2.0f%%\n", hum);
     homekit_characteristic_t * ch= homekit_service_characteristic_by_type(temperature, HOMEKIT_CHARACTERISTIC_CURRENT_TEMPERATURE);
     ch->value.float_value=tmp;
     homekit_characteristic_notify(ch,ch->value);
     homekit_characteristic_t * ch2= homekit_service_characteristic_by_type(humidity, HOMEKIT_CHARACTERISTIC_CURRENT_RELATIVE_HUMIDITY);
     ch->value.float_value=hum;
     homekit_characteristic_notify(ch2,ch2->value);
      
    delay(500);
}
