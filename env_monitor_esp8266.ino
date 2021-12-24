#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <Adafruit_HTS221.h>

// Set variables in rhe file
#include "arduino_secrets.h"

#define wifi_ssid "wifi"
#define wifi_password "password"

// https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
const char* TZstr = "CST+6CDT";

#define mqtt_server "192.168.61.34"

#define humidity_topic "home/roomtemp"
#define temperature_topic "home/roomtemp"

const char* ntpServer = "time.sigint.cxm";


WiFiClient espClient;
PubSubClient client(espClient);
//Adafruit_HDC1000 hdc = Adafruit_HDC1000();
Adafruit_HTS221 hts;

String clientunqiueid = "schmeckles";

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // Set SDA and SDL ports
  //Wire.begin(2, 14);
  Wire.begin(4, 5);

  // Start sensor
  if (!hts.begin_I2C()) {
    Serial.println("Couldn't find sensor!");
    while (1);
  }
  Serial.println("HTS221 Found!");

  Serial.print("Data rate set to: ");
  switch (hts.getDataRate()) {
   case HTS221_RATE_ONE_SHOT: Serial.println("One Shot"); break;
   case HTS221_RATE_1_HZ: Serial.println("1 Hz"); break;
   case HTS221_RATE_7_HZ: Serial.println("7 Hz"); break;
   case HTS221_RATE_12_5_HZ: Serial.println("12.5 Hz"); break;
  }
  
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
  clientunqiueid  = String(WiFi.macAddress());
  clientunqiueid.replace(":", "_");

  configTime(TZstr, ntpServer);
  Serial.println("\nWaiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
}




void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    // if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//bool checkBound(float newValue, float prevValue, float maxDiff) {
//  return !isnan(newValue) &&
//         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
//}

long lastMsg = 0;
float ttemp = 0.0;
float hum = 0.0;
float diff = 1.0;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  
  
  
  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;

    time_t mynow = time(nullptr);
    //Serial.println("Time from timeserver");
    char *t = ctime(&mynow);
    if (t[strlen(t)-1] == '\n') t[strlen(t)-1] = '\0';
    
  
    sensors_event_t temp;
    sensors_event_t humidity;
    hts.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data

    float newTemp = temp.temperature;
    float newHum = humidity.relative_humidity;

      String jsonTemp = "'{ \"sensor_id\":\"'" + clientunqiueid + "'\", \"temperature\":\"'" + temp.temperature + "'\", \"datetime\":\"'" + String(t).c_str() + "'\" }'";
      Serial.println(String(jsonTemp));
      client.publish(temperature_topic, String(jsonTemp).c_str(), true);

      String jsonHumidity = "'{ \"sensor_id\":\"'" + clientunqiueid + "'\", \"humidity\":\"'" + humidity.relative_humidity + "'\", \"datetime\":\"'" + String(t).c_str() + "'\" }'";
      Serial.println(String(jsonHumidity));
      client.publish(humidity_topic, String(jsonHumidity).c_str(), true);
  
  }
}
