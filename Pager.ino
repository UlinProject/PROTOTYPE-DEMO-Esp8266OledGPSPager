
// #UlinProject 22

#include "all_pins.h"
#include <TinyGPS++.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include "SH1106Wire.h"`
//#include "atime.h"
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <ESP8266HTTPUpdateServer.h>

#define ssid "SalutePoint"
#define password "123456qwerty"
#define host "http://openshiftmaster-denis2005991-dev.apps.sandbox.x8i5.p1.openshiftapps.com"

String channel_name;
String nickname;
bool is_nickname_update;
bool is_update_linepanel;
bool is_update_gps;
bool is_blink_gps;

bool is_connected_gps;
bool is_update_messages;

unsigned int old_update_messages = 0;
unsigned int old_update_channels = 0;
uint8_t c_bytes;

String old_time;

String m_creator;
String m_body;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
DynamicJsonBuffer jsonBuffer(24000);
TinyGPSPlus gps;
SoftwareSerial ss(D5, D6);

//ATime atime;
SH1106Wire display(0x3c, SDA, SCL);

void setup() {
  Serial.begin(115200);
  ss.begin(9600);
  while (!Serial || !ss) {
    yield();
  }
  yield();
  Serial.println("OkLoad");

  WiFi.persistent(false);
  nickname = "";
  old_time = "";
  channel_name = "";
  is_connected_gps = false;
  is_nickname_update = false;
  is_update_linepanel = true;
  is_update_messages = false;
  is_update_gps = true;
  
  display.init();
  display.clear();
  display.flipScreenVertically();
  display.setContrast(255);

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.drawString(display.getWidth() / 2, display.getHeight() / 2 - 10, "WI-FI Connecting...");
  display.display();
  yield();
  
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.hostname("SaluteHALClient");
  WiFi.setOutputPower(20.5);
  WiFi.begin ( ssid, password );
  
  unsigned int c = 1;
  while ( WiFi.status() != WL_CONNECTED ) {
    c_bytes = 40;
    while (c_bytes > 0 && ss.available() > 0) {
      if (gps.encode(ss.read())) {
        displayInfo();
      }
      yield();
      c_bytes = c_bytes - 1;
    }
    yield();
    
    display.clear();
    display.drawString(display.getWidth() / 2, display.getHeight() / 2 - 10, "WI-FI Connecting... " + (String)c + "s");
    display.drawString(display.getWidth() / 2, display.getHeight() / 2 + 10, "name: " + (String)ssid);
    display.drawString(display.getWidth() / 2, display.getHeight() / 2 + 20, "password: " + (String)password);
    display.display();
    
    c += 1;
    yield();
    delay ( 1000 );
  }
  display.clear();

  reset_settings_panel();
  clear_data();

  is_nickname_update = true;
  nickname = WiFi.localIP().toString();
  full_yield();
  
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.drawString(display.getWidth() / 2, display.getHeight() / 2, "Wait OTA 0.2...");
  if (!full_yield()) {
    update_display();
  }

  httpUpdater.setup(&httpServer);
  httpServer.begin();
  yield();

  uint8_t t_wait_update = 6;
  while (t_wait_update != 0) {
    c_bytes = 40;
    while (c_bytes > 0 && ss.available() > 0) {
      if (gps.encode(ss.read())) {
        displayInfo();
      }
      yield();
      c_bytes = c_bytes - 1;
    }
    yield();
  
    httpServer.handleClient();
    yield();
    delay(1000);
    httpServer.handleClient();
    t_wait_update -= 1;
  }

  clear_data();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.drawString(display.getWidth() / 2, display.getHeight() / 2, "OTA 0.2 Ignore...");
  if (!full_yield()) {
    update_display();
  }
  delay(1000);
  

  /*
  display.clear();
  display.drawString(display.getWidth() / 2, display.getHeight() / 2 - 10, "NTP Connecting...");
  display.drawString(display.getWidth() / 2, display.getHeight() / 2 + 10, "url: 0.openwrt.pool.ntp.org");
  display.display();
  yield();
  
  atime.begin("0.openwrt.pool.ntp.org", 3600 + 3600 + 3600);
  
  while (!atime.force_update()) {
    delay(1000);
    yield();
  }*/

  c_bytes = 40;
  while (c_bytes > 0 && ss.available() > 0) {
    if (gps.encode(ss.read())) {
      displayInfo();
    }
    yield();
    c_bytes = c_bytes - 1;
  }
  yield();

  is_nickname_update = true;
  nickname = "Loading...";
  full_yield();

  while (true) { // CHANNELS
    clear_data();
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, "Get channels...");
    if (!full_yield()) {
      update_display();
    }

    yield();
    httpServer.handleClient();
    yield();

    if (update_channels()){
      delay(3000);
      break;
    }else {
      clear_data();
      display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
      display.drawString(display.getWidth() / 2, display.getHeight() / 2, "Invalid Get channels\nreset...");
      if (!full_yield()) {
        update_display();
      }
    }

    yield();
    httpServer.handleClient();
    yield();
    
    delay(3000);
  }
  clear_data();
  full_yield();

  while (true) { // MESSAGES
    clear_data();
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, "Get messages...");
    if (!full_yield()) {
      update_display();
    }

    yield();
    httpServer.handleClient();
    yield();

    if (update_messages()){
      delay(3000);
      break;
    }else {
      clear_data();
      display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
      display.drawString(display.getWidth() / 2, display.getHeight() / 2, "Invalid Get messages\nreset...");
      if (!full_yield()) {
        update_display();
      }
    }

    yield();
    httpServer.handleClient();
    yield();
    
    delay(3000);
  }
  clear_data();
  full_yield();
}

