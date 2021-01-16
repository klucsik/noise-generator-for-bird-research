/***************************************************
Noise generator for birds 
Written by Krisztián Pál Klucsik

TODOs:
time get
logging
error handling (message to discord)
parameterizing the playing
sound error detecting
 ****************************************************/

#include "config.h" //change to change envrionment
Config conf;
#include "secrets.h"
Secrets sec;

static String name = conf.name;
static String ver = "0_1";

const String update_server = sec.update_server; //at this is url is the python flask update server, which I wrote
const String GScriptId = sec.gID; //This is the secret ID of the Google script app which connects to the Google Spreadsheets
const String data_sheet = conf.data_sheet; //name of the sheet on the Spreadsheet where the data will be logged
const String log_sheet = conf.log_sheet; //name of the sheet on the Spreadsheet where the events will be logged
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
  myDFPlayer.volume(15); //Set volume value. From 0 to 30
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






/*
TODO
sychronizes the time
*/
void syncClock(){};

/*
TODO
reports data to the spreadsheet server
*/
void runHourlyReport(){};

/*
TODO
checks if the daily paramteres need an update
*/
void checkParams(){};

/*
TODO
*/
void startGSM(){};

/*
TODO
*/
void stopGSM(){};








/* TODO: delete this. This is an example from arduinojson

{
"7":{
   "tracks":[1,2],
  "minT":30,
  "maxT":50
  },
"8":{
   "tracks":[2],
  "minT":35,
  "maxT":35
  },
"9":{
   "tracks":[1],
  "minT":40,
  "maxT":50
  }
}

-------------
const char* json = "{\"7\":{\"tracks\":[1,2],\"minT\":30,\"maxT\":50},\"8\":{\"tracks\":[2],\"minT\":35,\"maxT\":35},\"9\":{\"tracks\":[1],\"minT\":40,\"maxT\":50}}";

DynamicJsonDocument doc(384);
deserializeJson(doc, json);

JsonObject root_7 = doc["7"];

int root_7_tracks_0 = root_7["tracks"][0]; // 1
int root_7_tracks_1 = root_7["tracks"][1]; // 2

int root_7_minT = root_7["minT"]; // 30
int root_7_maxT = root_7["maxT"]; // 50

JsonObject root_8 = doc["8"];

int root_8_tracks_0 = root_8["tracks"][0]; // 2

int root_8_minT = root_8["minT"]; // 35
int root_8_maxT = root_8["maxT"]; // 35

JsonObject root_9 = doc["9"];

int root_9_tracks_0 = root_9["tracks"][0]; // 1

int root_9_minT = root_9["minT"]; // 40
int root_9_maxT = root_9["maxT"]; // 50
*/

/*
//get config data from network or flashdisk or mock
char getParamJson()
{
  char mockjson = "{\"7\":{\"tracks\":[1,2],\"minT\":30,\"maxT\":50},\"8\":{\"tracks\":[2],\"minT\":35,\"maxT\":35},\"9\":{\"tracks\":[1],\"minT\":40,\"maxT\":50}}";
  return mockjson;
}*/
const char* mockjson = "{\"7\":{\"tracks\":[1,2],\"minT\":30,\"maxT\":50},\"9\":{\"tracks\":[2],\"minT\":35,\"maxT\":35},\"9\":{\"tracks\":[1],\"minT\":40,\"maxT\":50}}";
DynamicJsonDocument doc(384);

