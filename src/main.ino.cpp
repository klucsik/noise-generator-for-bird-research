# 1 "C:\\Users\\klucs\\AppData\\Local\\Temp\\tmphtmndyil"
#include <Arduino.h>
# 1 "D:/PR/okosodu/noise-generator-for-bird-research/src/main.ino"
# 13 "D:/PR/okosodu/noise-generator-for-bird-research/src/main.ino"
#include "config.h"
Config conf;
#include "secrets.h"
Secrets sec;

static String name = conf.name;
static String ver = "0_4";

const String update_server = sec.update_server;
const String GScriptId = sec.gID;
const String data_sheet = conf.data_sheet;
const String log_sheet = conf.log_sheet;
const String discord_chanel = sec.discord_chanel;

#define USE_SERIAL Serial


#define mp3RxPin D1
#define mp3TxPin D2
#define mp3PwrPin D3

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
#include <WiFiManager.h>
ESP8266WiFiMulti WiFiMulti;
WiFiClient client;

#include "LittleFS.h"

int volume = 10;




SoftwareSerial mySoftwareSerial(mp3RxPin, mp3TxPin);
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);
boolean startMp3(Stream &softSerial);
void stopMp3();
void forceStopMp3();
void syncClock();
void printDigits(int digits);
void digitalClockDisplay();
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);
void startGSM();
void stopGSM();
String getBatteryVoltage();
void runHourlyReport();
void syncParams();
String getParams();
JsonObject getPlayParams();
void syncTrackLength();
int getTrackLength(int tracknumber);
void updateFunc(String Name, String Version);
void httpUpdateFunc(String update_url);
void update_started();
void update_finished();
void update_progress(int cur, int total);
void update_error(int err);
void GsheetPost(String sheet_name, String datastring);
void discordPost(String message);
String GETTask(String url);
String POSTTask(String url, String payload);
void setup();
void loop();
#line 59 "D:/PR/okosodu/noise-generator-for-bird-research/src/main.ino"
boolean startMp3(Stream &softSerial)
{
  pinMode(mp3PwrPin, OUTPUT);
  digitalWrite(mp3PwrPin, HIGH);

  delay(1000);
  if (!myDFPlayer.begin(softSerial))
  {
    USE_SERIAL.println(F("Unable to begin:"));
    USE_SERIAL.println(F("1.Please recheck the connection!"));
    USE_SERIAL.println(F("2.Please insert the SD card!"));


  }

  return true;
}

void stopMp3()
{
  myDFPlayer.pause();

  digitalWrite(mp3PwrPin, LOW);
}

void forceStopMp3()
{
  digitalWrite(mp3PwrPin, LOW);
}







static const char ntpServerName[] = "us.pool.ntp.org";
const int timeZone = 1;
WiFiUDP Udp;
unsigned int localPort = 8888;

void syncClock()
{
  USE_SERIAL.println(F("Synchronizing inner clock..."));
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(1);
  delay(3000);
  setSyncInterval(3000);
  void digitalClockDisplay();
};

void printDigits(int digits)
{

  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void digitalClockDisplay()
{

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

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

time_t getNtpTime()
{
  IPAddress ntpServerIP;

  while (Udp.parsePacket() > 0)
    ;
  Serial.println(F("Transmit NTP Request"));

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
      Udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long secsSince1900;

      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println(F("No NTP Response :-("));
  return 0;
}


void sendNTPpacket(IPAddress &address)
{

  memset(packetBuffer, 0, NTP_PACKET_SIZE);


  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;

  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;


  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}





void startGSM(){};




void stopGSM(){};

String getBatteryVoltage()
{

  return "666";
}




void runHourlyReport()
{

  String resp = GETTask("https://script.google.com/macros/s/" + GScriptId + "/exec?deviceId=" + ESP.getChipId() + "&batteryVoltage=" + getBatteryVoltage());
};

int paramVersionHere = -2;




void syncParams()
{
  String resp = GETTask("https://script.google.com/macros/s/" + GScriptId + "/exec?deviceId=" + ESP.getChipId() + "&paramVersion=" + String(paramVersionHere));
  if (resp.indexOf(F("playParams")) != -1)
  {

    File file = LittleFS.open(F("/playParams.json"), "w");
    if (!file)
    {
      USE_SERIAL.println(F("file open failed"));
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
      }

      file.close();
    }

    DynamicJsonDocument doc(5000);
    deserializeJson(doc, getParams());
    JsonObject object;
    paramVersionHere = doc["paramVersion"] | -1;
  }
};

String getParams()
{
  File file = LittleFS.open("/playParams.json", "r");
  if (!file)
  {
    USE_SERIAL.println("file open failed");
  }
  else
  {
    String data = file.readString();
    file.close();
    return data;
  }
};




DynamicJsonDocument doc(5000);
JsonObject getPlayParams()
{
  Serial.println(F("Getplayparams"));

  int currentHour = hour();
  deserializeJson(doc, getParams());
  JsonObject object;
  paramVersionHere = doc["paramVersion"] | -1;
  volume = doc["vol"] | 15;
  USE_SERIAL.println("this hour is: " + String(currentHour));

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
  Serial.println(F("syncing tracklengths"));
  String resp = GETTask("https://script.google.com/macros/s/" + GScriptId + "/exec?trackLengths=1");
  if (resp.indexOf(F("tracklengths")) != -1)
  {

    File file = LittleFS.open(F("/trackLengths.json"), "w");
    if (!file)
    {
      USE_SERIAL.println(F("file open failed"));
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
      }

      file.close();
    }
  }
  else
  {
    Serial.println(F("response invalid!"));
  }
}




