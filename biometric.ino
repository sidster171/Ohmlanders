/*
 * IoT-Based Biometric Attendance System
 * Microcontroller: ESP8266 (NodeMCU)
 * Fingerprint Sensor: R307
 * Display: I2C LCD
 * Cloud: Google Sheets via Apps Script
 * 
 * Required Libraries:
 * - Adafruit_Fingerprint (Install via Library Manager)
 * - LiquidCrystal_I2C (Install via Library Manager)
 * - ESP8266WiFi (Built-in with ESP8266 board package)
 * - ESP8266HTTPClient (Built-in with ESP8266 board package)
 * - WiFiClientSecure (Built-in with ESP8266 board package)
 */

gendu siddddddd
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>  // ADDED for HTTPS support

// ========== PIN CONFIGURATION ==========
#define FINGER_RX 14 // D5 - Connect to R307 TX
#define FINGER_TX 12 // D6 - Connect to R307 RX

// ========== WiFi CREDENTIALS ==========
const char* ssid = "sidice";          // Replace with your WiFi name
const char* password = "wtap4337";    // Replace with your WiFi password

// ========== GOOGLE SHEETS CONFIGURATION ==========
// Replace with your Google Apps Script Web App URL
const char* serverName = "https://script.google.com/macros/s/AKfycbxvho8BTSxNltQMT4jBwVt53ljhf8yP7liN0x_Exi5EpX97nuqzFge5EcJ0ETZFBoGsLw/exec";

// ========== HARDWARE INITIALIZATION ==========
SoftwareSerial mySerial(FINGER_RX, FINGER_TX);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27, 16x2 LCD

// ========== WiFi MONITORING ==========
unsigned long previousMillis = 0;
const long interval = 30000;  // Check WiFi every 30 seconds

// ========== DATA STRUCTURES ==========
#define MAX_USERS 50

struct UserData {
  uint8_t id;
  char name[30];
  char rollNumber[15];
  char className[20];
  bool isActive;
};

UserData users[MAX_USERS];
int userCount = 0;

// ========== FUNCTION DECLARATIONS ==========
void enrollFingerprint();
void takeAttendance();
uint8_t getFingerprintEnroll(uint8_t id);
int getFingerprintID();
void sendToGoogleSheets(String name, String roll, String className);
void connectWiFi();
void displayMessage(String line1, String line2);
void addUser(uint8_t id, String name, String roll, String className);
UserData* getUserByID(uint8_t id);

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  displayMessage("Biometric Sys", "Initializing...");
  
  // Initialize fingerprint sensor
  mySerial.begin(57600);
  delay(100);
  
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor found!");
    displayMessage("Sensor: OK", "");
    delay(1000);
  } else {
    Serial.println("Fingerprint sensor not found!");
    displayMessage("Sensor: ERROR", "Check Connection");
    while (1) { delay(1); }
  }
  
  // Connect to WiFi
  connectWiFi();
  
  // Initialize user data
  for (int i = 0; i < MAX_USERS; i++) {
    users[i].isActive = false;
  }
  
  displayMessage("System Ready", "Place Finger");
  
  Serial.println("\n=== Biometric Attendance System ===");
  Serial.println("Commands:");
  Serial.println("E - Enroll new fingerprint");
  Serial.println("A - Attendance mode (automatic)");
  Serial.println("====================================\n");
}

// ========== MAIN LOOP ==========
void loop() {
  // Periodic WiFi connection check
  unsigned long currentMillis = millis();
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {
    Serial.println("WiFi connection lost. Reconnecting...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
  
  // Check for serial commands
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'E' || command == 'e') {
      enrollFingerprint();
    }
  }
  
  // Continuous attendance scanning
  takeAttendance();
  delay(50);
}