/*
returns the object for the current hours play parameters (which tracks, pause times between tracks) from the global json object
*/
JsonObject getPlayParams()
{
  int currentHour = hour(); //TODO: get time from the GSM module
  deserializeJson(doc, mockjson);
  JsonObject object;
  USE_SERIAL.println("this hour is: " + String(currentHour));
  switch (currentHour)
  { //pls don't judge, i didn't found any other way
  case 1:
       object = doc["1"]; 
    break;
  case 2:
       object = doc["2"]; 
    break;
case 3:
       object = doc["3"]; 
    break;
case 4:
       object = doc["4"]; 
    break;
    case 5:
       object = doc["5"]; 
    break;
    case 6:
       object = doc["6"]; 
    break;
    case 7:
       object = doc["7"]; 
    break;
    case 8:
       object = doc["8"]; 
    break;
    case 9:
       object = doc["9"]; 
    break;
    case 10:
       object = doc["10"]; 
    break;
    case 11:
       object = doc["11"]; 
    break;
    case 12:
       object = doc["12"]; 
    break;
    case 13:
       object = doc["13"]; 
    break;
    case 14:
       object = doc["14"]; 
    break;
    case 15:
       object = doc["15"]; 
    break;
    case 16:
       object = doc["16"]; 
    break;
    case 17:
       object = doc["17"]; 
    break;
    case 18:
       object = doc["18"]; 
    break;
    case 19:
       object = doc["19"]; 
    break;
    case 20:
       object = doc["20"]; 
    break;
    case 21:
       object = doc["21"]; 
    break;
    case 22:
       object = doc["22"]; 
    break;
    case 23:
       object = doc["23"]; 
    break;
    case 24:
       object = doc["24"]; 
    break;
        
  default:
    break;
  }

  USE_SERIAL.println("hourly config:");
  serializeJsonPretty(object,USE_SERIAL);
  return object;
}

/*
returns the length in seconds of the given tracknumber. If it doesn't know, return -1
*/
int getTrackLength(int tracknumber)
{
  return 11;
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

String POSTTask(String url,  String payload)
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
  Serial.print("name: ");
  Serial.println(name);
  Serial.print(" ver: ");
  Serial.print(ver);
  Serial.print(" deviceID: ");
  Serial.println(ESP.getChipId());

  WiFiManager wifiManager;
  wifiManager.setTimeout(300);
  wifiManager.autoConnect("birdnoise_config_accesspoint");

  //UNCOMMENTME updateFunc(name, ver); //checking update


  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  startMp3(mySoftwareSerial);
  //TODO:handle error if returns false

  //TODO: if resetcause is normal reset/powering on, give a wellcome sound to the operator
  myDFPlayer.play(1);
  delay(1000);
  Serial.println(F("DFPlayer Mini online."));
  syncClock();
}

void loop()
{
  Serial.println(F("begining of main loop"));

  if (minute() < 5 )
  {
    if(!hourlySetupFlag){ //just once do the hourly stuff
    Serial.println(F("First minutes of hour, do hourly stuff:"));
    syncClock();
    startGSM();
    runHourlyReport();
    checkParams();
    stopGSM();
    hourlySetupFlag=true;
    }
  }
  else{
    hourlySetupFlag=false;
  }

  //main business logic

  JsonObject thisHourParams = getPlayParams();
  if (thisHourParams.isNull())
  {
    long sleeptime = 55 - minute() * 60 * 1000 * 1000;
    if (sleeptime > 0)
    {
      Serial.println("going to deepsleep for:" + String(sleeptime));
      ESP.deepSleep(sleeptime);
    }
  }

  for (int i = 0; i < thisHourParams["tracks"].size(); i++)
  {
    startMp3(mySoftwareSerial);
    int currentTrack = thisHourParams["tracks"][i];
    int minT = thisHourParams["minT"];
    int maxT = thisHourParams["maxT"];
    Serial.println("play track:" + String(currentTrack));
    myDFPlayer.play(currentTrack);
    delay(getTrackLength(currentTrack) * 1000 + 5000); //wait until the end of track and 5 more secs to be sure
    //TODO a tracklength 10edénél rámérni hogy megy-e
    Serial.print(F("DFplayer state: "));
    Serial.println(myDFPlayer.readState());                           //determine stop
      if (myDFPlayer.available())
        {
          printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
        }
    stopMp3();
    //calculate pause between tracks:
    long calculatedDelay = random(minT, maxT) * 1000;
    Serial.println("pause play for secs: " + String(calculatedDelay/1000));
    delay(calculatedDelay);
  }


}