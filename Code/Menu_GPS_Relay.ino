// memanggil semua library yang diperlukan
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h> 
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <pgmspace.h> // PROGMEM support for ESP32
Adafruit_SSD1306 OLED(128, 64, &Wire, -1);
TinyGPSPlus gps;
HardwareSerial GPS_Serial(1); // serial komunikasi yang digunakan bukan default

// setting pin GPIO yang digunakan
#define bottonUp    26      
#define bottonDown  25
#define bottonOk    33
#define bottonBack  32

// setting status awal semua input false untuk memastikan input yang masuk pasti akan bernilai true:
bool statusUp     = false;
bool statusDown   = false;
bool statusOk     = false;
bool statusBack   = false;

bool Up     = false;
bool Down   = false;
bool Ok     = false;
bool Back   = false;

// memastikan kondisi true (input) untuk kembali ke kondisi awalnya
bool statusAkhirUp    = false;
bool statusAkhirDown  = false;
bool statusAkhirOk    = false;
bool statusAkhirBack  = false;

bool inputOk      = true;
bool statusRelay  = HIGH;

// optional setting status relay: dipisah untuk memudahkan perancangan alur logika relay
bool statusRelay2 = !statusRelay;                     // 
bool statusRelay3 = !statusRelay2;
bool statusRelay4 = statusRelay3;

int page                      = 1;                  // memulai indeks menu pokok bersarang dari 1 
int menuItem                  = 1;                  // memulai indeks submenu bersarang dari 1 
const int totalMenuItem       = 5;                  // setting jumlah total menu utama yaitu 5
const int totalSubItem        = 2;                  // setting jumlah total semua submenu yaitu 2
int password[4]               = {0, 0, 0, 0};       // setting jumlah digit passsword
int indexPassword             = 0;                  // setting indeks password dimulai dari 0 untuk mempermudah iterasi looping
const int correctPassword[4] = {2, 2, 2, 2};        // setting digit password dengan tipe data constan integer

const char* menuPokok[] = {                         // Menjabarkan 5 menu utama dari menuItem
  "Call",                                           
  "Power On",
  "Jaringan",
  "Emergency",
  "Mode",
};

const char* menuGSM[] = {                          // Menjabarkan 2 submenu dari fitur SOS
  "ARDDTracker",
  "Pak Joko",
};

const char* menuMode[] = {                        // Menjabarkan 2 submenu dari jenis mode Car
  "Manual",
  "Otomatis",
};

// jaringan yang akan terhubung ke ESP32
const char* ssid = "up";                          // sesuaikan dengan nama jaringan
const char* sandi = "GodblessTEUNS";              // sesuaikan dengan password/sandi jaringan

// setting alamat pengiriman pesan SOS ke bot telegram
#define BOTtoken "7689031816:AAFipbZXuMtUana873zQz-xPmEDvPY-dFnc"       // kode token dari @BotFather
#define CHAT_ID "7220266556"                                            // kode ID chat dari @myidbot
#define CHAT_ID1 "1393124956"
#define CHAT_ID2 "5090269185"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

// Variabel untuk menyimpan lokasi 
char googleMapsLink[128] = "Tidak ada modul GPS terhubung."; 

// Pin Relay
const int relayPin = 13;

const char startMessage[] = 
  "Selamat Datang, kelompok PKE-9: ARDD-Tracker El-Semar#12. \n"
  "Masukkan perintah yang ingin dilakukan: \n\n"
  "/Lokasi - untuk mendapatkan lokasi kendaraan.\n"
  "/CutOff - untuk mematikan kendaraan.\n"
  "/PowerOn - untuk menghidupkan kendaraan.\n";

const char SOSMessage[]= "Pengendara Dalam Keadaan darurat!! \n";

const char unauthorizedMessage[] = "Unauthorized user";
const char pesanPengambilan_Lokasi[] = "Mengambil lokasi...";
const char pesanLokasi[]  = "Lokasi kendaraan: ";
const char cutOffMessage[]  = "Kendaraan dimatikan!";
const char powerOnMessage[] = "Kendaraan dihidupkan!";

