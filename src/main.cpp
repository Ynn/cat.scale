#include <Arduino.h>
#include "HX711.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>


// The ESP8266 RTC memory is arranged into blocks of 4 bytes. The access methods read and write 4 bytes at a time,
// so the RTC data structure should be padded to a 4-byte multiple.
struct {
  uint32_t crc32;   // 4 bytes
  uint8_t channel;  // 1 byte,   5 in total
  uint8_t bssid[6]; // 6 bytes, 11 in total
  uint8_t padding;  // 1 byte,  12 in total
} rtcData;


#define wifi_ssid "SUSAN_nomap"
#define wifi_password "celui qui ne pense pas a moi est un egoiste"

#define mqtt_server "54.154.107.222"
#define mqtt_user "oipayvjr"  
#define mqtt_password "57zb4p0F7R7n"
#define mqtt_port 16798
#define cat_topic "cat/bowl"

// Weight(gr) = A*(x+B)
#define A 0.00257916
#define B 18614

IPAddress ip( 192, 168, 1, 201 );
IPAddress gateway( 192, 168, 1, 1 );
IPAddress subnet( 255, 255, 255, 0 );

WiFiClient espClient;
PubSubClient client(espClient);


// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = D4;
const int LOADCELL_SCK_PIN = D3;

HX711 scale;


//Connexion au réseau WiFi
void setup_wifi() {
 
  // Try to read WiFi settings from RTC memory
  bool rtcValid = false;
  if( ESP.rtcUserMemoryRead( 0, (uint32_t*)&rtcData, sizeof( rtcData ) ) ) {
    // Calculate the CRC of what we just read from RTC memory, but skip the first 4 bytes as that's the checksum itself.
    uint32_t crc = calculateCRC32( ((uint8_t*)&rtcData) + 4, sizeof( rtcData ) - 4 );
    if( crc == rtcData.crc32 ) {
      rtcValid = true;
    }
  }
 
  delay(10);
  // Serial.println();
  // Serial.print("Connect to");
  // Serial.println(wifi_ssid);

  WiFi.forceSleepWake();
  delay( 1 );


  // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
  WiFi.persistent( false );


  // Bring up the WiFi connection
  WiFi.mode( WIFI_STA );
  WiFi.config( ip, gateway, subnet );

  if( rtcValid ) {
    // The RTC data was good, make a quick connection
    WiFi.begin( WLAN_SSID, WLAN_PASSWD, rtcData.channel, rtcData.ap_mac, true );
  }
  else {
    // The RTC data was not valid, so make a regular connection
    WiFi.begin(wifi_ssid, wifi_password);
  }


  int retries = 0;
  int wifiStatus = WiFi.status();
  while( wifiStatus != WL_CONNECTED ) {
    retries++;
    if( retries == 100 ) {
      // Quick connect is not working, reset WiFi and try regular connection
      WiFi.disconnect();
      delay( 10 );
      WiFi.forceSleepBegin();
      delay( 10 );
      WiFi.forceSleepWake();
      delay( 10 );
      WiFi.begin( WLAN_SSID, WLAN_PASSWD );
    }
    if( retries == 600 ) {
      // Giving up after 30 seconds and going back to sleep
      WiFi.disconnect( true );
      delay( 1 );
      WiFi.mode( WIFI_OFF );
      ESP.deepSleep( SLEEPTIME, WAKE_RF_DISABLED );
      return; // Not expecting this to be called, the previous call will never return.
    }
    delay( 50 );
    wifiStatus = WiFi.status();
  }


  // Serial.println("");
  // Serial.println("Wifi ok ");
  // Serial.print("=> IP : ");
  // Serial.print(WiFi.localIP());
}

//Reconnexion
void reconnect() {
  //Boucle jusqu'à obtenur une reconnexion
  while (!client.connected()) {
    // Serial.print("MQTT...");
    if (client.connect("Cat", mqtt_user, mqtt_password)) {
      // Serial.println("OK");
    } else {
      // Serial.print("KO, error : ");
      // Serial.print(client.state());
      // Serial.println(" wait 5s...");
      delay(500);
    }
  }
}



/**
 * return the weight or -1 if the mesure failed
 ***/
long mesure(){
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.power_up();

  long adjusted = 100000;

  //Takes the minimum of 3 mesures in 3s.
  for(int i =0; i< 3; i++){
    if (scale.is_ready()) {
      long reading = scale.read();
      long new_adjusted = (reading + B)*A;
      if(new_adjusted < adjusted){
        adjusted = new_adjusted;
      }
      delay(1000);
    }
  }
  scale.power_down();			        // put the ADC in sleep mode

  if(adjusted < 0){
    adjusted = 0;
  }else{
    return -1;
  }

  return adjusted;
}


void sendAndSleep(long mesure){

  // Connect to WIFI
  setup_wifi();

  //Connect to MQTT
  client.setServer(mqtt_server, mqtt_port);   

  // TEST MQTT connection
  client.loop();
  while (!client.connected()) 
  {
    reconnect();
  }

  client.publish(cat_topic, String(mesure).c_str(), true); 

}


void setup() {
  // Serial.begin(MONITOR_SPEED);
  // Serial.print("Starting: ");

  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  delay( 1 );

  long mesure = mesure();
  if(mesure > -1){
    sendAndSleep(mesure);
    //Sleep for 10 minutes :
    ESP.deepSleepInstant(10*60*1000000, RF_DEFAULT);
  }else{
    //retry in one minute
    ESP.deepSleepInstant(60*1000000, RF_DISABLED);
  }
}

void loop() {


}