// ========== WiFi CONNECTION ==========
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  displayMessage("Connecting WiFi", "Please wait...");
  
  WiFi.mode(WIFI_STA);  // Set to station mode
  WiFi.persistent(false);  // Don't save WiFi config to flash
  delay(100);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Enable auto-reconnect
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    
    displayMessage("WiFi: Connected", WiFi.localIP().toString());
    delay(2000);
  } else {
    Serial.println("\nWiFi Connection Failed!");
    displayMessage("WiFi: Failed", "Check Credentials");
    delay(3000);
  }
}

// ========== DISPLAY MESSAGE ==========
void displayMessage(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  if (line2.length() > 0) {
    lcd.setCursor(0, 1);
    lcd.print(line2);
  }
}

// ========== ENROLLMENT MODE ==========
void enrollFingerprint() {
  Serial.println("\n=== ENROLLMENT MODE ===");
  displayMessage("Enrollment Mode", "");
  
  // Get ID
  Serial.print("Enter ID (1-127): ");
  displayMessage("Enter ID", "via Serial");
  
  while (Serial.available() == 0) {}
  uint8_t id = Serial.parseInt();
  while(Serial.available()) Serial.read(); // Clear buffer
  Serial.println(id);
  
  if (id == 0 || id > 127) {
    Serial.println("Invalid ID!");
    displayMessage("Invalid ID", "Try Again");
    delay(2000);
    return;
  }
  
  // Get Name
  Serial.print("Enter Name: ");
  displayMessage("Enter Name", "via Serial");
  while (Serial.available() == 0) {}
  String name = Serial.readStringUntil('\n');
  name.trim();
  Serial.println(name);
  
  if (name.length() == 0) {
    Serial.println("Name cannot be empty!");
    displayMessage("Empty Name", "Try Again");
    delay(2000);
    return;
  }
  
  // Get Roll Number
  Serial.print("Enter Roll Number: ");
  displayMessage("Enter Roll No", "via Serial");
  while (Serial.available() == 0) {}
  String rollNumber = Serial.readStringUntil('\n');
  rollNumber.trim();
  Serial.println(rollNumber);
  
  // Get Class
  Serial.print("Enter Class: ");
  displayMessage("Enter Class", "via Serial");
  while (Serial.available() == 0) {}
  String className = Serial.readStringUntil('\n');
  className.trim();
  Serial.println(className);
  
  // Enroll fingerprint
  Serial.println("\nStarting fingerprint enrollment...");
  displayMessage("Place Finger", "on Sensor");
  
  if (getFingerprintEnroll(id) == FINGERPRINT_OK) {
    addUser(id, name, rollNumber, className);
    Serial.println("Enrollment Successful!");
    displayMessage("Success!", name);
    delay(2000);
  } else {
    Serial.println("Enrollment Failed!");
    displayMessage("Failed!", "Try Again");
    delay(2000);
  }
  
  displayMessage("System Ready", "Place Finger");
}

// ========== FINGERPRINT ENROLLMENT PROCESS ==========
uint8_t getFingerprintEnroll(uint8_t id) {
  int p = -1;
  
  // Take first image
  Serial.println("Place finger on sensor...");
  displayMessage("Place Finger", "Image 1 of 2");
  
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) continue;
    if (p != FINGERPRINT_OK) return p;
  }
  
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) return p;
  
  Serial.println("Remove finger...");
  displayMessage("Remove Finger", "");
  delay(2000);
  
  while (finger.getImage() != FINGERPRINT_NOFINGER);
  
  // Take second image
  p = -1;
  Serial.println("Place same finger again...");
  displayMessage("Place Finger", "Image 2 of 2");
  
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) continue;
    if (p != FINGERPRINT_OK) return p;
  }
  
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) return p;
  
  // Create model
  Serial.println("Creating model...");
  displayMessage("Processing...", "");
  
  p = finger.createModel();
  if (p != FINGERPRINT_OK) return p;
  
  // Store model
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Fingerprint stored!");
  }
  
  return p;
}

