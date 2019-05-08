#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

//#define debug

// display settings
int pinCS = 4; // Attach display CS to this pin (GPIO4), DIN to MOSI (GPIO13) and CLK to SCK (GPIO14)
int numberOfHorizontalDisplays = 8;
int numberOfVerticalDisplays = 1;

Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

// running string settings
int spacer = 1; // beetwen chars
int width = 5 + spacer; // the font width is 5 pixels
int width1 = 0 - width; // need to draw escaping char
String tape = "File read error"; // main tape in utf8
String tape1 = ""; // working tape in cp1251
unsigned long waitTape = 50; // delay beetwen tape shift in milliseconds
int counterTape = 0;
unsigned long previousMillisTape = 0;

// interval at which to draw connection sign in milliseconds
const unsigned long interval = 300;

// webserver settings
ESP8266WebServer server(80);

// wi-fi hotspot settings
#define AP_HOST "ESP"
#define AP_password "thereisno"

// button settings
int keyPin = 5; // attached to GPIO5
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
int keyFlag = LOW;
int keyState = HIGH;
int lastKeyState = HIGH;

// handle button without delay
void handleKey() {
  if (keyFlag == LOW) {
    int keyReading = digitalRead(keyPin);
    if (keyReading != lastKeyState) {
      lastDebounceTime = millis();    
    }
    if (millis() - lastDebounceTime > debounceDelay) {
      if (keyReading != keyState) {
        keyState = keyReading;
        if (keyState == LOW) {
          keyFlag = HIGH;
          #ifdef debug
            Serial.println("key pressed");
          #endif
        }          
      }
    }
    lastKeyState = keyReading;
  }
}

// converts utf8 cyrillic to cp1251
String utf8ukr(String source) {
  int i,k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };

  k = source.length();
  i = 0;

  while (i < k) {
    n = source[i];
    i++;

    if (n >= 0xC0) { // 0xC0 = B1100 0000
      switch (n) {
        case 0xD0: { // 0xD0 = B1101 0000
          n = source[i];
          i++;
          if (n == 0x81) {
            n = 0xA8; // Ё
            break;
          }
          if (n == 0x84) {
            n = 0xAA; // Є
            break;
          }
          if (n == 0x86) {
            n = 0xB1; // І
            break;
          }
          if (n == 0x87) {
            n = 0xAF; // Ї
            break;
          }
          if (n >= 0x90 && n <= 0xBF) { // from А to п
            n += 0x2F;
          }
          break;
        }
        case 0xD1: { // 0xD1 = B1101 0001
          n = source[i];
          i++;
          if (n == 0x91) {
            n = 0xB8; // ё
            break;
          }
          if (n == 0x94) {
            n = 0xB9; // є
            break;
          }
          if (n == 0x96) {
            n = 0xB2; // і
            break;
          }
          if (n == 0x97) {
            n = 0xBE; // ї
            break;
          }
          if (n >= 0x80 && n <= 0x8F) { // from р to я
            n += 0x6F;
          }
          break;
        }
      }
    }
    m[0] = n;
    target += String(m);
  }
  return target;
}

void handleRoot() {
  // catch user response
  if (server.hasArg("tape")) {
    tape = server.arg("tape");
      #ifdef debug
        Serial.print("new tape: ");
        Serial.println(tape);
      #endif 
  }
  // generate HTML page  
  String page = "<html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
  <title>Tape setting</title></head><body><p>Current tape:</p><p>";
  page += tape;
  page += "</p><form action=\"\" method=\"post\"><input type=\"text\" name=\"tape\" size=50><input type=\"submit\" value=\"Submit\"/></form></body></html>";  
  // send page
  server.send(200, "text/html", page);  
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  #ifdef debug
    Serial.println(message);
  #endif
  server.send(404, "text/plain", message);
}

boolean startSoftAPServer() {
  #ifdef debug
    Serial.println("Setting SoftAP");
    Serial.setDebugOutput(true);
  #endif
  WiFi.mode(WIFI_AP);
  // sets uniq hostname and password
  String softApSSID(AP_HOST);
  softApSSID += String(ESP.getChipId(), HEX);
  String softApPassword(AP_password);
  softApPassword += String(ESP.getChipId(), HEX);
  boolean result = WiFi.softAP(softApSSID.c_str(), softApPassword.c_str());
  if (result) {
    #ifdef debug
      Serial.println("SoftAP ready");
      IPAddress myIP = WiFi.softAPIP();    
      Serial.print("AP IP address: ");
      Serial.println(myIP);
      Serial.print("wifi mode: ");
      Serial.println(WiFi.getMode());
      Serial.print("SSID: ");
      Serial.println(softApSSID);
      Serial.print("password: ");
      Serial.println(softApPassword);
      Serial.println();
      WiFi.printDiag(Serial);
      Serial.println();
    #endif
    // clear screen, print AP_HOST name
    matrix.fillScreen(LOW);
    matrix.setCursor(8,0);
    matrix.print(softApSSID);
    matrix.write();
    // mDNS start
    MDNS.begin(AP_HOST);
    #ifdef debug
      Serial.println("mDNS started");
    #endif    
    // start web server
    server.on("/", handleRoot);
    server.onNotFound(handleNotFound);
    server.begin();
    #ifdef debug
      Serial.println("HTTP server started");
    #endif    
  }
  else {
    #ifdef debug
      Serial.println("SoftAP start failed!");
      Serial.setDebugOutput(false);
    #endif
  }
  return result;
}

