/***************************************************
Noise generator for birds 
Written by Krisztián Pál Klucsik

TODOs:
webupdating of the tracklengths file
logging
error handling (message to discord)
GSM module for networking
sound error detecting
 ****************************************************/

#include "config.h" //change to change envrionment
Config conf;
#include "secrets.h"
Secrets sec;

static String name = conf.name;
static String ver = "0_5";

const String update_server = sec.update_server;   //at this is url is the python flask update server, which I wrote
const String GScriptId = sec.gID;                 //This is the secret ID of the Google script app which connects to the Google Spreadsheets
const String log_sheet = conf.log_sheet;          //name of the sheet on the Spreadsheet where the events will be logged
const String discord_chanel = sec.discord_chanel; //a discord channel webhook, we send startup messages there

#define USE_SERIAL Serial

//pin configuration
#define mp3RxPin D1
#define mp3TxPin D2
#define mp3PwrPin D3 //a mosfet circuit attached to switch power to the mp3 player

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

int volume = 10;
/**********************************************
             MP3 player functions

***********************************************/
SoftwareSerial mySoftwareSerial(mp3RxPin, mp3TxPin); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);

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

    // TODO: try to determine cause, and return false
  }

  return true;
}

void stopMp3()
{
  myDFPlayer.pause();
  //TODO check the status, if playing, dont power down
  digitalWrite(mp3PwrPin, LOW);
}

