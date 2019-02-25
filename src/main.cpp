#include <Arduino.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <Private.h>
#include <Adafruit_NeoPixel.h>

#define PIN D7 // WEMOS D1 mini
#define NUMPIXELS 12

#define APP_NAME "NeoPixel12Clock"
#define APP_VERSION "0.1.3"
#define APP_AUTHOR "Dr. Thorsten Ludewig <t.ludewig@gmail.com>"

time_t now;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

uint8 pixelBuffer[NUMPIXELS][3];

void clearPixels()
{
  for (int i = 0; i < NUMPIXELS; i++)
  {
    pixelBuffer[i][0] = pixelBuffer[i][1] = pixelBuffer[i][2] = 0;
  }
}

void showPixels( int delayInMillis )
{
  for (int i = 0; i < NUMPIXELS; i++)
  {
    pixels.setPixelColor( i, pixelBuffer[i][0], pixelBuffer[i][1], pixelBuffer[i][2] );
  }

  pixels.show();

  if ( delayInMillis > 0 )
  {
    yield();
    delay( delayInMillis );
  }
}

void setup()
{
  pixels.begin();
  clearPixels();
  showPixels(0);
  Serial.begin(74880);
  delay(3000);
  Serial.println();
  Serial.println();
  Serial.println(F(APP_NAME ", Version " APP_VERSION ", by " APP_AUTHOR));
  Serial.println("Build date: " __DATE__ " " __TIME__);

  configTime(0, 0, "pool.ntp.org");

  WiFi.disconnect();
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int i = 0;
  while((now = time(nullptr)) < 1550922262 )
  {
    clearPixels();
    pixelBuffer[i++][2] = 100;
    i %= NUMPIXELS;
    showPixels(250);
  }

  Serial.println();
  Serial.println();
}

void setPixelBase( int base, int index, int maxColor, int value )
{
  value %= base;

  int basePixel = value / ( base / NUMPIXELS );
  int nextPixel = basePixel + 1;
  nextPixel %= NUMPIXELS;

  float basePixelValue = maxColor;
  float nextPixelValue = ((float)value) / (((float)base) / ((float)NUMPIXELS) );

  nextPixelValue -= basePixel;
  nextPixelValue *= maxColor;
  basePixelValue -= nextPixelValue;

  pixelBuffer[basePixel][index] = (uint8)basePixelValue;
  pixelBuffer[nextPixel][index] = (uint8)nextPixelValue;
}

void setHourPixel( tm *tm )
{
  int hourPixel = tm->tm_hour;
  hourPixel %= 12;
  pixelBuffer[hourPixel][2] = 90;
}

void setMinutePixel( tm *tm )
{
  setPixelBase( 60, 1, 90, tm->tm_min );
}

void setHourMinutePixel( tm *tm )
{
  clearPixels();
  setMinutePixel( tm );
  setHourPixel( tm );
}

void loop()
{
  now = time(nullptr);
  tm *tm = localtime( &now );

  for( int i = 0; i< 20; i++ )
  {
    setHourMinutePixel( tm );
    setPixelBase( 1200, 0, 140, ( tm->tm_sec * 20 ) + i );

    if ( tm->tm_sec == 0 && i < 12 )
    {
      pixelBuffer[i][0] = pixelBuffer[i][1] = pixelBuffer[i][2] = 128;
    }

    showPixels(50);
  }

  Serial.print("ctime:(UTC) ");
  Serial.print(ctime(&now));
}
