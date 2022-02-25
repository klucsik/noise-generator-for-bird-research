/***************************************************
Noise generator for birds
Written by Krisztián Pál Klucsik
 ***************************************************/

#include "config.h" //change to change envrionment
Config conf;
#include "secrets.h"
Secrets sec;

static String name = conf.name;
static String ver = "0_16";

const String update_server = sec.update_server; // at this is url is the python flask update server, which I wrote
const String server_url = conf.server_url;
const String logcollector_url = conf.logcollector_url;

#define USE_SERIAL Serial

// pin configuration
#define mp3RxPin D1
#define mp3TxPin D5
#define i2cSDAPin D6
#define i2cSCLPin D7

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include "ArduinoJson.h"
#include "TimeLib.h"
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
ESP8266WiFiMulti WiFiMulti;
WiFiClient client;

#include "LittleFS.h"

#include <Wire.h>

#include <DS3232RTC.h> // https://github.com/JChristensen/DS3232RTC
DS3232RTC rtc;
int volume = 2;
static char* LOG_COLLCETOR_SSID = "logcollector-access-point";
/**********************************************
             Logging

***********************************************/
// content types
#define TRACK_PLAYED 2 // tracknumber-tracklength
#define PAUSE_TIME 3
#define RTC_NOW 4
#define INNER_NOW 5
#define NEW_PLAYPARAM_VER 6
#define START_UP 7 // version
#define SLEEP 9    // sleep minutes
#define CURRENT_PLAYPARAM 10
#define DF_PLAYER_MESSAGE 11
// MessageCodes:
//   errors:
#define DFPLAYER_START_ERROR 91
#define FILE_OPEN_ERROR 92
#define FILE_WRITE_FAIL 93
#define PLAYPARAM_INVALID 94
#define TRACKLENGTH_INVALID 95
#define FAILED_START 96

void saveLog(int messageCode, String additional)
{
  Serial.print("timestamp: " + String(now()));
  Serial.print(", messageCode: " + String(messageCode));
  Serial.println(", additional: " + additional);

  //send directly to server if available
  if (WiFi.status() == WL_CONNECTED && WiFi.SSID() != String(LOG_COLLCETOR_SSID))
  {
    String url = server_url + "/deviceLog/save?chipId=" +String(ESP.getChipId());
    String payload = "{\"timestamp\":" + String(now()) + ", \"messageCode\":\"" + String(messageCode) + "\", \"additional\":\"" + additional + "\"}";
    Serial.println(payload);
    int returned = POSTTask(url, payload);
  }
  else
  {
  // save to file
  additional.trim();
  File file = LittleFS.open(F("/logfile1.txt"), "a");
  file.print(String(now()) + "#" + String(messageCode) + "#" + additional + "\n");
  file.flush();
  file.close();
  }
}

void postLog(String logfilename, String unsentlogfilename)
{
  String payload;
  payload.reserve(1000);
  File file = LittleFS.open(logfilename, "r");
  File unsentLogs = LittleFS.open(unsentlogfilename, "a");
  while (file.available())
  {
    // read up a logline
    unsigned long timestamp = file.readStringUntil('#').toInt();
    Serial.print("timestamp: " + String(timestamp));
    int messageCode = file.readStringUntil('#').toInt();
    Serial.print(", messageCode: " + String(messageCode));
    String additional;
    additional.reserve(30);
    additional = file.readStringUntil('\n');
    additional.trim();
    Serial.println(", additional: " + additional);
    // send logline to server
    String collectorURL = logcollector_url + "/sendLog?chipId=" + String(ESP.getChipId()) + "&log=" + String(timestamp) + "#" + String(messageCode) + "#" + additional;
    String getreturn = GETTask(collectorURL);
    int returned = StatusOnlyGetTask(collectorURL);
    if (returned > 399 || returned < 0)
    {
      // if send fails, append the line to unsentlogs
      Serial.println("log send failed");
      unsentLogs.print(timestamp + "#" + String(messageCode) + "#" + additional + "\n");
      unsentLogs.flush();
    }
  }
  unsentLogs.close();
  file.close();
  // delete logfile
  LittleFS.remove(logfilename);
}

/**********************************************
             MP3 player functions

***********************************************/
SoftwareSerial mySoftwareSerial(mp3RxPin, mp3TxPin); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
String printDetail(uint8_t type, int value);

