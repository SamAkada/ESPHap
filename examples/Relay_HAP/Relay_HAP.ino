#include <M5StickC.h>

#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>

//適宜WiFi設定を変更してください。
const char* ssid     = "myssid";
const char* password = "mypassword";

//M5StickCのLEDピンに割り当て
const int identity_led=10;
//M5StickCのGroveピン(Relay Unit)に割り当て
const int relay_gpio = 32;

extern "C"{
#include "homeintegration.h"
}
homekit_service_t* hapservice={0};
String pair_file_name="/pair.dat";

void init_hap_storage(){
  Serial.print("init_hap_storage");
 
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

void set_relay(bool val){
   Serial.println("set_relay"); 
   digitalWrite(relay_gpio, val?HIGH:LOW);
   digitalWrite(identity_led, val?LOW:HIGH);
  
  if(hapservice){
    Serial.println("notify hap"); 
    homekit_characteristic_t * ch= homekit_service_characteristic_by_type(hapservice, HOMEKIT_CHARACTERISTIC_ON);
    if(ch){
        Serial.println("found characteristic"); 
        if(ch->value.bool_value!=val){  //状態が変化した時のみ通知する
          ch->value.bool_value=val;
          //リレーの状態を通知する
          homekit_characteristic_notify(ch,ch->value);
        }
    }
  }
}

void relay_callback(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
    Serial.println("relay_callback");
    set_relay(ch->value.bool_value);
}

void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.setTextFont(4);
  M5.Lcd.setTextColor(YELLOW, BLACK);
  M5.Lcd.printf("HomeKit");
  
  pinMode(identity_led, OUTPUT);
  pinMode(relay_gpio, OUTPUT);
  
    delay(10);

    // We start by connecting to a WiFi network
     if (!SPIFFS.begin(true)) {
       Serial.print("SPIFFS Mount failed");
     }
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        M5.Lcd.printf(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    pinMode(relay_gpio,OUTPUT);
    //ペアリング時のLED点滅GPIO
    hap_set_identity_gpio(identity_led);
    
    //内部ストレージに情報を書き込み\pair.dat
    //HomeKitアクセラリーが見えなくなった場合は、
    //esptool.pyにて、erase_flashしてください。
    init_hap_storage();
    set_callback_storage_change(storage_changed);

    //HomeKitのカテゴリをランプ照明にする
    hap_setbase_accessorytype(homekit_accessory_category_lightbulb);
    //プロパティ情報を設定する
    hap_initbase_accessory_service("Led","A-TEAM","123456","Relay","1.0");

    //ランプ照明のサービスにコールバック関数を設定する
    hapservice= hap_add_lightbulb_service("Led",relay_callback,(void*)&relay_gpio);

    //最後にHAPの初期化
    hap_init_homekit_server();
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(10);
}
