#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>


// Wifi

#define WLAN_SSID "reallyslowwifi"
#define WLAN_PASS "88888888"


// MQTT Broker
const char *mqtt_broker = "192.168.1.2";
const char *temp_topic = "plant1/temperature";
const char *humidity_topic = "plant1/humidity";
const char *soil_moisture_topic = "plant1/soil_moisture";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);


// DHT22 temperature sensor 

#define DHTPIN 0     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);


// soil moisture sensor

const int AirValue = 3620;   //you need to replace this value with Value_1
const int WaterValue = 1680;  //you need to replace this value with Value_2
const int SensorPin = 35;
int soilMoistureValue = 1;
int soilmoisturepercent=0;




void setup() {
  Serial.begin(9600);
  dht.begin();

  // connecting to a WiFi network
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.println(".");
  }
  Serial.println("Connected to the WiFi network");
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
     String client_id = "esp32-client-";
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
         Serial.println("Connect to MQTT broker");
     } else {
         Serial.print("failed with state ");
         Serial.print(client.state());
         delay(2000);
     }
  }

  // initialize moisture sensor pin maybe?
  soilMoistureValue = analogRead(SensorPin);
}


void loop() {
  // Wait 10 seconds between measurements.
  delay(5000);


  // ############# TEMP/HUMIDITY SENSOR ############# 

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  // pub to mqtt
  char msgBuffer[20];
  client.publish(temp_topic, dtostrf(f, 5, 2, msgBuffer));
  client.publish(humidity_topic, dtostrf(h, 5, 2, msgBuffer));

  // console logging
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));


  // ######## soil moisture sensor ########

  soilMoistureValue = analogRead(SensorPin);  //put Sensor insert into soil
  soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);

  // print reading to console
  Serial.print("Raw moisture value: ");
  Serial.print(soilMoistureValue);
  Serial.println("%");

  Serial.print("Mapped moisture value: ");
  Serial.print(soilmoisturepercent);
  Serial.println("%");


  char cstr[16];
  itoa(soilmoisturepercent, cstr, 10);


  client.publish(soil_moisture_topic, cstr);
}


// for receving messages via MQTT
void callback(char *topic, byte *payload, unsigned int length) {
 Serial.print("Message arrived in topic: ");
 Serial.println(topic);
 Serial.print("Message:");
 for (int i = 0; i < length; i++) {
     Serial.print((char) payload[i]);
 }
 Serial.println();
 Serial.println("-----------------------");
}