// ========== ADD USER TO LOCAL STORAGE ==========
void addUser(uint8_t id, String name, String roll, String className) {
  if (userCount < MAX_USERS) {
    users[userCount].id = id;
    
    // Use strncpy for proper null-terminated string handling
    strncpy(users[userCount].name, name.c_str(), 29);
    users[userCount].name[29] = '\0';  // Force null termination
    
    strncpy(users[userCount].rollNumber, roll.c_str(), 14);
    users[userCount].rollNumber[14] = '\0';
    
    strncpy(users[userCount].className, className.c_str(), 19);
    users[userCount].className[19] = '\0';
    
    users[userCount].isActive = true;
    userCount++;
    
    Serial.println("User added to local storage:");
    Serial.print("  ID: ");
    Serial.println(users[userCount-1].id);
    Serial.print("  Name: ");
    Serial.println(users[userCount-1].name);
    Serial.print("  Roll: ");
    Serial.println(users[userCount-1].rollNumber);
    Serial.print("  Class: ");
    Serial.println(users[userCount-1].className);
  }
}

// ========== GET USER BY ID ==========
UserData* getUserByID(uint8_t id) {
  for (int i = 0; i < MAX_USERS; i++) {
    if (users[i].isActive && users[i].id == id) {
      return &users[i];
    }
  }
  return nullptr;
}

// ========== ATTENDANCE MODE ==========
void takeAttendance() {
  int fingerprintID = getFingerprintID();
  
  if (fingerprintID > 0) {
    Serial.print("Fingerprint ID: ");
    Serial.println(fingerprintID);
    
    UserData* user = getUserByID(fingerprintID);
    
    if (user != nullptr) {
      // Display on LCD
      displayMessage(String(user->name), String(user->rollNumber));
      
      Serial.println("=== Attendance Marked ===");
      Serial.print("Name: ");
      Serial.println(user->name);
      Serial.print("Roll: ");
      Serial.println(user->rollNumber);
      Serial.print("Class: ");
      Serial.println(user->className);
      
      // Send to Google Sheets
      sendToGoogleSheets(String(user->name), 
                        String(user->rollNumber), 
                        String(user->className));
      
      delay(3000);
      displayMessage("System Ready", "Place Finger");
    } else {
      displayMessage("ID: " + String(fingerprintID), "Not Registered");
      Serial.println("Fingerprint found but user not registered!");
      delay(2000);
      displayMessage("System Ready", "Place Finger");
    }
  }
}

// ========== GET FINGERPRINT ID ==========
int getFingerprintID() {
  uint8_t p = finger.getImage();
  
  if (p != FINGERPRINT_OK) return -1;
  
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;
  
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    return finger.fingerID;
  }
  
  return -1;
}

// ========== SEND DATA TO GOOGLE SHEETS (FIXED FOR HTTPS) ==========
void sendToGoogleSheets(String name, String roll, String className) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;  // Changed from WiFiClient to WiFiClientSecure
    HTTPClient http;
    
    // Skip certificate validation for Google Apps Script
    // For production, consider using proper certificate validation
    client.setInsecure();
    
    // URL encode special characters
    name.replace(" ", "%20");
    roll.replace(" ", "%20");
    className.replace(" ", "%20");
    
    // Prepare URL with parameters
    String url = String(serverName) + "?name=" + name + 
                 "&roll=" + roll + 
                 "&class=" + className;
    
    Serial.println("Sending to Google Sheets...");
    Serial.println("URL: " + url);
    
    http.begin(client, url);  // Use secure client
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(15000);  // 15 second timeout
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
      
      lcd.setCursor(0, 1);
      lcd.print("Data Sent!      ");
      delay(1000);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      Serial.println("Troubleshooting:");
      Serial.println("1. Verify Google Apps Script URL is correct");
      Serial.println("2. Check script is deployed as 'Anyone' can access");
      Serial.println("3. Ensure WiFi connection is stable");
      
      lcd.setCursor(0, 1);
      lcd.print("Send Failed!    ");
      delay(1000);
    }
    
    http.end();
    delay(100);  // Allow WiFi stack to stabilize
  } else {
    Serial.println("WiFi Disconnected! Reconnecting...");
    connectWiFi();
  }
}