int getTrackLength(int tracknumber)
{
  JsonObject lengthObject;
  File file = LittleFS.open(F("/trackLengths.json"), "r");
  if (!file)
  {
    USE_SERIAL.println(F("file open failed"));
  }
  else
  {
    String data = file.readString();
    file.close();

    DynamicJsonDocument docPuffer(4000);
    deserializeJson(docPuffer, data);

    for (int i = 0; i < 100; i++)
    {
      lengthObject = docPuffer["tracklengths"][i];


      if (lengthObject["trackNumber"] == tracknumber)
        break;
    }
    USE_SERIAL.println("tracklength obtained: ");
    serializeJsonPretty(lengthObject["length"], USE_SERIAL);
    Serial.println();
    return lengthObject["length"];
  }
}


void printDetail(uint8_t type, int value)
{
  switch (type)
  {
  case TimeOut:
    Serial.println(F("Time Out!"));
    break;
  case WrongStack:
    Serial.println(F("Stack Wrong!"));
    break;
  case DFPlayerCardInserted:
    Serial.println(F("Card Inserted!"));
    break;
  case DFPlayerCardRemoved:
    Serial.println(F("Card Removed!"));
    break;
  case DFPlayerCardOnline:
    Serial.println(F("Card Online!"));
    break;
  case DFPlayerUSBInserted:
    Serial.println("USB Inserted!");
    break;
  case DFPlayerUSBRemoved:
    Serial.println("USB Removed!");
    break;
  case DFPlayerPlayFinished:
    Serial.print(F("Number:"));
    Serial.print(value);
    Serial.println(F(" Play Finished!"));
    break;
  case DFPlayerError:
    Serial.print(F("DFPlayerError:"));
    switch (value)
    {
    case Busy:
      Serial.println(F("Card not found"));
      break;
    case Sleeping:
      Serial.println(F("Sleeping"));
      break;
    case SerialWrongStack:
      Serial.println(F("Get Wrong Stack"));
      break;
    case CheckSumNotMatch:
      Serial.println(F("Check Sum Not Match"));
      break;
    case FileIndexOut:
      Serial.println(F("File Index Out of Bound"));
      break;
    case FileMismatch:
      Serial.println(F("Cannot Find File"));
      break;
    case Advertise:
      Serial.println(F("In Advertise"));
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
}

boolean hourlySetupFlag = false;





void updateFunc(String Name, String Version)
{
  HTTPClient http;

  String url = update_server + "/check?" + "name=" + Name + "&ver=" + Version;
  USE_SERIAL.print("[HTTP] check at " + url);
  if (http.begin(client, url))
  {

    USE_SERIAL.print("[HTTP] GET...\n");

    int httpCode = http.GET();
    delay(10000);
    httpCode = http.GET();

    if (httpCode > 0)
    {

      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);


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

void httpUpdateFunc(String update_url)
{
  if ((WiFiMulti.run() == WL_CONNECTED))
  {







    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);


    ESPhttpUpdate.onStart(update_started);
    ESPhttpUpdate.onEnd(update_finished);
    ESPhttpUpdate.onProgress(update_progress);
    ESPhttpUpdate.onError(update_error);

    t_httpUpdate_return ret = ESPhttpUpdate.update(client, update_url);



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






#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>

void GsheetPost(String sheet_name, String datastring)
{
  USE_SERIAL.println(F("POST to spreadsheet:"));
  String url = String(F("https://script.google.com/macros/s/")) + String(GScriptId) + "/exec";
  String payload = String("{\"command\": \"appendRow\", \  \"sheet_name\": \"") + sheet_name + "\", \ \"values\": " + "\"" + datastring + "\"}";

  USE_SERIAL.println(POSTTask(url, payload));
};

void discordPost(String message)
{

  String payload = "{\"content\": \"" + message + "\"}";
  String url = discord_chanel;
  USE_SERIAL.println(POSTTask(url, payload));
};

String GETTask(String url)
{
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  https.setFollowRedirects(true);
  if (https.begin(*client, url))
  {
    USE_SERIAL.print(F("[HTTPS] GET "));
    USE_SERIAL.println(url);

    int httpCode = https.GET();


    if (httpCode > 0)
    {

      USE_SERIAL.printf("[HTTPS] GET... code: %d\n", httpCode);
      if (httpCode == 302)
      {
        String redirectUrl = https.getLocation();
        https.end();
        return redirectUrl;
      }
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        String payload = https.getString();
        USE_SERIAL.println(payload);
        https.end();

        return payload;
      }
    }
    else
    {
      USE_SERIAL.print(F("[HTTPS] GET... failed, error: "));
      USE_SERIAL.println(httpCode);
      https.end();
      return "";
    }

    https.end();
  }
  else
  {
    USE_SERIAL.println(F("[HTTPS] Unable to connect"));
    return "";
  }
  return "";
};

String POSTTask(String url, String payload)
{
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  if (https.begin(*client, url))
  {
    USE_SERIAL.print(F("[HTTPS] POST "));
    USE_SERIAL.print(url);
    USE_SERIAL.print(" --> ");
    USE_SERIAL.println(payload);
    https.addHeader(F("Content-Type"), F("application/json"));
    https.setFollowRedirects(true);

    int httpCode = https.POST(payload);


    if (httpCode > 0)
    {

      USE_SERIAL.print(F("[HTTPS] POST... code: "));
      USE_SERIAL.println(httpCode);


      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        String payload = https.getString();
        https.end();
        return payload;
      }
    }
    else
    {
      USE_SERIAL.print(F("[HTTPS] POST... failed, error: "));
      USE_SERIAL.println(httpCode);
      https.end();
      return "";
    }

    https.end();
  }
  else
  {
    USE_SERIAL.println(F("[HTTPS] Unable to connect"));
    return "";
  }
  return "";
};
# 679 "D:/PR/okosodu/noise-generator-for-bird-research/src/main.ino"
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

  WiFiManager wifiManager;
  wifiManager.setTimeout(300);
  wifiManager.autoConnect("birdnoise_config_accesspoint");
  delay(3000);
  updateFunc(name, ver);

  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  startMp3(mySoftwareSerial);

  LittleFS.begin();
  if (ESP.getResetReason() == "External System")
  {
    Serial.println(F("Showing of sound skills for humans"));
    myDFPlayer.volume(volume);
    myDFPlayer.play(1);
    delay(3000);
    myDFPlayer.stop();
    syncTrackLength();
    syncTrackLength();
    syncParams();
    syncParams();
  }
  Serial.println(F("DFPlayer Mini online."));
  syncClock();


  GETTask("https://script.google.com/macros/s/" + GScriptId + "/exec?deviceId=" + ESP.getChipId() + "&batteryVoltage=" + getBatteryVoltage() + "&startUp=1");
}

void loop()
{
  Serial.println(F("begining of main loop"));
  delay(100);

  if (minute() < 5)
  {
    if (!hourlySetupFlag)
    {
      Serial.println(F("First minutes of hour, do hourly stuff:"));
      syncClock();
      startGSM();
      runHourlyReport();
      syncParams();
      stopGSM();
      hourlySetupFlag = true;
    }
  }
  else
  {
    hourlySetupFlag = false;
  }



  JsonObject thisHourParams = getPlayParams();

  int tracksize =thisHourParams["tracks"].size();
  Serial.println("number of tracks in this hour: " + String(tracksize));

  if (tracksize<1)
  {
    long sleeptime = (55 - minute()) * 60 * 1000 * 1000;
    if (sleeptime > 0)
    {
      Serial.println("going to deepsleep for:" + String(sleeptime));
      ESP.deepSleep(sleeptime);
    }
  }

  for (int i = 0; i < tracksize; i++)
  {
    Serial.printf("free heap size: %u\n", ESP.getFreeHeap());

    int currentTrack = thisHourParams["tracks"][random(0,tracksize)];
    int minT = thisHourParams["minT"];
    int maxT = thisHourParams["maxT"];
    int tracklength = getTrackLength(currentTrack);
    Serial.println("play track: " + String(currentTrack) + " for secs: " + tracklength);
    myDFPlayer.volume(volume);
    myDFPlayer.play(currentTrack);
    delay(tracklength * 1000 + 5000);

    Serial.print(F("DFplayer state: "));
    Serial.println(myDFPlayer.readState());
    if (myDFPlayer.available())
    {
      printDetail(myDFPlayer.readType(), myDFPlayer.read());
    }


    long calculatedDelay = random(minT, maxT) * 1000;
    Serial.println("pause play for secs: " + String(calculatedDelay / 1000));
    delay(calculatedDelay);

  }
}