void stopSoftAPServer() {
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  //delay(100);
  server.stop();
  #ifdef debug
    Serial.println("HTTP server stopped");
    Serial.print("wifi mode: ");
    Serial.println(WiFi.getMode());
    Serial.setDebugOutput(false);
  #endif
}

void userInterract() {
  #ifdef debug
    Serial.println("user interact started");
  #endif
  // clear button flag
  keyFlag = LOW;  
  // start soft AP + web server
  if (startSoftAPServer() == false) {
    #ifdef debug
      Serial.println("SoftAP init failed");
      Serial.print("wifi mode: ");
      Serial.println(WiFi.getMode());
    #endif
    return;
  }
  // display connection sign and handle client
  unsigned long previousMillis = 0;  
  int signState = 0;
  while (keyFlag == LOW) {
    // display connection sign without delay
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      // save the last time you change state of sign
      previousMillis = currentMillis;
      // light decision
      switch (signState) {
        case 0: // no sign
          matrix.drawPixel(3,6,LOW);
          matrix.drawPixel(1,5,LOW);
          matrix.drawPixel(2,4,LOW);
          matrix.drawPixel(3,4,LOW);
          matrix.drawPixel(4,4,LOW);
          matrix.drawPixel(5,5,LOW);
          matrix.drawPixel(0,3,LOW);
          matrix.drawPixel(1,2,LOW);
          matrix.drawPixel(2,2,LOW);
          matrix.drawPixel(3,2,LOW);
          matrix.drawPixel(4,2,LOW);
          matrix.drawPixel(5,2,LOW);
          matrix.drawPixel(6,3,LOW);
          signState++;
          break;
        case 1: // dot
          matrix.drawPixel(3,6,HIGH);
          signState++;
          break;
        case 2: // first umbrella
          matrix.drawPixel(1,5,HIGH);
          matrix.drawPixel(2,4,HIGH);
          matrix.drawPixel(3,4,HIGH);
          matrix.drawPixel(4,4,HIGH);
          matrix.drawPixel(5,5,HIGH);
          signState++;
          break;
        case 3: // second umbrella
          matrix.drawPixel(0,3,HIGH);
          matrix.drawPixel(1,2,HIGH);
          matrix.drawPixel(2,2,HIGH);
          matrix.drawPixel(3,2,HIGH);
          matrix.drawPixel(4,2,HIGH);
          matrix.drawPixel(5,2,HIGH);
          matrix.drawPixel(6,3,HIGH);
          signState = 0;
          break;
        default:
          break;
      }
      // display sign
      matrix.write();
    }    
    // interract with user from web browser
    server.handleClient();
    delay(15);
    // update MDNS
    MDNS.update();
    // handle button
    handleKey();
  }
  // stop soft AP + web server
  stopSoftAPServer ();
  // clear display
  matrix.fillScreen(LOW);
  matrix.write();
  // start running string from begin
  counterTape = 0;
  // save tape to file
  File f = SPIFFS.open("/tape.txt", "w");
  f.print(tape);  
  f.close();
  tape1 = utf8ukr(tape);
  // clear button flag
  keyFlag = LOW;
  #ifdef debug
    Serial.println("user interact stopped");
  #endif
}

// handle running string without delay
void handleTape() {
  unsigned long currentMillisTape = millis();
  if (currentMillisTape - previousMillisTape >= waitTape) {    
    previousMillisTape = currentMillisTape;
    matrix.fillScreen(LOW);
    unsigned int letter = counterTape / width;
    int x = (matrix.width() - 1) - counterTape % width;
    int y = (matrix.height() - 8) / 2; // center the text vertically
    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( x > width1 && letter < tape1.length() ) {
        matrix.drawChar(x, y, tape1[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= width;
    }
    matrix.write(); // Send bitmap to display
    counterTape++;
    if (counterTape >= width * tape1.length() + matrix.width() - 1 - spacer) {
      counterTape = 0;
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  #ifdef debug 
    Serial.begin(115200);
    Serial.println();
  #endif

  SPIFFS.begin();
  File f = SPIFFS.open("/tape.txt", "r");
  
  if (!f) {
    File f = SPIFFS.open("/tape.txt", "w");
    f.print("Demo string - please change me!");
  } else {
    tape = f.readString();    
  }
  f.close();
  
  tape1 = utf8ukr(tape);
  
  // init displays
  for (int i = 0; i < numberOfHorizontalDisplays; i++) {
    matrix.setPosition(i, i, 0); // all displays stays in one row
    matrix.setRotation(i, 1);    // all displays position is 90 degrees clockwise
  }

  // init button pin
  pinMode(keyPin, INPUT_PULLUP);

  // stop wifi
  WiFi.mode(WIFI_OFF);

  #ifdef debug        
    Serial.print("free memory: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("flash size: ");
    Serial.println(ESP.getFlashChipRealSize());
    Serial.println("Done with setup");
  #endif  
}

void loop() {
  // put your main code here, to run repeatedly:
  handleTape();
  handleKey();
  if (keyFlag == HIGH) {
    userInterract(); // key pressed
  }
}
