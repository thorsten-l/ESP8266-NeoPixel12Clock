#include <Arduino.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <Utils.hpp>
#include <Private.h>
#include <Adafruit_NeoPixel.h>

#define PIN D7
#define NUMPIXELS 12

#define TIMEZONE "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"
#define NTP_SERVER1 "ptbtime1.ptb.de"
#define NTP_SERVER2 "ptbtime2.ptb.de"
#define NTP_SERVER3 "de.pool.ntp.org"

#define APP_NAME "NeoPixel12Clock"
#define APP_VERSION "0.1.6"
#define APP_AUTHOR "Dr. Thorsten Ludewig <t.ludewig@gmail.com>"

time_t now;
struct tm appTimeinfo;
char appDateTimeBuffer[32];

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

uint8 pixelBuffer[NUMPIXELS][3];

void clearPixels()
{
  for (int i = 0; i < NUMPIXELS; i++)
  {
    pixelBuffer[i][0] = pixelBuffer[i][1] = pixelBuffer[i][2] = 0;
  }
}

const char *appDateTime()
{
  time_t now = time(nullptr);
  localtime_r(&now, &appTimeinfo);
  sprintf(appDateTimeBuffer, "%4d-%02hd-%02hd %02hd:%02hd:%02hd", appTimeinfo.tm_year + 1900, appTimeinfo.tm_mon + 1,
          appTimeinfo.tm_mday, appTimeinfo.tm_hour, appTimeinfo.tm_min, appTimeinfo.tm_sec);
  return appDateTimeBuffer;
}

void showPixels(int delayInMillis)
{
  for (int i = 0; i < NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, pixelBuffer[i][0], pixelBuffer[i][1], pixelBuffer[i][2]);
  }

  pixels.show();

  if (delayInMillis > 0)
  {
    yield();
    delay(delayInMillis);
  }
}

void setup()
{
  int bootdevice = getBootDevice();
  pinMode(BOARD_LED_PIN, OUTPUT);
  digitalWrite(BOARD_LED_PIN, 1 ^ BOARD_LED_ON);
  pixels.begin();
  clearPixels();
  showPixels(0);
  Serial.begin(115200);
  delay(3000);
  Serial.println();
  Serial.println();
  Serial.println(F(APP_NAME ", Version " APP_VERSION ", by " APP_AUTHOR));
  Serial.println("Build date: " __DATE__ " " __TIME__);
  Serial.print("Framework full version=");
  Serial.println(ESP.getFullVersion());
  Serial.println("PIOENV=" PIOENV ", PIOPLATFORM=" PIOPLATFORM ", PIOFRAMEWORK=" PIOFRAMEWORK);
  Serial.printf("boot device=%d\n", bootdevice);

  if (bootdevice == 1)
  {
    Serial.println("\nThis sketch has just been uploaded over the UART.");
    Serial.println("The ESP8266 will freeze on the first restart.");
    Serial.println("Press the reset button or power cycle the ESP");
    Serial.println("and operation will be resumed thereafter.");
  }

  Serial.println();
  Serial.println();
  showChipInfo();

  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(200);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(BOARD_LED_PIN, 1 ^ digitalRead(BOARD_LED_PIN));
    clearPixels();
    pixelBuffer[i++][2] = 100;
    i %= NUMPIXELS;
    showPixels(250);
    Serial.print(".");
  }

  Serial.println("\n\nWiFi connected to : " WIFI_SSID);
  Serial.printf("WiFi Channel      : %d\n", WiFi.channel());

  Serial.printf("WiFi Hostname     : %s\n", WiFi.hostname().c_str());
  Serial.print("WiFi IP-Address   : ");
  Serial.println(WiFi.localIP());
  Serial.print("WiFi Gateway-IP   : ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("WiFi Subnetmask   : ");
  Serial.println(WiFi.subnetMask());
  Serial.print("WiFi DNS Server   : ");
  Serial.println(WiFi.dnsIP());
  Serial.println();

  configTime(TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

  i = 0;
  while ((now = time(nullptr)) < 1550922262)
  {
    digitalWrite(BOARD_LED_PIN, 1 ^ digitalRead(BOARD_LED_PIN));
    clearPixels();
    pixelBuffer[i++][1] = 100;
    i %= NUMPIXELS;
    showPixels(250);
  }

  Serial.printf("Time              : %s\n", appDateTime());
  Serial.print("Timezone          : ");
  Serial.println(getenv("TZ"));

  digitalWrite(BOARD_LED_PIN, BOARD_LED_ON);

  Serial.println();
}

void setPixelBase(int base, int index, int maxColor, int value)
{
  value %= base;

  int basePixel = value / (base / NUMPIXELS);
  int nextPixel = basePixel + 1;
  nextPixel %= NUMPIXELS;

  float basePixelValue = maxColor;
  float nextPixelValue = ((float)value) / (((float)base) / ((float)NUMPIXELS));

  nextPixelValue -= basePixel;
  nextPixelValue *= maxColor;
  basePixelValue -= nextPixelValue;

  pixelBuffer[basePixel][index] = (uint8)basePixelValue;
  pixelBuffer[nextPixel][index] = (uint8)nextPixelValue;
}

void setHourPixel(tm *tm)
{
  int hourPixel = tm->tm_hour;
  hourPixel %= 12;
  pixelBuffer[hourPixel][2] = 90;
}

void setMinutePixel(tm *tm)
{
  setPixelBase(60, 1, 90, tm->tm_min);
}

void setHourMinutePixel(tm *tm)
{
  clearPixels();
  setMinutePixel(tm);
  setHourPixel(tm);
}

void loop()
{
  now = time(nullptr);
  tm *tm = localtime(&now);

  for (int i = 0; i < 20; i++)
  {
    setHourMinutePixel(tm);
    setPixelBase(1200, 0, 140, (tm->tm_sec * 20) + i);

    if (tm->tm_sec == 0 && i < 12)
    {
      pixelBuffer[i][0] = pixelBuffer[i][1] = pixelBuffer[i][2] = 128;
    }

    showPixels(50);
  }

  Serial.printf("Time : %s\n", appDateTime());
}
