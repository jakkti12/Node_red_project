#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN 14        // GPIO สำหรับเซ็นเซอร์ DHT22
#define DHTTYPE DHT22

const char* ssid = "Banana";
const char* password = "999888999";
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* mqttTopicTemperature = "sensor1/tempkit";
const char* mqttTopicHumidity = "sensor2/humikit";

const char* mqtttopicLed = "sensor1/tempkit/led";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

bool systemActive = false; // สถานะเปิด-ปิดระบบ

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(13, OUTPUT);

  pinMode(36, INPUT);
  pinMode(37, INPUT);

  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  
  delay(2000);          // รอให้เซ็นเซอร์พร้อม
  dht.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // ตรวจสอบสถานะปุ่มเปิด/ปิดระบบ
  if (digitalRead(36) == LOW) { // กดปุ่ม 36 เพื่อเปิดระบบ
    systemActive = true;
    Serial.println("System Activated");
    client.publish("system/status", "Activated");
    delay(500); // ป้องกันการกดซ้ำ
  }

  if (digitalRead(37) == LOW) { // กดปุ่ม 37 เพื่อปิดระบบ
    systemActive = false;
    Serial.println("System Deactivated");
    client.publish("system/status", "Deactivated");
    delay(500); // ป้องกันการกดซ้ำ
  }

  // ถ้าระบบไม่เปิด ให้หยุดการทำงานของเซ็นเซอร์และหลอดไฟ
  if (!systemActive) {
    digitalWrite(2, LOW); // ปิด LED สีแดง
    digitalWrite(4, LOW); // ปิด LED สีเหลือง
    digitalWrite(5, LOW); // ปิด LED สีเขียว
    return;
  }

  // ระบบทำงาน อ่านค่าเซ็นเซอร์
  Serial.println("Reading sensor...");
  
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // ตรวจสอบข้อผิดพลาดในการอ่านค่า
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor");
    delay(1000);
    return;
  }

  // ส่งค่าไปยัง MQTT broker
  char temperatureString[6];
  dtostrf(temperature, 5, 2, temperatureString);
  client.publish(mqttTopicTemperature, temperatureString);

  char humidityString[6];
  dtostrf(humidity, 5, 2, humidityString);
  client.publish(mqttTopicHumidity, humidityString);

  // แสดงผลค่าที่อ่านได้
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  // ควบคุม LED ตามอุณหภูมิ
  if (temperature < 25) {
    digitalWrite(5, HIGH);  // เปิด LED สีเขียว
    digitalWrite(4, LOW);   // ปิด LED สีเหลือง
    digitalWrite(2, LOW);   // ปิด LED สีแดง
    client.publish(mqtttopicLed,"green");
    Serial.println("G");
  } 
  else if (temperature >= 25 && temperature < 30) {
    digitalWrite(5, LOW);   // ปิด LED สีเขียว
    digitalWrite(4, HIGH);  // เปิด LED สีเหลือง
    digitalWrite(2, LOW);   // ปิด LED สีแดง
    client.publish(mqtttopicLed,"yellow");
    Serial.println("Y");
  } 
  else {  // temperature >= 30
    digitalWrite(5, LOW);   // ปิด LED สีเขียว
    digitalWrite(4, LOW);   // ปิด LED สีเหลือง
    digitalWrite(2, HIGH);  // เปิด LED สีแดง
    client.publish(mqtttopicLed,"red");
    Serial.println("R");
  }

  delay(5000); // หน่วงเวลา 5 วินาที
}

void setup_wifi() {
  delay(10);
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
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32jakkti", "", "" )) {
      Serial.println("mqtt connected");
      client.subscribe("master/kit");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = 0;
  Serial.print("MQTT: topic=");
  Serial.println(topic);
  Serial.print("MQTT: payload=");
  Serial.println((char*) payload);

  if (strcmp(topic, "master/kit") == 0) {
    if (payload[0] == '0') {  //ปิดระบบ
      systemActive = false;
      Serial.println("System Deactivated");
      client.publish("system/status", "Deactivated");
      delay(500); //กันการกดซ้ำ
    }
    if (payload[0] == '1') {  //เปิดระบบ
      systemActive = true;
      Serial.println("System Activated");
      client.publish("system/status", "Activated");
      delay(500); //กันการกดซ้ำ
    }
   }
}
