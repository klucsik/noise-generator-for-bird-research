/***************************************************
Noise generator for birds 
Written by Krisztián Pál Klucsik

TODOs:
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

//pin configuration
#define mp3RxPin D1
#define mp3TxPin D2
#define mp3PwrPin D3 //a mosfet circuit attached to switch power to the mp3 player

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include "ArduinoJson.h"
#include "TimeLib.h"



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
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while (true)
    {
      delay(0); // TODO: try to determine cause, and return false
    }
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
const char* mockjson = "{\"7\":{\"tracks\":[1,2],\"minT\":30,\"maxT\":50},\"8\":{\"tracks\":[2],\"minT\":35,\"maxT\":35},\"9\":{\"tracks\":[1],\"minT\":40,\"maxT\":50}}";
DynamicJsonDocument doc(384);

/*
returns the object for the current hours play parameters (which tracks, pause times between tracks) from the global json object
*/
JsonObject getPlayParams()
{
  int currentHour = 7; //TODO: get time from the GSM module
  deserializeJson(doc, mockjson);
  JsonObject object = doc["7"]; //TODO make this dynamic
  serializeJsonPretty(object,Serial);
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

  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  startMp3(mySoftwareSerial);
  //TODO:handle error if returns false

  //TODO: if resetcause is normal reset/powering on, give a wellcome sound to the operator
  myDFPlayer.play(1);

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