void startMp3(Stream &softSerial)
{
  if (!myDFPlayer.begin(softSerial))
  {
    USE_SERIAL.println(F("Unable to begin:"));
    USE_SERIAL.println(F("1.Please recheck the connection!"));
    USE_SERIAL.println(F("2.Please insert the SD card!"));
    saveLog(DFPLAYER_START_ERROR, "-");
    
  }
}
/**************end of section********************/

/**********************************************
             time functions

***********************************************/
// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
const int timeZone = 1;
WiFiUDP Udp;
unsigned int localPort = 8888;

void syncClock()
{
  if (WiFi.status() == WL_CONNECTED && WiFi.SSID() != String(LOG_COLLCETOR_SSID))
  {
    USE_SERIAL.println(F("Synchronizing inner clock..."));
    Udp.begin(localPort);
    setSyncProvider(getNtpTime);
    setSyncInterval(1);
    for (int i = 0; i < 10; i++)
    {
      delay(3000);
      Serial.println("waiting for ntp response");
      if (timeStatus() == timeSet)
      {
        break;
      }
    }
    rtc.set(now());
    setSyncInterval(3000);
    delay(100);
  }
  else
  {
    setSyncProvider(rtc.get);
  }
  void digitalClockDisplay();
  saveLog(INNER_NOW, String(now()));
  saveLog(RTC_NOW, String(rtc.get()));
};

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
}

