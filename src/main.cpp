#include <Arduino.h>
#include "HX711.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

#define wifi_ssid "SUSAN_nomap"
#define wifi_password "celui qui ne pense pas a moi est un egoiste"

#define mqtt_server "hairdresser.cloudmqtt.com"
#define mqtt_user "oipayvjr"  //s'il a été configuré sur Mosquitto
#define mqtt_password "57zb4p0F7R7n" //idem
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
  Serial.println();
  Serial.print("Connect to");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Wifi ok ");
  Serial.print("=> IP : ");
  Serial.print(WiFi.localIP());
}

//Reconnexion
void reconnect() {
  //Boucle jusqu'à obtenur une reconnexion
  while (!client.connected()) {
    Serial.print("MQTT...");
    if (client.connect("Cat", mqtt_user, mqtt_password)) {
      Serial.println("OK");
    } else {
      Serial.print("KO, error : ");
      Serial.print(client.state());
      Serial.println(" wait 5s...");
      delay(5000);
    }
  }
}

void sendAndSleep(){
  for (int i = 0; i <= 3; i++) {

    client.loop();
    while (!client.connected()) 
    {
      reconnect();
    }

    if (scale.is_ready()) {
      long reading = scale.read();
      long adjusted = (reading + B)*A;
      if(adjusted < 0){
        adjusted = 0;
      }
      Serial.println(adjusted);
      client.publish(cat_topic, String(adjusted).c_str(), true); 
      delay(1000);
    }
  }
  scale.power_down();			        // put the ADC in sleep mode

  //Sleep for 10 minutes :
  ESP.deepSleep(10*60* 1000000, RF_DEFAULT);

}

void setup() {
  Serial.begin(MONITOR_SPEED);
  Serial.print("Starting: ");
  setup_wifi();      
  client.setServer(mqtt_server, mqtt_port);   
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.power_up();			        // put the ADC in sleep mode
  sendAndSleep();
}



void loop() {


}