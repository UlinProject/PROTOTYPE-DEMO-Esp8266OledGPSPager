
#include <NTPClient.h>
#include <WiFiUdp.h>

const String daysOfTheWeek[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

struct ATime {
  WiFiUDP antpUDP = WiFiUDP();
  NTPClient atimeClient = NTPClient(antpUDP);

  ATime() { // TODO
  }

  ATime(const char* ntp_url, const int utc_offset) {
    this->begin(ntp_url, utc_offset);
  }

  const void begin(const char* ntp_url, const int utc_offset) {
    //this->antpUDP = WiFiUDP();
    //this->atimeClient = NTPClient(&this->antpUDP, ntp_url, utc_offset);
    this->atimeClient.setPoolServerName(ntp_url);
    this->atimeClient.setTimeOffset(utc_offset);

    this->atimeClient.begin();
  }

  const inline bool force_update(void) {
    return this->atimeClient.forceUpdate();
  }

  static const unsigned int inline get_offset0to_seconds(const uint8_t hours, const unsigned int minuts, const unsigned int seconds) {
    return ((unsigned int)hours *60*60) + (minuts *60) + seconds;
  }

  const unsigned int inline get_offsetday_seconds(void) {
    const uint8_t h = this->atimeClient.getHours();
    const uint8_t m = this->atimeClient.getMinutes();
    const uint8_t s = this->atimeClient.getSeconds();
  
    return ATime::get_offset0to_seconds(h, m, s);
  }

  const String get_current_daystr() {
    return daysOfTheWeek[this->getDay()];
  }

  const unsigned int getDay() {
    return this->atimeClient.getDay();
  }

  const uint8_t getHours() {
    return this->atimeClient.getHours();
  }

  const uint8_t getMinutes() {
    return this->atimeClient.getMinutes();
  }

  const uint8_t getSeconds() {
    return this->atimeClient.getSeconds();
  }

  static const unsigned int inline get_minoffsetday_seconds(void) {
    return 0;
  }
  
  static const unsigned int inline get_maxoffsetday_seconds(void) {
    const unsigned int h = 24;
    const unsigned int m = 0;
    const unsigned int s = 0;
  
    return ATime::get_offset0to_seconds(h, m, s);
  }
};


struct ATimeData {
  unsigned int d;
  uint8_t h;
  uint8_t m;
  uint8_t s;

  unsigned int old_millis;
  unsigned int offsetday_seconds;

  ATimeData() {
    this->old_millis = 0;
  }

  ATimeData(ATime *atimeClient) {
    this->old_millis = 0;
    this->force_update(atimeClient);
  }

  const unsigned int getDay() {
    return this->d;
  }

  const uint8_t getHours() {
    return this->h;
  }

  const uint8_t getMinutes() {
    return this->m;
  }

  const uint8_t getSeconds() {
    return this->s;
  }

  const String get_current_daystr() {
    return daysOfTheWeek[this->getDay()];
  }

  const unsigned int getOffsetDay() {
    return this->offsetday_seconds;
  }

  const unsigned int get_offsetday_seconds() {
    return this->offsetday_seconds;
  }
  

  const bool /*inline*/ update(ATime *atimeClient) {
    unsigned int current_millis = millis();
    if ((int)current_millis - this->old_millis >= 1000) {
      this->old_millis = current_millis;
      this->__force_update(atimeClient);
      
      //this->old_millis = current_millis - (this->s/1000);
      return true;
    }
    
    return false;
  }

  const void inline force_update(ATime *atimeClient) {
    this->__force_update(atimeClient);
    this->old_millis = millis() - (this->s/1000);
  }

  const void inline __force_update(ATime *atimeClient) {
    this->d = atimeClient->getDay();
    this->h = atimeClient->getHours();
    this->m = atimeClient->getMinutes();
    this->s = atimeClient->getSeconds();
  
    this->offsetday_seconds = atimeClient->get_offset0to_seconds(
      this->h, 
      this->m, 
      this->s
    );
  }
};