void setup() {
  Serial.begin(15200);
  if (!OLED.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println(F("SSD1306 failed"));
      for (;;);  // Stop the loop
    }
    OLED.clearDisplay();
    pinMode(bottonUp, INPUT_PULLUP);
    pinMode(bottonDown, INPUT_PULLUP);
    pinMode(bottonOk, INPUT_PULLUP);
    pinMode(bottonBack, INPUT_PULLUP);

  // Inisialisasi GPS
    GPS_Serial.begin(9600, SERIAL_8N1, 4, 2); // RX: GPIO 2, TX: GPIO 4
    // Inisialisasi komunikasi Serial2 untuk pengiriman data (Antar ESP)
    Serial2.begin(9600, SERIAL_8N1, 17, 16); // TX: GPIO 16, RX: GPIO 17
    Serial.println(F("Mencari Koordinat..."));

    // Inisialisasi pin relay
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, statusRelay); // Pastikan relay OFF di awal

    // Koneksi WiFi
    WiFi.disconnect(true); // Reset WiFi config
    delay(1000);
    WiFi.begin(ssid, sandi);

    Serial.print(F("Connecting to "));
    Serial.print(ssid);
    Serial.println(F("..."));

    // Cek status koneksi
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(F("."));
    }

    Serial.println();
    Serial.println(F("WiFi connected"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());

    #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    #endif
}