void reset_settings_panel() {
  is_update_linepanel = true;
  is_nickname_update = true;
  is_update_gps = true;
  is_blink_gps = true;
  is_connected_gps = false;
  old_time = "";
}

bool update_channels() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
  
    if (http.begin(client, (String)host + "/channels")) {
      http.addHeader("Content-Type", "text/html");
      http.setTimeout(24000);
      int httpCode = http.GET();
      Serial.println(httpCode);
    
      if(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        jsonBuffer.clear();
        String message = http.getString();
        JsonArray& root = jsonBuffer.parseArray(message);
        if (root.success()) {
          if (root.size() > 0) {
            String name = (String)root[0]["displayName"]; 
      
            old_update_channels = millis();
            channel_name = name;
            nickname = name;
            is_nickname_update = true;
    
            http.end();
            return true;
          }
        }
      }
      http.end();
    }
  }
  return false;
}

bool update_messages() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    if (http.begin(client, (String)host + "/read")) {
      http.addHeader("Content-Type", "text/html");
      http.setTimeout(24000);
      
      int httpCode = http.GET();
      if(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        jsonBuffer.clear();
        String message = http.getString();
        JsonArray& root = jsonBuffer.parseArray(message);
    
        if (root.success()) {
          if (root.size() > 0) {
            root.printTo(Serial);
            
            m_body = (String)root[0]["body"]; 
            m_creator = (String)root[0]["creator"]; 
            
            //Serial.println(m_creator);
            //Serial.println(m_body);
      
            is_update_messages = true;
            old_update_messages = millis();
            
            http.end();
            return true;
          }
        }
      }
      http.end();
    }
  }
  return false;
}

bool send_gps_data(double lat, double lng) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    if(http.begin(client, (String)host + "/send?lat=" + String(lat, 8) + "&lng=" + String(lng, 8))) {
      http.setTimeout(24000);
      http.addHeader("Content-Type", "text/html");
      
      int httpCode = http.GET();
      if(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
      
        if(payload == "1") {
          http.end();
          return true;
        }
      }
      http.end();
    }
  }
  return false;
}

const uint8_t get_width_str(String *astr) {
  return astr->length() * 6;
}

const bool draw_panel() {
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  bool is_flush = false;
  bool is_ok = false;
  
  uint8_t x = 0;
  if (is_nickname_update) {
    is_nickname_update = false;
    is_flush = true;

    is_ok = true;
    is_update_gps = true;
    reset_xy_len(x, 0, display.getWidth() - 35 - x, 13);

    if (nickname != "") {
      uint8_t width_text = get_width_str(&nickname) + 2;
      reset_xy_len(x, 0, width_text + 4, 13);
      
      display.drawString(x, 0, nickname);
      x += width_text +2;
      
      display.drawVerticalLine(x, 0, 14);
      x += 2;
    }
  }else {
    uint8_t width_text = get_width_str(&nickname) + 2;
    x += width_text +2;
    x += 2;
  }

  if (is_update_gps) {
    is_update_gps = false;
    is_flush = true;
    is_ok = true;

    if (is_connected_gps) {
      String text = "GPS";
      uint8_t width_text = get_width_str(&text) + 2;
      reset_xy_len(x, 0, width_text + 4, 13);
      
      display.drawString(x, 0, text);
      x += width_text +3;
  
      display.drawVerticalLine(x, 0, 14);
      x += 2;
    }else {
      String text = "GPS";
      uint8_t end_x = get_width_str(&text) + 2 + 3 + 2;
      reset_xy_len(x, 0, end_x, 13);
    }
  }else {
    String text = "GPS";
    uint8_t width_text = get_width_str(&text) + 2;

    x += width_text +3;
    x += 2;
  }

  // CLEAR UNK
  if (is_flush) {
    reset_xy_len(x, 0, display.getWidth() - 35 - x, 13);
  }
  
  // TIME
  String atime_str = get_atime_str();
  if (atime_str != old_time) {
    old_time = atime_str;
    is_ok = true;
    
    reset_xy_len(display.getWidth() - 35, 0, 35, 10);
    display.drawVerticalLine(display.getWidth() - 35, 0, 14);
    display.drawString(display.getWidth() - 30, 0, atime_str);
  }

  // END LINE
  if (is_update_linepanel) {
    is_update_linepanel = false;
    is_ok = true;
    
    display.drawHorizontalLine(0, 13, display.getWidth());
    display.drawHorizontalLine(0, 14, display.getWidth());
  }

  return is_ok;
}

