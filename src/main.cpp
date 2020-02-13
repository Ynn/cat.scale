#include <Arduino.h>
#include "HX711.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

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

WiFiClient espClient;
PubSubClient client(espClient);


// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = D4;
const int LOADCELL_SCK_PIN = D3;

HX711 scale;


//Connexion au réseau WiFi
void setup_wifi() {
  delay(10);
  // Serial.println();
  // Serial.print("Connect to");
  // Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
  }

  // Serial.println("");
  // Serial.println("Wifi ok ");
  // Serial.print("=> IP : ");
  Serial.print(WiFi.localIP());
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
      delay(5000);
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
    ESP.deepSleepInstant(60*1000000, RF_DEFAULT);
  }
}

void loop() {


}