void loop() { 
  tampil();
  statusUp    = digitalRead(bottonUp);
  statusDown  = digitalRead(bottonDown);
  statusOk    = digitalRead(bottonOk);
  statusBack  = digitalRead(bottonBack);
  pressUp();
  pressDown();
  pressOk();
  pressBack();

  if (Up && page == 1) {
    Up = false;
    menuItem--;
    if (menuItem < 1) {
      menuItem = totalMenuItem;
    }
  }

  if (Down && page == 1) {
    Down = false;
    menuItem++;
    if (menuItem > totalMenuItem) {
      menuItem = 1;
    }
  }

  if (Up && page == 4) {
    Up = false;
    menuItem--;
    if (menuItem < 1) {
      menuItem = totalMenuItem;
    }
  } 

  if (Up && page == 6) {
    Up = false;
    menuItem--;
    if (menuItem < 1) {
      menuItem = totalMenuItem;
    }
  } 

  if (Down && page == 4) {
    Down = false;
    menuItem++;
    if (menuItem > totalMenuItem) {
      menuItem = 1;
    }
  }

  if (Down && page == 6) {
    Down = false;
    menuItem++;
    if (menuItem > totalMenuItem) {
      menuItem = 1;
    }
  }

  if (Up && page == 5) {
    Up = false;
    menuItem--;
    if (menuItem < 1) {
      menuItem = totalMenuItem;
    }
  }

  if (Down && page == 5) {
    Down = false;
    menuItem++;
    if (menuItem > totalMenuItem) {
      menuItem = 1;
    }
  }

  if (Ok) {
    Ok = false;
    if (page == 1){
      page = menuItem + 1;
    } if(page == 4){
      int indexArray = (menuItem - 1) % totalSubItem;
      if (strcmp(menuGSM[indexArray], "ARDDTracker")== 0) {
        //  kirim chat SOS via telegram
        updateGPS(); // pembaruan GPS
        char SOSMessage[256];
        snprintf(SOSMessage, sizeof(SOSMessage),"Pengendara dalam kondisi darurat!! \nLokasi: %s", googleMapsLink);
        bot.sendMessage(CHAT_ID, SOSMessage, "");
        // kembali ke page awal
        page = 7;
        displayMessage("Pesan Terkirim");
        delay(1500);
        page = 4;
      } else {
        updateGPS(); // pembaruan GPS
        char SOSMessage[256];
        snprintf(SOSMessage, sizeof(SOSMessage),"Pengendara dalam kondisi darurat!! \nLokasi: %s", googleMapsLink);
        bot.sendMessage(CHAT_ID1, SOSMessage, "");
        bot.sendMessage(CHAT_ID2, SOSMessage, "");
        // kembali ke page awal
        page = 7;
        displayMessage("Pesan Terkirim");
        delay(1500);
        page = 4;
      }
    } else if (page == 5){
      int indexArray = (menuItem - 1) % totalSubItem;
      if (strcmp(menuMode[indexArray], "Manual")== 0) {
        Serial2.println("1");  
        page = 8;
        displayMessage("Mode RC-Car!");
        delay(1500);
        page = 5;
      } else {
        Serial2.println("2"); 
        page = 8;
        displayMessage("Mode Line Follower");
        delay(1500);
        page = 5;
      }
    } else if (page == 6){
      page = 9;
    } else if(statusOk == LOW) {
      password[indexPassword] = 3;
      indexPassword++;
    }
      delay(100);
    }

  if (Back) {
    Back = false;
    if (page > 2 && page <= 5) {
      page = 1;
      delay(1000);
    } else if (page == 7) {
      page = 4;
    } else if (page == 8) {
      page = 5;
    } else if (page == 6){
      page = 1;
    } else if (page == 9){
      page = 6;
    }
  }

  if (page == 2) {
  static bool inputOk = true; 
  if (inputOk) {
    inputOk = false;
    delay(250);
  
    for (int i = 0; i < 4; i++) {
      password[i] = 0;
    }
    indexPassword = 0;
  }
  if (statusUp == LOW) {
    password[indexPassword] = 1;
    indexPassword++;
    delay(250);
  } else if (statusDown == LOW) {
    password[indexPassword] = 2;
    indexPassword++;
    delay(250);
  } else if (statusBack == LOW) {
    password[indexPassword] = 4;
    indexPassword++;
    delay(250);
  }
    

  if (indexPassword == 4) {
    if (checkPassword()) {
      displayMessage("Password Valid");
    } else {
      displayMessage("Invalid Password");
    }
    delay(2000); // delay 2 detik trus otomatis balek ke page 1
    indexPassword = 0; 
    page = 1; 
    
    inputOk = true; // Reset first entry flag
  }
}

  if (page == 9){
    displayMessage("Berhasil Memanggil");
    delay(1500);
    page =6;
  }

  if (page == 3){
    if (WiFi.status() == WL_CONNECTED){
      displayMessage("Terhubung...");
      delay(2000);
      page =1;
    } else{
      displayMessage("Terputus!!");
      delay(2000);
      page =1;
    }
  }

  // Periksa pesan baru dari Telegram
  if (millis() > lastTimeBotRan + botRequestDelay) {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

      while (numNewMessages) {
          handleNewMessages(numNewMessages);
          numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      lastTimeBotRan = millis();
  }

  // Perbarui data GPS di latar belakang
  updateGPS();
};

void pressUp() {
  if (statusUp != statusAkhirUp) {
    if (statusUp == LOW) {
      Up = true;
    }
    delay(100);
  }
  statusAkhirUp = statusUp;
}

void pressDown() {
  if (statusDown != statusAkhirDown) {
    if (statusDown == LOW) {
      Down = true;
    }
    delay(100);
  }
  statusAkhirDown = statusDown;
}

void pressOk() {
  if (statusOk != statusAkhirOk) {
    if (statusOk == LOW) {
      Ok = true;
    }
    delay(50);
  }
  statusAkhirOk = statusOk;
}

void pressBack() {
  if (statusBack != statusAkhirBack) {
    if (statusBack == LOW) {
      Back = true;
    }
    delay(100);
  }
  statusAkhirBack = statusBack;
}

void tampil() {
  if (page == 1) {                  // interface page awal : menu Utama
    OLED.clearDisplay();
    OLED.setTextSize(1);
    OLED.setTextColor(SSD1306_WHITE);
    OLED.setCursor(10, 0);
    OLED.print("Smart GPS Tracking");
    OLED.drawLine(0, 12, 128, 12, SSD1306_WHITE);
    int indexArray1 = (menuItem - 1) % totalMenuItem;
    for (int i = 0; i < 3; i++) {
      int index1 = (indexArray1 + i) % totalMenuItem;
      OLED.setCursor(5, 24 + i * 10);
      if (i == 1) {
        OLED.fillRect(0, 22 + (i * 10), 128, 10, SSD1306_WHITE);
        OLED.setTextColor(SSD1306_BLACK);
      } else {
        OLED.setTextColor(SSD1306_WHITE);
      }
      OLED.print(menuPokok[index1]);
    }
    OLED.display();
  } else if (page == 2) {         // Masuk subMenu PowerOn
    OLED.clearDisplay();
    OLED.setTextSize(2);
    OLED.setTextColor(SSD1306_WHITE);
    OLED.setCursor(17, 0);
    OLED.print("Password:");
    OLED.drawLine(0, 20, 128, 20, SSD1306_WHITE);
    inputPassword();
    OLED.display();
  } else if (page == 3){          // Masuk Submenu SOS: Message
    OLED.clearDisplay();  // koneksi/jaringan
  } else if (page == 4) {
    OLED.clearDisplay();
    OLED.setTextSize(2);
    OLED.setTextColor(WHITE);
    OLED.setCursor(0, 0);
    OLED.print("Message To");
    OLED.drawLine(0, 20, 128, 20, WHITE);
    
    int indexArray2 = (menuItem - 1) % totalSubItem;

    for (int i = 0; i < totalSubItem; i++) { 
      OLED.setCursor(5, 24 + i * 10);
      OLED.setTextSize(1);
      if (i == indexArray2) { 
        OLED.fillRect(0, 22 + i * 10, 128, 10, WHITE);
        OLED.setTextColor(BLACK);
      } else {
        OLED.setTextColor(WHITE);
      }
      OLED.print(menuGSM[i]);
    }
  } else if (page == 6) {         // Masuk Submenu SOS: Call
    OLED.clearDisplay();
    OLED.setTextSize(2);
    OLED.setTextColor(WHITE);
    OLED.setCursor(20, 0);
    OLED.print("Call To:");
    OLED.drawLine(0, 20, 128, 20, WHITE);
    
    int indexArray3 = (menuItem - 1) % totalSubItem;

    for (int i = 0; i < totalSubItem; i++) { 
      OLED.setCursor(5, 24 + i * 10);
      OLED.setTextSize(1);
      if (i == indexArray3) { 
        OLED.fillRect(0, 22 + i * 10, 128, 10, WHITE);
        OLED.setTextColor(BLACK);
      } else {
        OLED.setTextColor(WHITE);
      }
      OLED.print(menuGSM[i]);
    }
  } else if (page == 7) {         // page untuk display : pesan terkirim
    OLED.clearDisplay();
  } else if (page == 9) {         // page untuk display: berhasil memanggil (SOS call)
    OLED.clearDisplay();
  } else if (page == 5){          // masuk Submenu car-Mode
    OLED.clearDisplay();
    OLED.setTextSize(2);
    OLED.setTextColor(WHITE);
    OLED.setCursor(0, 0);
    OLED.print("Car Mode");
    OLED.drawLine(0, 20, 128, 20, WHITE);
    
    int indexArray4 = (menuItem - 1) % totalSubItem;

    for (int i = 0; i < totalSubItem; i++) { 
      OLED.setCursor(5, 24 + i * 10);
      OLED.setTextSize(1);
      if (i == indexArray4) { 
        OLED.fillRect(0, 22 + i * 10, 128, 10, WHITE);
        OLED.setTextColor(BLACK);
      } else {
        OLED.setTextColor(WHITE);
      }
      OLED.print(menuMode[i]);
  }
  }
  else if (page == 8){            // page untuk display: Mode Car yang dijalankan (subdari page 5)
    OLED.clearDisplay();
  }

  OLED.display();
}

void inputPassword() {
  OLED.setCursor(38, 38);
  for (int i = 0; i < 4; i++) {
    OLED.print(password[i]);
  }
  OLED.display();
}

bool checkPassword() {
  if (password[0] != correctPassword[0] || password[1] != correctPassword[1] || 
      password[2] != correctPassword[2] || password[3] != correctPassword[3]){
    return false;
  } else{
    digitalWrite(relayPin, statusRelay3);
    return true;
  }
}

void displayMessage(const char* message) {
  OLED.clearDisplay();
  OLED.setTextColor(WHITE);
  OLED.setTextSize(1);
  OLED.setCursor(18, 30);
  OLED.print(message);
  OLED.display();
}


// Fungsi membaca data GPS
void updateGPS() {
  while (GPS_Serial.available() > 0) {
      gps.encode(GPS_Serial.read()); // Decode GPS data
  }

  if (gps.location.isUpdated()) {
      double latitude = gps.location.lat();
      double longitude = gps.location.lng();

      // Generate Google Maps link
      snprintf(googleMapsLink, sizeof(googleMapsLink), 
                "https://www.google.com/maps/search/?api=1&query=%.6f,%.6f", 
                latitude, longitude);

      Serial.print(F("Koordinat diperbarui: "));
      Serial.println(googleMapsLink);
  }
}

// Fungsi untuk menangani pesan Telegram
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
      String chat_id = bot.messages[i].chat_id;
      if (chat_id != CHAT_ID) {
          bot.sendMessage(chat_id, FPSTR(unauthorizedMessage), "");
          continue;
      }

      String text = bot.messages[i].text;
      Serial.print(F("Pesan diterima: "));
      Serial.println(text);

      if (text == "/start") {
          bot.sendMessage(chat_id, FPSTR(startMessage), "");
      } else if (text == "/Lokasi") {
          bot.sendMessage(chat_id, FPSTR(pesanPengambilan_Lokasi), "");
          updateGPS(); // Perbarui data GPS
          char message[200]; // untuk pesan lokasi
          snprintf(message, sizeof(message), "%s%s", FPSTR(pesanLokasi), googleMapsLink);
          bot.sendMessage(chat_id, message, "");
      } else if (text == "/CutOff") {
          // Kendaraan dimatikan (Relay LOW)
          digitalWrite(relayPin, statusRelay2);
          if (statusRelay2 == LOW){
          bot.sendMessage(chat_id, FPSTR(cutOffMessage), "");
      } else {
        bot.sendMessage(chat_id, "Gagal Mematikan Kendaran!");
      }
      } 
      else if (text = "/PowerOn"){
        // Kendaraan dihidupkan (Relay HIGH)
          digitalWrite(relayPin, statusRelay4);
          if (statusRelay4 == HIGH){
          bot.sendMessage(chat_id, FPSTR(powerOnMessage), "");
      } else {
        bot.sendMessage(chat_id, "Gagal Mematikan Kendaran!");
      }
      }
  }
}