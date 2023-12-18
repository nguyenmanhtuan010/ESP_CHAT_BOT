#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   
#include <ArduinoJson.h>
#include "DHT.h" // Thư viện cảm biến

// DHT 11
#define DHTPIN 4 
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);

// Replace with your network credentials
const char* ssid = "tun";
const char* password = "12345678";

// Initialize Telegram BOT
#define BOTtoken "6642102681:AAF1ym4abNYFek1MaM-HUjWinY_Sfv_CYVE"  // your Bot Token (Get from Botfather)
#define CHAT_ID "6442269516"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

const int ledPin = 12;
bool ledState = LOW;
const int fanPin = 13;
bool fanState = LOW;

float temperatureThreshold = 25.0;  // Đặt ngưỡng nhiệt độ cần cảnh báo
int temperatureCheckInterval = 5000;  // Kiểm tra nhiệt độ mỗi 5 giây
unsigned long lastTemperatureCheckTime = 0;

String getReadings() {
  float t = dht.readTemperature();
  String message = "Temperature: " + String(t) + " ºC \n";
  return message;
}

// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control your outputs.\n\n";
      welcome += "/led_on to turn GPIO ON \n";
      welcome += "/led_off to turn GPIO OFF \n";
      welcome += "/fan_on to turn GPIO ON \n";
      welcome += "/fan_off to turn GPIO OFF \n";
      welcome += "/state to request current GPIO state \n";
      welcome += "set_threshold to update temperature Threshold\n";
      welcome += "set_CheckInterval to update Check Interval\n";
      welcome += "/readings \n";
      bot.sendMessage(chat_id, welcome, "");
    }
    
    if (text.startsWith("/set_threshold ")) {
      // Lấy giá trị ngưỡng mới từ tin nhắn
      String newThresholdStr = text.substring(15);  // 15 là độ dài của "/set_threshold "
      float newThreshold = newThresholdStr.toFloat();

      if (!isnan(newThreshold)) {
        temperatureThreshold = newThreshold;
        bot.sendMessage(CHAT_ID, "Temperature threshold set to: " + String(temperatureThreshold) + " ºC", "");
      } else {
        bot.sendMessage(CHAT_ID, "Invalid threshold value. Please provide a valid number", "");
      }
    }
    
    if (text.startsWith("/set_interval ")) {
      // Lấy giá trị khoảng thời gian mới từ tin nhắn
      String newIntervalStr = text.substring(14);  // 14 là độ dài của "/set_interval "
      int newInterval = newIntervalStr.toInt();

      if (newInterval > 0) {
        temperatureCheckInterval = newInterval;
        bot.sendMessage(CHAT_ID, "Check Interval set to: " + String(temperatureCheckInterval) + " milliseconds", "");
      } else {
        bot.sendMessage(CHAT_ID, "Invalid threshold value. Please provide a valid number", "");
      }
    }
   
    if (text == "/led_on") {
      bot.sendMessage(chat_id, "LED state set to ON", "");
      ledState = HIGH;
      digitalWrite(ledPin, ledState);
    }
    
    if (text == "/led_off") {
      bot.sendMessage(chat_id, "LED state set to OFF", "");
      ledState = LOW;
      digitalWrite(ledPin, ledState);
    }
    
    if (text == "/fan_on") {
      bot.sendMessage(chat_id, "LED state set to ON", "");
      fanState = HIGH;
      digitalWrite(fanPin, fanState);
    }
    
    if (text == "/fan_off") {
      bot.sendMessage(chat_id, "LED state set to OFF", "");
      fanState = LOW;
      digitalWrite(fanPin, fanState);
    }
    
    if (text == "/state") {
      if (digitalRead(ledPin)){
        bot.sendMessage(chat_id, "LED is ON", "");
      }
      else{
        bot.sendMessage(chat_id, "LED is OFF", "");
      }
      if (digitalRead(fanPin)){
        bot.sendMessage(chat_id, "FAN is ON", "");
      }
      else{
        bot.sendMessage(chat_id, "FAN is OFF", "");
      }
    }
    if (text == "/readings") {
      String readings = getReadings();
      bot.sendMessage(chat_id, readings, "");
    }  
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin(); //khởi động DHT11
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);
  pinMode(fanPin, OUTPUT);
  digitalWrite(fanPin, fanState);
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected.");
}

void loop() {
  // Kiểm tra nhiệt độ liên tục mỗi khoảng thời gian
  if (millis() > lastTemperatureCheckTime + temperatureCheckInterval)  {
    float currentTemperature = dht.readTemperature();
    if (!isnan(currentTemperature) && currentTemperature > temperatureThreshold) {
      String alertMessage = "Cảnh báo: Nhiệt độ vượt quá ngưỡng! Nhiệt độ hiện tại: " + String(currentTemperature) + " ºC";
      bot.sendMessage(CHAT_ID, alertMessage, "");
    }

    lastTemperatureCheckTime = millis();
  }

  // Kiểm tra tin nhắn từ chatbot
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}