const int NTP_PACKET_SIZE = 48;     // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0)
    ; // discard any previously received packets
  Serial.println(F("Transmit NTP Request"));
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500)
  {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE)
    {
      Serial.println(F("Receive NTP Response"));
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println(F("No NTP Response :-("));
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
/**************end of section********************/

int paramVersionHere = -2;

/*
checks if the daily paramteres need an update
*/
void syncParams()
{
  String resp = GETTask(server_url + "/devicePlayParamSelector/selectSlimPlayParam?chipId=" + ESP.getChipId() + "&paramVersion=" + String(paramVersionHere));

  if (resp.indexOf(F("playParams")) != -1)
  {
    // save to file
    File file = LittleFS.open(F("/playParams.json"), "w");
    if (!file)
    {
      USE_SERIAL.println(F("file open failed"));
      saveLog(FILE_OPEN_ERROR, "p");
    }
    else
    {

      int bytesWritten = file.print(resp);

      if (bytesWritten > 0)
      {
        Serial.println(F("File was written "));
        Serial.println(bytesWritten);
      }
      else
      {
        Serial.println(F("File write failed"));
        saveLog(FILE_WRITE_FAIL, "p");
      }

      file.close();
    }

    DynamicJsonDocument doc(5000);
    deserializeJson(doc, getParams());
    JsonObject object;
    paramVersionHere = doc["paramVersion"] | -1;
    saveLog(NEW_PLAYPARAM_VER, String(paramVersionHere));
  }
};

String getParams()
{
  File file = LittleFS.open("/playParams.json", "r");
  if (!file)
  {
    saveLog(FILE_OPEN_ERROR, "p");
    return "";
  }
  else
  {
    String data = file.readString();
    file.close();
    return data;
  }
};

/*
returns the object for the current hours play parameters (which tracks, pause times between tracks) from the global json object
*/
DynamicJsonDocument doc(5000); // 8knál már nem mennek a get requestek
JsonObject getPlayParams()
{
  Serial.println(F("Getplayparams"));

  int currentHour = hour(); // TODO: get time from the GSM module
  deserializeJson(doc, getParams());
  JsonObject object;
  paramVersionHere = doc["paramVersion"] | -1;
  volume = doc["vol"] | 15;
  USE_SERIAL.println("this hour is: " + String(currentHour));
  if (doc["playParams"].size() < 1)
  {
    Serial.println("No playUnits in PlayParam");
    return object;
  }
  for (int i = 0; i < 24; i++)
  {
    object = doc["playParams"][i];
    if (object["hour"] == currentHour)
      break;
  }

  USE_SERIAL.println(F("hourly config:"));
  serializeJsonPretty(object, USE_SERIAL);
  Serial.println();

  return object;
}

void syncTrackLength()
{

  String resp = GETTask(server_url + "/TrackPageSlim/page");
  if (resp.indexOf(F("tracklengths")) != -1)
  {
    // save to file
    File file = LittleFS.open(F("/trackLengths.json"), "w");
    if (!file)
    {
      saveLog(FILE_OPEN_ERROR, "t");
    }
    else
    {

      int bytesWritten = file.print(resp);

      if (bytesWritten > 0)
      {
      }
      else
      {
        saveLog(FILE_WRITE_FAIL, "t");
      }

      file.close();
    }
  }
  else
  {
    saveLog(TRACKLENGTH_INVALID, "t");
  }
}

/*
returns the length in seconds of the given tracknumber. If it doesn't know, return -1
*/
int getTrackLength(int tracknumber)
{
  JsonObject lengthObject;
  File file = LittleFS.open(F("/trackLengths.json"), "r");
  if (!file)
  {
    saveLog(FILE_OPEN_ERROR, "t");
    return 120;
  }
  else
  {
    String data = file.readString();
    file.close();
    // Serial.println(data);
    DynamicJsonDocument docPuffer(4000);
    deserializeJson(docPuffer, data);

    for (int i = 0; i < 100; i++)
    {
      lengthObject = docPuffer["tracklengths"][i];
      // USE_SERIAL.println("Processing tracklength record:" );
      // serializeJsonPretty(lengthObject,USE_SERIAL);
      if (lengthObject["trackNumber"] == tracknumber)
        break;
    }
    USE_SERIAL.println("tracklength obtained: ");
    serializeJsonPretty(lengthObject["length"], USE_SERIAL);
    Serial.println();
    return lengthObject["length"];
  }
}

String printDetail(uint8_t type, int value)
{
  String message = "";

  switch (type)
  {
  case TimeOut:
    message = F("Time Out!");
    break;
  case WrongStack:
    message = F("Stack Wrong!");
    break;
  case DFPlayerCardInserted:
    message = F("Card Inserted!");
    break;
  case DFPlayerCardRemoved:
    message = F("Card Removed!");
    break;
  case DFPlayerCardOnline:
    message = F("Card Online!");
    break;
  case DFPlayerUSBInserted:
    message = F("USB Inserted!");
    break;
  case DFPlayerUSBRemoved:
    message = F("USB Removed!");
    break;
  case DFPlayerPlayFinished:
    message = "Number: " + String(value) + " Play Finished!";
    break;
  case DFPlayerError:
    message = F("DFPlayerError:");
    switch (value)
    {
    case Busy:
      message = message + F("Card not found");
      break;
    case Sleeping:
      message = message + F("Sleeping");
      break;
    case SerialWrongStack:
      message = message + F("Get Wrong Stack");
      break;
    case CheckSumNotMatch:
      message = message + F("Check Sum Not Match");
      break;
    case FileIndexOut:
      message = message + F("File Index Out of Bound");
      break;
    case FileMismatch:
      message = message + F("Cannot Find File");
      break;
    case Advertise:
      message = message + F("In Advertise");
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
  return message;
}

void logDFPlayerMessage()
{
  if (myDFPlayer.available())
  {
    if (myDFPlayer.readType() == DFPlayerError)
    {
      saveLog(DF_PLAYER_MESSAGE, String(myDFPlayer.readState()) + "-" + myDFPlayer.readType() + "-" + myDFPlayer.read());
    }
  }
}

boolean hourlySetupFlag = false;

/**********************************************
             webupdate functions - only works with WiFi

***********************************************/
void updateFunc(String Name, String Version) // TODO: documentation
{
  HTTPClient http;

  String url = update_server + "/check?" + "name=" + Name + "&ver=" + Version;
  USE_SERIAL.print("[HTTP] check at " + url);
  if (http.begin(client, url))
  { // HTTP

    USE_SERIAL.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();
    delay(10000); // wait for bootup of the server
    httpCode = http.GET();
    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        String payload = http.getString();
        USE_SERIAL.println(payload);
        if (payload.indexOf("bin") > 0)
        {
          LittleFS.end();
          httpUpdateFunc(update_server + payload);
        }
      }
    }
    else
    {
      USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}

void httpUpdateFunc(String update_url) // this is from the core example
{
  if ((WiFiMulti.run() == WL_CONNECTED))
  {

    // The line below is optional. It can be used to blink the LED on the board during flashing
    // The LED will be on during download of one buffer of data from the network. The LED will
    // be off during writing that buffer to flash
    // On a good connection the LED should flash regularly. On a bad connection the LED will be
    // on much longer than it will be off. Other pins than LED_BUILTIN may be used. The second
    // value is used to put the LED on. If the LED is on with HIGH, that value should be passed
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

    // Add optional callback notifiers
    ESPhttpUpdate.onStart(update_started);
    ESPhttpUpdate.onEnd(update_finished);
    ESPhttpUpdate.onProgress(update_progress);
    ESPhttpUpdate.onError(update_error);

    t_httpUpdate_return ret = ESPhttpUpdate.update(client, update_url);
    // Or:
    // t_httpUpdate_return ret = ESPhttpUpdate.update(client, "server", 80, "file.bin");

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
      USE_SERIAL.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      USE_SERIAL.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      USE_SERIAL.println("HTTP_UPDATE_OK");
      break;
    }
  }
}

void update_started()
{
  USE_SERIAL.println("CALLBACK:  HTTP update process started");
}

void update_finished()
{
  USE_SERIAL.println("CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total)
{
  USE_SERIAL.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err)
{
  USE_SERIAL.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}
/**************end of section********************/

/**********************************************
             wifi http calls

***********************************************/
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>

String GETTask(String url)
{
  WiFiClient client;
  HTTPClient http;
  if (http.begin(client, url))
  {
    USE_SERIAL.print(F("[HTTP] GET "));
    USE_SERIAL.println(url);

    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
      if (httpCode == 302)
      {
        String redirectUrl = http.getLocation();
        http.end();
        return redirectUrl;
      }
      else
      {
        String payload = http.getString();
        USE_SERIAL.println(payload);
        http.end();

        return payload;
      }
    }
    else
    {
      USE_SERIAL.print(F("[HTTP] GET... failed, error: "));
      USE_SERIAL.println(httpCode);
      http.end();
      return "";
    }

    http.end();
  }
  else
  {
    USE_SERIAL.println(F("[HTTP] Unable to connect"));
    return "";
  }
  return "";
};

int StatusOnlyGetTask(String url)
{
  WiFiClient client;
  HTTPClient http;
  if (http.begin(client, url))
  {
    USE_SERIAL.print(F("[HTTP] GET (statuscode only) "));
    USE_SERIAL.println(url);

    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
      return httpCode;
    }
    else
    {
      USE_SERIAL.print(F("[HTTP] GET... failed, error: "));
      USE_SERIAL.println(httpCode);
      http.end();
      return 998;
    }

    http.end();
  }
  else
  {
    USE_SERIAL.println(F("[HTTP] Unable to connect"));
    return 999;
  }
}

int POSTTask(String url, String payload) // Make a post request
{
  WiFiClient client;
  HTTPClient http;
  if (http.begin(client, url))
  {
    Serial.print(F("[HTTPS] POST "));
    Serial.print(url);
    Serial.print(" --> ");
    Serial.println(payload);
    http.addHeader(F("Content-Type"), F("application/json"));

    int httpCode = http.POST(payload);

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      Serial.print(F("[HTTPS] POST... code: "));
      Serial.println(httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        String payload = http.getString();
        http.end();
        return httpCode;
      }
    }
    else
    {
      Serial.print(F("[HTTPS] POST... failed, error: "));
      Serial.println(httpCode);
      Serial.println(" " + http.getString());
      http.end();
      return httpCode;
    }

    http.end();
  }
  else
  {
    Serial.println(F("[HTTPS] Unable to connect"));
    return 999;
  }
  return 999;
};

/**************end of section********************/

/**********************************************
             Main arduino functions

***********************************************/

void setup()
{
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  Serial.println();
  Serial.print(F("name: "));
  Serial.print(name);
  Serial.print(F(" ver: "));
  Serial.print(ver);
  Serial.print(F(" deviceID: "));
  Serial.println(ESP.getChipId());
  Wire.begin(i2cSDAPin, i2cSCLPin);
  rtc.begin();
  digitalClockDisplay(); // show thwe current time in the terminal
  startMp3(mySoftwareSerial);
  LittleFS.begin();
 if (ESP.getResetReason() != "Deep-Sleep Wake")
  { // kézi újraindításnál, vagy power cyclenél
    Serial.println(F("Showing of sound skills for humans"));
    myDFPlayer.volume(2);
    myDFPlayer.play(1);
  }
  int numberOfNetworks = WiFi.scanNetworks();
  delay(100);
  if (numberOfNetworks > 0)
  {
    for (int i = 0; i < numberOfNetworks; i++)
    {
      Serial.println(WiFi.SSID(i));
    }
    delay(100);
    // connect to fix networks here with WiFiMulti. This will ovberride the saved network in wifiManager, so only uncomment this when its okay to reconnect with wifimanager on every boot
    WiFi.mode(WIFI_STA);
    // WiFiMulti.addAP("ssid","pass"); //office
    WiFiMulti.addAP(LOG_COLLCETOR_SSID, "testpass"); // logcollector
    WiFiMulti.addAP(sec.SSID_1,sec.pass_1);
    WiFiMulti.addAP(sec.SSID_2,sec.pass_2);
    WiFiMulti.run();
    // delay(1000);
    if (WiFi.status() != WL_CONNECTED)
    {
      WiFiManager wifiManager;
      wifiManager.setTimeout(60); // TODO: prepare the device for offline working
      wifiManager.autoConnect("birdnoise_config_accesspoint");
    }
    WiFiMulti.run(5000);
    delay(1000); // give somte time for the Wifi connection
  }
  syncClock();

  saveLog(START_UP, ver);
  if (ESP.getResetReason() != "Deep-Sleep Wake")
  { // kézi újraindításnál, vagy power cyclenél
    syncTrackLength();
    syncParams();
    delay(1000);
    syncTrackLength(); // tuti, ami biztos
    delay(1000);
    syncParams();
  }
  if(ESP.getResetReason() == "Exception" && ESP.getResetReason() == "Hardware Watchdog" && ESP.getResetReason() == "Software Watchdog"){
    saveLog(FAILED_START,String(ESP.getResetInfo()));
  }
  myDFPlayer.stop();
}

void loop()
{
  Serial.println(F("begining of main loop"));
delay(1000); //if there is no track, there will be a lot of logs. This will prevent to make logs every ms or so.

  // Maintain WiFi connection
  if (WiFiMulti.run(5000) == WL_CONNECTED)
  {
    Serial.print("WiFi connected: ");
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.localIP());
  }

  delay(1000); // if there is no track, there will be a lot of logs. This will prevent to make logs every ms or so.
  if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == String(LOG_COLLCETOR_SSID) )
  {
    postLog("/logfile1.txt", "/logfile2.txt");
    postLog("/logfile2.txt", "/logfile1.txt");
  }

  if (minute() < 5)
  {
    if (!hourlySetupFlag) // just once do the hourly stuff
    {
      syncClock();
      updateFunc(name, ver); // checking update
      syncParams();
      delay(1000);
      syncTrackLength();
      saveLog(CURRENT_PLAYPARAM, String(paramVersionHere));
      hourlySetupFlag = true;
    }
  }
  else
  {
    hourlySetupFlag = false;
  }

  // main business logic

  JsonObject thisHourParams = getPlayParams();

  int tracksize = thisHourParams["tracks"].size();
  if (tracksize < 1)
  {
    long long sleeptime = (59 - minute() + 2) * 60 * 1000; //wait until next hour first minutes
    char str[256];
    sprintf(str, "%lld", sleeptime);
    if (sleeptime > 0LL && sleeptime < 3600000LL)
    {
      saveLog(SLEEP, String(59 - minute()));
      delay(sleeptime);
    }
    else if (sleeptime < 0LL)
    {
      delay(300000); // az óra első 5 percében van ez az eset.
    }
  }

  Serial.printf("free heap size: %u\n", ESP.getFreeHeap());
  // startMp3(mySoftwareSerial); //Ettől recseg
  int currentTrack = thisHourParams["tracks"][random(0, tracksize)];
  int minT = thisHourParams["minT"];
  int maxT = thisHourParams["maxT"];
  int tracklength = getTrackLength(currentTrack);
  saveLog(TRACK_PLAYED, String(currentTrack) + "-" + String(tracklength));
  myDFPlayer.volume(volume);
  myDFPlayer.play(currentTrack);
  delay(tracklength / 2 * 1000); // wait until the half of the track
  logDFPlayerMessage();
  delay(tracklength / 2 * 1000); // wait until the end of the track
  logDFPlayerMessage();

  // calculate pause between tracks:
  long calculatedDelay = random(minT, maxT) * 1000;
  saveLog(PAUSE_TIME, String(calculatedDelay / 1000));
  delay(calculatedDelay);
}