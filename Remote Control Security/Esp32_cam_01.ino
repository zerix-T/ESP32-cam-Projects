// The program uses esp32 cam and ultrasonic sensor . It 


#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include "FS.h"                
#include "SD_MMC.h" 
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "driver/rtc_io.h"
#include <EEPROM.h>
#include <Wire.h>
#include<ctime>
using namespace std;


// My network credentials
const char* ssid = "Manish";
const char* password = "password06";

// my telegram chat userID
String chatId = "1068770797";

// Initializing telegram BOT with my token
String BOTtoken = "5179073368:AAEsNBJDK0ce1ixubZlCEzyAKFAt-IfbE_s";

bool sendPhoto = false;

WiFiClientSecure clientTCP;

UniversalTelegramBot bot(BOTtoken, clientTCP);

//CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//#define EEPROM_SIZE 1

#define FLASH_LED_PIN 4 //esp32 inbuilt flash light
bool flashState = LOW;

//ultrasonic sensor
int trig = 13;
int echo = 15;
long duration; //variable stores time duration taken for sensor to receive the signal 
long dist; 
 
int botRequestDelay = 1000;   // mean time between scan messages
long lastTimeBotRan;     // last time messages' scan has been done

void handleNewMessages(int numNewMessages);
String sendPhotoTelegram();

void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);
  
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, flashState);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); //root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  //checking sd card 
 /*  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    return;
  }
  //checking card type
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }*/

  // Drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_CIF);  // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA

}

void loop(){
  digitalWrite(trig,LOW);
  delayMicroseconds(2);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  duration = pulseIn(echo, HIGH);
  dist = duration/58;
  
  if (sendPhoto){
    Serial.println("Preparing photo");
    sendPhotoTelegram(0); 
    sendPhoto = false; 
  }

  if(dist == 10){
    bot.sendMessage(chatId, "The object is at 10cm !", "");
    Serial.println("Object Detected at 10 cm");
    sendPhotoTelegram(dist);
  
  }
    if(dist==20){
    bot.sendMessage(chatId, "The object is at 20cm !!", "");
    Serial.println("Object Detected at 20 cm");
    sendPhotoTelegram(dist);
    
  }
    if(dist==30){
    bot.sendMessage(chatId, "The object is a 30cm away !!!", "");
    Serial.println("Object Detected 30 cm away");
    sendPhotoTelegram(dist);
    
  }
  
  if (millis() > lastTimeBotRan + botRequestDelay){
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages){
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}

String sendPhotoTelegram(long dist){
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  //capturing picture
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }

 /* //For local storage
  // initialize EEPROM 
  EEPROM.begin(EEPROM_SIZE);

    // Declaring argument for time()
    time_t curr_time;
  tm * curr_tm;
  char date_string[100];
  char time_string[100];
  
  time(&curr_time);
  curr_tm = localtime(&curr_time);
  
  strftime(date_string, 50, "%H_%M_%S_%d_%m_%Y", curr_tm);
 
  

  String path = "/image/dump_intern/photo_" +String(dist)+"cm_"+date_string+".jpg";

  fs::FS &fs = SD_MMC; 
  Serial.printf("Picture file name: %s\n", path.c_str());
  
  File file = fs.open(path.c_str(), FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file in writing mode");
  } 
  else {
    file.write(fb->buf, fb->len); 
    Serial.printf("Saved file to path: %s\n", path.c_str());
    EEPROM.write(0, 1);
    EEPROM.commit();
  }
  file.close();*/

  //Send to user telegram bot 
  
  Serial.println("Connect to " + String(myDomain));

  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    
    String head = "--EspManishBot\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chatId + "\r\n--EspManishBot\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"photo_10.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--EspManishBot--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=EspManishBot");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   // timeout 10 seconds
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);
  }
  else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }
  return getBody;
}

void handleNewMessages(int numNewMessages){
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++){
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != chatId){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String fromName = bot.messages[i].from_name;

    if (text == "/flash") {
      flashState = !flashState;
      digitalWrite(FLASH_LED_PIN, flashState);
    }
    if (text == "/photo") {
      sendPhoto = true;
      Serial.println("New photo  request");
    }
   
    if (text == "/start"){
      String welcome = "Welcome to the ESP32-CAM Telegram bot.\n";
      welcome += "/photo : takes a new photo\n";
      welcome += "/flash : toggle flash LED\n";
      welcome += "You'll receive a photo whenever object is detected.\n";
      bot.sendMessage(chatId, welcome, "Markdown");
    }
  }
}