bool full_yield() {
  yield();
  if (draw_panel()) {
    update_display();

    return true;
  }

  return false;
}

void clear_data() {
  reset_xy_len(0, 15, display.getWidth(), display.getHeight()-14);
}

void update_display() {
  display.display();
}

void reset_xy_len(unsigned int x, unsigned int y, const unsigned int ixmax, const unsigned int iymax) {
  const unsigned int xmax = x + ixmax;
  const unsigned int ymax = y + iymax;
  
  while (x != xmax) {
    unsigned int cy = y;
    
    while (cy != ymax) {
      display.clearPixel(x, cy);
      cy += 1;
    }
    x += 1;
  }
}

const String get_atime_str() {
  uint8_t h = gps.time.hour(); //atime.getHours();
  uint8_t m = gps.time.minute();

  String out;
  if (10 > h) {
    out = "0" + (String)h;
  }else {
    out = (String)h;
  }

  if (10 > m) {
    out = out + ":0" + (String)m;
  }else {
    out = out + ":" + (String)m;
  }
  return out;
}

unsigned int old_loop_update = 0;
unsigned int old_update_location = 0;
double old_lat = 0;
double old_lng = 0;

void loop() {
  c_bytes = 40;
  while (c_bytes > 0 && ss.available() > 0) {
    if (gps.encode(ss.read())) {
      displayInfo();
    }
    yield();
    c_bytes = c_bytes - 1;
  }
  yield();
  httpServer.handleClient();
  yield();
  
  unsigned int c_millis = millis();
  if ((signed int)c_millis - old_update_location >= 60000) {
    old_update_location = c_millis;

    if (gps.location.isValid()) {
      double lat = gps.location.lat();
      double lng = gps.location.lng();

      if (old_lat != lat && old_lng != lng) {
        is_update_messages = true;
        if (send_gps_data(lat, lng)) {
          old_lat = lat;
          old_lng = lng;
        }
      }
    }

    full_yield();
  }

  
  if ((signed int)c_millis - old_loop_update >= 1000) {
    old_loop_update = c_millis;

    if (!is_connected_gps) {
      if (gps.location.isValid()) {
        is_blink_gps = false;
        is_connected_gps = true;
        is_update_gps = true;
      }
    } else {
      if (!is_blink_gps) {
        if (!gps.location.isValid()) {
          is_blink_gps = true;
          is_connected_gps = true;
          is_update_gps = true;
        }
      }
    }
    
    if (is_blink_gps) {
      is_connected_gps = !is_connected_gps;
      is_update_gps = true;
    }
    
  
    if (is_update_messages) {
      is_update_messages = false;
      clear_data();
  
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(5, 15 + 5, m_creator);
      display.drawStringMaxWidth(5, 15 + 5 + 10 + 2, display.getWidth() - 10, m_body);
      
      /*if (gps.location.isValid()) {
        display.drawString(display.getWidth() - 30, display.getHeight() - 10, 
          (String)((unsigned int)gps.location.lat()) + ":" + (String)((unsigned int)gps.location.lng()) 
        );
      }*/

      if (!full_yield()) {
        update_display();
      }
    }

    if (c_millis - old_update_messages >= 8000) {
      old_update_messages = c_millis;
      
      update_messages();
      full_yield();
    }
  
    if (c_millis - old_update_channels >= 15000) {
      old_update_channels = c_millis;
  
      update_channels();
      full_yield();
    }
    
    full_yield();
  }
  yield();
}

void displayInfo() {
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();  
}
