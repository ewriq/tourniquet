#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <SD.h> 
#include <ArduinoJson.h>
#include <WebServer.h>

#define WIFI_SSID "ewriq"
#define WIFI_PASSWORD "f9663ddc"

#define SS_PIN 5
#define RST_PIN 22 
#define OUTPUT_PIN 4

#define SD_SCK  25
#define SD_MISO 33
#define SD_MOSI 32
#define SD_CS   15
#define Buzzer 4
const long utcOffsetInSeconds = 3 * 3600; 

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0);  
MFRC522 mfrc522(SS_PIN, RST_PIN);
SPIClass customSPI(HSPI); 
WebServer server(80);

void setup() {
    Serial.begin(115200);
    SPI.begin();
    mfrc522.PCD_Init();
    customSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    
    pinMode(OUTPUT_PIN, OUTPUT);
    digitalWrite(OUTPUT_PIN, LOW);

    Serial.println("RFID Okuyucu Hazır!");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("WiFi'ye bağlanıyor...");""
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi bağlantısı başarılı!");
    Serial.println(WiFi.localIP());
    timeClient.begin();
    updateTime(); 
    
    

    if (!SD.begin(SD_CS, customSPI)) {
        Serial.println("SD Kart başlatılamadı!");
        return;
    }
    Serial.println("SD Kart başarılı şekilde başlatıldı!");

    server.on("/", HTTP_GET, handleFileDisplay);
    server.begin();
    Serial.println("Web sunucusu başlatıldı.");
}

void loop() {
    server.handleClient();
    updateTime();  
    String currentTime = getFormattedTime();
    String uid = getCardUID();
    
    if (uid != "") {
        Serial.println(uid);
        Serial.println(currentTime);
        digitalWrite(Buzzer, 1);
        File file = SD.open("/data.txt", FILE_APPEND);
        if (file) {
            file.print("ID: ");
            file.print(uid);
            file.print(", Zaman: ");
            file.println(currentTime);
            file.close();

            Serial.println("Yeni veri kaydedildi.");
               digitalWrite(Buzzer, 0);
        } else {
          delay(5000);
             digitalWrite(Buzzer, 0);
            Serial.println("Dosya açılamadı!");
        }
    }
}

String getCardUID() {
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return "";
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
        return "";
    }

    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        uid += String(mfrc522.uid.uidByte[i], HEX);
    }

    mfrc522.PICC_HaltA();
    return uid;
}

String getFormattedTime() {
    char timeStr[20]; 
    sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d",
            year(), month(), day(),
            hour(), minute(), second());
    return String(timeStr);
}

void updateTime() {
    timeClient.update();
    setTime(timeClient.getEpochTime() + utcOffsetInSeconds);
}

void handleFileDisplay() {
    File file = SD.open("/data.txt", FILE_READ);
    
    if (!file) {
        server.send(500, "text/plain", "Dosya açılamadı!");
        return;
    }

    String fileContent = "<html><body><h1> Log </h1><pre>";
    while (file.available()) {
        fileContent += (char)file.read();
    }
    fileContent += "</pre></body></html>";
    file.close();
    
    server.send(200, "text/html", fileContent);
}