void forceStopMp3()
{
  digitalWrite(mp3PwrPin, LOW);
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
  USE_SERIAL.println(F("Synchronizing inner clock..."));
  GsheetPost(log_sheet, "INFO;Synchronizing inner clock...");
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(1);
  delay(3000);
  setSyncInterval(3000);
  void digitalClockDisplay();
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
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

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
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
/**************end of section********************/

/*
TODO
*/
void startGSM(){};

/*
TODO
*/
void stopGSM(){};

String getBatteryVoltage()
{
  //TODO
  return "666";
}

/*
reports data to the spreadsheet server
*/
void runHourlyReport()
{

  String resp = GETTask("https://script.google.com/macros/s/" + GScriptId + "/exec?deviceId=" + ESP.getChipId() + "&batteryVoltage=" + getBatteryVoltage());
};

int paramVersionHere = -2;

/*
checks if the daily paramteres need an update
*/
void syncParams()
{
    GsheetPost(log_sheet, "INFO;Syncing Params");
  String resp = GETTask("https://script.google.com/macros/s/" + GScriptId + "/exec?deviceId=" + ESP.getChipId() + "&paramVersion=" + String(paramVersionHere));
  if (resp.indexOf(F("playParams")) != -1)
  {
    //save to file
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
    //send an acknowledge back to the sheet, that we are refreshed
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

/*
returns the object for the current hours play parameters (which tracks, pause times between tracks) from the global json object
*/
DynamicJsonDocument doc(5000); //8knál már nem mennek a get requestek
JsonObject getPlayParams()
{
  Serial.println(F("Getplayparams"));

  int currentHour = hour(); //TODO: get time from the GSM module
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
  GsheetPost(log_sheet, "INFO;Syncing tracklengths...");
  Serial.println(F("Syncing tracklengths"));
  String resp = GETTask("https://script.google.com/macros/s/" + GScriptId + "/exec?trackLengths=1");
  if (resp.indexOf(F("tracklengths")) != -1)
  {
    //save to file
    File file = LittleFS.open(F("/trackLengths.json"), "w");
    if (!file)
    {
      USE_SERIAL.println(F("file open failed"));
      GsheetPost(log_sheet, "ERROR;file open failed!");
    }
    else
    {

      int bytesWritten = file.print(resp);

      if (bytesWritten > 0)
      {
        Serial.println(F("File was written "));
        Serial.println(bytesWritten);
        GsheetPost(log_sheet, "INFO;File was written with bytes: " + String(bytesWritten));
      }
      else
      {
        Serial.println(F("File write failed"));
        GsheetPost(log_sheet, "ERROR;File write failed!");
      }

      file.close();
    }
  }
  else
  {
    Serial.println(F("response invalid!"));
    GsheetPost(log_sheet, "ERROR;Response invalid!");
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
    USE_SERIAL.println(F("file open failed"));
  }
  else
  {
    String data = file.readString();
    file.close();
    //Serial.println(data);
    DynamicJsonDocument docPuffer(4000);
    deserializeJson(docPuffer, data);

    for (int i = 0; i < 100; i++)
    {
      lengthObject = docPuffer["tracklengths"][i];
      //USE_SERIAL.println("Processing tracklength record:" );
      //serializeJsonPretty(lengthObject,USE_SERIAL);
      if (lengthObject["trackNumber"] == tracknumber)
        break;
    }
    USE_SERIAL.println("tracklength obtained: ");
    serializeJsonPretty(lengthObject["length"], USE_SERIAL);
    Serial.println();
    return lengthObject["length"];
  }
}

//TODO rewrite this to give back the message dont print it out, i want to put this message into logs
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

/**********************************************
             webupdate functions - only works with WiFi

***********************************************/
void updateFunc(String Name, String Version) //TODO: documentation
{
  HTTPClient http;

  String url = update_server + "/check?" + "name=" + Name + "&ver=" + Version;
  USE_SERIAL.print("[HTTP] check at " + url);
  if (http.begin(client, url))
  { // HTTP

    USE_SERIAL.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();
    delay(10000); //wait for bootup of the server
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

void httpUpdateFunc(String update_url) //this is from the core example
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
    //t_httpUpdate_return ret = ESPhttpUpdate.update(client, "server", 80, "file.bin");

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

void GsheetPost(String sheet_name, String datastring)
{
  datastring = String(ESP.getChipId()) + ";" + datastring; //add ID as first field
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
  client->setInsecure(); //This will set the http connection to insecure! This is not advised, but I have found no good way to use real SSL, and my application doesn't need the added security
  HTTPClient https;
  https.setFollowRedirects(true); //this is needed for the Google backend, which always redirects
  if (https.begin(*client, url))
  {
    USE_SERIAL.print(F("[HTTPS] GET "));
    USE_SERIAL.println(url);

    int httpCode = https.GET();

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
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

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      USE_SERIAL.print(F("[HTTPS] POST... code: "));
      USE_SERIAL.println(httpCode);

      // file found at server
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

  WiFiManager wifiManager;
  wifiManager.setTimeout(300);
  wifiManager.autoConnect("birdnoise_config_accesspoint");
  delay(3000);           //give somte time for the Wifi connection
  updateFunc(name, ver); //checking update

  GsheetPost(log_sheet, "INFO;Startup, setup...");

  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  startMp3(mySoftwareSerial);
  //TODO:handle error if returns false
  LittleFS.begin();
  if (ESP.getResetReason() == "External System")
  { //kézi újraindításnál
    GsheetPost(log_sheet, "INFO;Manual reset");
    syncTrackLength();
    delay(1000);
    syncParams();
    Serial.println(F("Showing of sound skills for humans"));
    myDFPlayer.volume(volume);
    myDFPlayer.play(1);
    delay(3000);
    myDFPlayer.stop();

    syncTrackLength(); //tuti, ami biztos
    delay(1000);
    syncParams(); //to update the version in the excel in the first run
  }
  Serial.println(F("DFPlayer Mini online."));
  syncClock();

  GETTask("https://script.google.com/macros/s/" + GScriptId + "/exec?deviceId=" + ESP.getChipId() + "&batteryVoltage=" + getBatteryVoltage() + "&startUp=1");
  GsheetPost(log_sheet, "INFO;Setup finished");
}

void loop()
{
  Serial.println(F("begining of main loop"));
  delay(100);

  if (minute() < 5)
  {
    if (!hourlySetupFlag)
    { //just once do the hourly stuff
      Serial.println(F("First minutes of hour, do hourly stuff:"));
      GsheetPost(log_sheet, "INFO;First minutes of hour, do hourly stuff");
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

  //main business logic

  JsonObject thisHourParams = getPlayParams();

  int tracksize = thisHourParams["tracks"].size();
  Serial.println("number of tracks in this hour: " + String(tracksize));
  //TODO ha nincs beállítás go to sleep
  if (tracksize < 1)
  {
    long sleeptime = (55 - minute()) * 60 * 1000 * 1000;
    if (sleeptime > 0)
    {
      Serial.println("going to deepsleep for:" + String(sleeptime));
      GsheetPost(log_sheet, "INFO;No tracks to paly now, going to deepsleep for:" + String(sleeptime));
      ESP.deepSleep(sleeptime);
    }
  }

  for (int i = 0; i < tracksize; i++)
  {
    Serial.printf("free heap size: %u\n", ESP.getFreeHeap());
    //startMp3(mySoftwareSerial); //Ettől recseg
    int currentTrack = thisHourParams["tracks"][random(0, tracksize)];
    int minT = thisHourParams["minT"];
    int maxT = thisHourParams["maxT"];
    int tracklength = getTrackLength(currentTrack);
    Serial.println("play track: " + String(currentTrack) + " for secs: " + tracklength);
    GsheetPost(log_sheet, "INFO;Play track: " + String(currentTrack) + " for secs: " + tracklength);
    myDFPlayer.volume(volume);
    myDFPlayer.play(currentTrack);
    delay(tracklength * 1000 + 5000); //wait until the end of track and 5 more secs to be sure
    //TODO a tracklength 10edénél rámérni hogy megy-e
    Serial.print(F("DFplayer state: "));
    Serial.println(myDFPlayer.readState()); //determine stop
    if (myDFPlayer.available())
    {
      printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
    }
    //stopMp3();
    //calculate pause between tracks:
    long calculatedDelay = random(minT, maxT) * 1000;
    Serial.println("pause play for secs: " + String(calculatedDelay / 1000));
    GsheetPost(log_sheet, "INFO;Pause play for secs: " + String(calculatedDelay / 1000));
    delay(calculatedDelay);
  }
}