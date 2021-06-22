#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <stdio.h>
#include <FS.h>
// #include <LittleFS.h>
// #define SPIFFS LittleFS
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <wifipw.h>
#include <time.h>
#include <ArduinoJson.h>
#include "RTClib.h"
#define RELAY_PIN 16

ESP8266WebServer server(80);



int ledson = LOW;
bool curing = false;
int curing_end_time = 0;
String whatisthedoordoing="nothing";

// Sun position memes
double lat = 0;
double lon = 0;
double timezone = 2;
double gamma(tm t)
{
  return ((2*PI)/365)*(t.tm_yday-1+((t.tm_hour-12)/24));
}

double eqtime(double gamma)
{
  return 229.18*(0.000075 + 0.001868*cos(gamma) - 0.032077*sin(gamma) - 0.014615*cos(2*gamma) - 0.040849*sin(2*gamma) );
}

double decl(double gamma)
{
  return 0.006918 - 0.399912*cos(gamma) + 0.070257*sin(gamma) - 0.006758*cos(2*gamma) + 0.000907*sin(2*gamma)- 0.002697*cos(3*gamma) + 0.00148*sin(3*gamma);
}

double time_offset(tm t)
{
  return eqtime(gamma(t))+ 4*lon - 60*timezone;
}

double truesolartime(tm t)
{
  return t.tm_hour*60+t.tm_min+(t.tm_sec/60)+time_offset(t);
}

double solarhourangle(tm t)
{
  //return (truesolartime(t)/4)-180;
  auto dec = decl(gamma(t));
  return acos((cos(90.833)/(cos(lat)*cos(dec)))-(tan(lat)*tan(dec)));
}

// double solarzenithangle(tm t)
// {
//   auto dec = decl(gamma(t));
//   return acos((sin(lat)*sin(dec))+(cos(lat)*cos(dec)*cos(solarhourangle(t))));
// }

time_t sunrise(tm t, tm *now)
{
  auto sunriseminutes = (720-(4*(lon+solarhourangle(t)))-eqtime(gamma(t)))+timezone;
  
  now->tm_hour=sunriseminutes/60;
  now->tm_min=sunriseminutes-(now->tm_hour*60);
  now->tm_sec=(sunriseminutes-int(sunriseminutes))*60;

  time_t sr = mktime(now);

  free(now);
  return sr;
}
double sunset(tm t, tm *now)
{
  auto sunsetminutes = (720-(4*lon)-eqtime(gamma(t)))+timezone;
  
  now->tm_hour=sunsetminutes/60;
  now->tm_min=sunsetminutes-(now->tm_hour*60);
  now->tm_sec=(sunsetminutes-int(sunsetminutes))*60;

  time_t sr = mktime(now);

  free(now);
  return sr;
}




// TIME STUFF
RTC_Micros rtc;
time_t localtm = 0;
float leftover;
tm openTimeOrigin;
tm closeTimeOrigin;
time_t todayCloseTime = 20*60*60;
time_t todayOpenTime = 5*60*60;
time_t openOffset = 0;
time_t closeOffset = 0;
unsigned long lastTimeUpdate=0;
unsigned long lastWrite=0;


void setlocaltm(tm time)
{
  
  localtm = mktime(&time);
  rtc.adjust(DateTime(localtm));
}

void updateDoorTimes()
{
  auto local = gmtime(&localtm);
  todayCloseTime = sunset(*local, gmtime(&localtm))+(closeOffset*60);
  todayOpenTime = sunrise(*local, gmtime(&localtm))+(openOffset*60);
}

time_t getCurrentTimeInSeconds()
{
  auto now = rtc.now();
  time_t seconds = now.second();
  seconds+=now.minute()*60;
  seconds+=now.hour()*60*60;
  return seconds;
}



// ENDSTOP
int ENDSTOPLOWPIN = 3;
int ENDSTOPHIGHPIN = 5;
int highendstopstatus = 0;
int lowendstopstatus = 0;

void updateEndstopStatus()
{
  highendstopstatus = !digitalRead(ENDSTOPHIGHPIN);
  lowendstopstatus = !digitalRead(ENDSTOPLOWPIN);
}


// MOTOR
int motoren = LOW;
int moveright= HIGH;
int dir = moveright;
int moveon = LOW;
int DIRPIN=12;
int STEPPIN=14;
int MOTORENPIN=16;
int targetrpm = 100;
int stepspersecond = 100;
int stepsperrev = 100;
unsigned long motorLastStep = 0;
double targetstepinterval = 0;
int stepinterval = 4000;
void updateMotorSteps()
{
  stepspersecond = targetrpm*stepsperrev;
  targetstepinterval = 1000/stepspersecond;
}

void doStep()
{
  //if(!motoren)
  //{
    digitalWrite(STEPPIN, LOW);
    delayMicroseconds(stepinterval);
    digitalWrite(STEPPIN, HIGH);
    delayMicroseconds(stepinterval);
  //}
  motorLastStep = millis();
}

void handleMotorMove()
{
  // updateMotorSteps();
  // unsigned long difference = millis()-motorLastStep;
  // float numberSteps = difference/targetstepinterval;
  // numberSteps = min(stepspersecond, int(numberSteps));
  // for(int x =0; x<int(numberSteps);++x)
  // {
  //   doStep();
  // }
  
        digitalWrite(MOTORENPIN, motoren);
    auto now = millis();
    while(millis()-now<((stepinterval/1000)*2.5))
    {
      //handleMotorMove();
      doStep();
      //delay(1);
    }
    //digitalWrite(STEPPIN, HIGH);
}

bool setMotorDirection(bool left=true)
{
  digitalWrite(DIRPIN, moveright&left);
  return moveright&left;
}

bool openDoor()
{
  setMotorDirection(true);
  auto now = millis();
  while(lowendstopstatus && millis()-now<500)
  {
    handleMotorMove();
    updateEndstopStatus();
  }
  handleMotorMove();
  moveon=true;
  whatisthedoordoing="Trying to open the door!";
}

bool closeDoor()
{
  setMotorDirection(false);
  auto now = millis();
  while(highendstopstatus && millis()-now<500)
  {
    handleMotorMove();
    updateEndstopStatus();
  }
  handleMotorMove();
  moveon=true;
  whatisthedoordoing="Trying to close the door!";

}


String serializeState()
{
  String state="";
  state+=String(rtc.now().unixtime())+"\n";
  state+=String(todayOpenTime)+"\n";
  state+=String(todayCloseTime)+"\n";
  state+=String(openOffset)+"\n";
  state+=String(closeOffset)+"\n";
  state+=String(stepinterval);
  return state;
}


// void setMotorRPM(int rpm)
// {
//   targetrpm=rpm;
//   updateMotorSteps();
// }

String readFile(String path) { // send the right file to the client (if it exists)
  //if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = "text/html";            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    auto text = file.readString();
    //Serial.println("File:");
    //Serial.println(text);
    file.close();
    //size_t sent = server.streamFile(file, contentType); // And send it to the client
    //file.close();                                       // Then close the file again
    return text;
  }
  //Serial.println("File didn't exist");
  return "";                                         // If the file doesn't exist, return false
}
String getContentType(String filename){
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){  // send the right file to the client (if it exists)
  //startMessageLine();
  // server.client().setNoDelay(1);
//Serial.print("requesting file: ");
//Serial.println(path);
  if(path.endsWith("/")) path += "index.html";           // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  if(SPIFFS.exists(path)){  // If the file exists, either as a compressed archive, or normal                                       // Use the compressed version
	File file = SPIFFS.open(path, "r");
    auto s = file.size();
    server.send_P(200, contentType.c_str(), file.readString().c_str(), s);
    //size_t sent = server.streamFile(file, contentType);    // Send it to the client
    ////Serial.print(string_format("wrote %d bytes", sent));
    file.close();                                          // Close the file again
    return true;
  }
  return false;                                          // If the file doesn't exist, return false
}
void OTASetup();
bool writeFile(String text, String path, bool append=false)
{
  int bufsize= text.length()*sizeof(char);
  FSInfo info;
  SPIFFS.info(info);

  unsigned int freebytes = info.totalBytes-info.usedBytes;
  //Serial.println("Writing file: ");
  //Serial.println(path);
  //Serial.println(text);
  if(SPIFFS.exists(path))
  {
    auto file = SPIFFS.open(path, "r");
    if(!append)
    {if(freebytes+file.size()>bufsize)
    {
      file.close();
      SPIFFS.remove(path);
      file = SPIFFS.open(path, "w");
      file.write(text.c_str(), bufsize);
      //Serial.println("Overwrite file");
    }}
    else{
      if(freebytes>bufsize)
      {
        file.close();
      file = SPIFFS.open(path, "w");
      file.seek(file.size());
      file.write(text.c_str(), bufsize);
      //Serial.println("Append File");
      }
    }
  }
  else if(freebytes>bufsize)
  {
    auto file = SPIFFS.open(path, "w");

      file.write(text.c_str(), bufsize);
      //Serial.println("new file");
  }
  

  return true;
}

// void updatelocaltm()
// {
//   unsigned long now = millis();
//   if(now-lastWrite>1000*60)
//   {
    
//   writeFile(serializeState(), "state.cfg");
//   lastWrite=millis();
//   }
//   if(now<lastTimeUpdate)
//   {
//     leftover+= ULONG_MAX-lastTimeUpdate;
//     lastTimeUpdate=0;
//   }
//   unsigned long delta = now-lastTimeUpdate;
//   if(delta+leftover>1000)
//   {
//     double seconds= (delta+leftover)/1000;
//     double secondsFull;
//     double leftover = modf(seconds, &secondsFull);
//     localtm+= secondsFull;
//     lastTimeUpdate = millis();
//   }
  
// }

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
boolean ConnectWifi(bool tar);


void handleConfigSet()
{
	int counter = 0;
  
      if(server.hasArg("demosteps"))
  {
    motoren=LOW;//!motoren;
    digitalWrite(MOTORENPIN, motoren);
    //doStep();
    moveon=!moveon;

    
    // digitalWrite(DIRPIN, dir);
    // dir = !dir;
    //dir = setMotorDirection(!dir);
  }
    if(server.hasArg("manualclose"))
  {
    closeDoor();
  }
      if(server.hasArg("manualopen"))
  {
    openDoor();
  }
  //Serial.print("PUT arguments: ");
  //Serial.println(server.args());
  //Serial.println(server.argName(server.args()-1));
  //Serial.println(server.arg(server.args()-1));
  if(server.hasArg("setTime"))
  {
    String seconds =  server.arg("setTime");
    time_t newtime = (atol(seconds.c_str()));
        //Serial.print("setTime: ");
    //Serial.println(server.arg("setTime"));
    //Serial.println(newtime);
    // localtm = newtime;
    auto temptime = TimeSpan(newtime);
    auto n = rtc.now();
    
    rtc.adjust(DateTime(n.year(), n.month(), n.day(), temptime.hours(), temptime.minutes(), temptime.seconds()));
  }
  if(server.hasArg("setOpenTime"))
  {
    String seconds =  server.arg("setOpenTime");
    time_t newopentime = (atol(seconds.c_str()));
    todayOpenTime = newopentime;
  }
  if(server.hasArg("setCloseTime"))
  {
    String seconds =  server.arg("setCloseTime");
    time_t newclosetime = (atol(seconds.c_str()));
    todayCloseTime = newclosetime;
  }
  
  if(server.hasArg("setOpenOffset"))
  {
    String seconds =  server.arg("setOpenOffset");
    time_t newopentime = (atol(seconds.c_str()));
    openOffset = newopentime;
  }
  if(server.hasArg("setCloseOffset"))
  {
    String seconds =  server.arg("setCloseOffset");
    time_t newclosetime = (atol(seconds.c_str()));
    closeOffset = newclosetime;
  }
  if(server.hasArg("setMotorSpeed"))
  {
    String val = server.arg("setMotorSpeed");
    int newval = val.toInt();//atoi(val.c_str());
    stepinterval = max(min(newval, 10000), 1000);
  }
  //Serial.println("Trying to write state");
  writeFile(serializeState(), "state.cfg");
  //Serial.println("Serialized state: "+serializeState());
  readFile("state.cfg");
  server.send(200);
}


void loadConfig()
{
  String state = readFile("state.cfg");
  //Serial.println("readFile worked");
  //Serial.println(state.length());
  if(state.length()>0)
  {
    int cnt = 0;
    int lastidx = 0;
    lastidx = state.indexOf('\n', lastidx);
    while(lastidx>=0)
    {
      cnt++;
      lastidx = state.indexOf('\n', lastidx+1);
    }
    //Serial.print("Counted newlines: ");
    //Serial.println(cnt);
    if(cnt==5)
    {
      
  int timeend = state.indexOf('\n');
  int opentimeend = state.indexOf('\n', timeend+1);
  int closetimeend = state.indexOf('\n', opentimeend+1);
  int openoffsetend = state.indexOf('\n', closetimeend+1);
  int closeoffsetend= state.indexOf('\n', openoffsetend+1);

    time_t seconds_since_epoch = (state.substring(0, timeend)).toInt();
    rtc.adjust(DateTime(seconds_since_epoch));
    //Serial.print("Time: ");
    //Serial.println(localtm);
    todayOpenTime = (state.substring(timeend, opentimeend)).toInt();
    todayCloseTime = (state.substring(opentimeend, closetimeend)).toInt();
    openOffset = (state.substring(closetimeend, openoffsetend)).toInt();
    closeOffset = (state.substring(openoffsetend, closeoffsetend)).toInt();
    stepinterval = (state.substring(closeoffsetend)).toInt();
    }
  }
  
  //int motorspdend = state.indexOf('\n', closeoffsetend+1);
}

String getEvents()
{
  return "";
}

void handleStatusGet()
{
  //Serial.println("Tryng to get status...");
  StaticJsonDocument<0x5FF> jsonBuffer; //TODO SOMETHING + STATUS BUFFER SIZE
  char JSONmessageBuffer[0x1FF];
  //Serial.println("Buffer created");
  jsonBuffer["curtime"] = getCurrentTimeInSeconds();
  //Serial.println("About to 1");
  jsonBuffer["endstophigh"] = highendstopstatus;
  //Serial.println("About to 2");
  jsonBuffer["endstoplow"] = lowendstopstatus;
  jsonBuffer["doorpos"] = whatisthedoordoing;
  jsonBuffer["sunpos"] = "Coming SoonTM";
  jsonBuffer["opentime"] = todayOpenTime;
  jsonBuffer["closetime"] = todayCloseTime;
  jsonBuffer["openoffset"] = openOffset;
  //Serial.println("About to 3");
  jsonBuffer["closeoffset"] = closeOffset;
  jsonBuffer["motorspeed"] = stepinterval;
  
  jsonBuffer["doorlog"] = getEvents(); //TODO move time thing to read buffers part on condition of encountering a \n :)
  //Serial.println("About to serialize");
  serializeJsonPretty(jsonBuffer,JSONmessageBuffer);
  //Serial.println("Json serialized");
  server.send(200, "application/json", JSONmessageBuffer);
  //Serial.println("Sent status");
}

boolean ConnectWifi()
{
  boolean state = true;
  int i = 0;
  String ss = ssid;
  String pw = password;
  WiFi.mode(WIFI_STA);
  // //Serial.println("trying....");
  WiFi.begin(ss, pw);
  // //Serial.println("");
  // //Serial.println("Connecting to WiFi");

  // Wait for connection
  // //Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
    if (i > 20){
      state = false;
      break;
    }
    i++;
  }
  if (state){
    // //Serial.println("");
    // //Serial.print("Connected to ");
    // //Serial.println(ssid);
    // //Serial.print("IP address: ");
    // //Serial.println(WiFi.localIP());
  } else {
    // //Serial.println("");
    // //Serial.println("Connection failed.");
  }

  return state;
}


void setup() {
  rtc.begin(DateTime());
  WiFi.hostname(hostname);
  //pinMode(DIRPIN, FUNCTION_3); 
  pinMode(DIRPIN, OUTPUT);
  digitalWrite(DIRPIN, moveright);
  pinMode(MOTORENPIN, OUTPUT);
  digitalWrite(MOTORENPIN, motoren);
  pinMode(STEPPIN, OUTPUT);
  digitalWrite(STEPPIN, HIGH);
  pinMode(1, FUNCTION_3); 
  pinMode(ENDSTOPHIGHPIN, INPUT_PULLUP );
  pinMode(ENDSTOPLOWPIN, INPUT_PULLUP );
  // put your setup code here, to run once:
	//Serial.begin(9600);
  //Serial.println("test!");
	// ConnectWifi();
  
  WiFi.setOutputPower(20.5);
  WiFi.softAP(ssid, password);

// 	//WiFi.forceSleepBegin();
	auto res = SPIFFS.begin();
  server.on("/set/config", HTTP_PUT, handleConfigSet);
    server.on("/set/config", HTTP_POST, handleConfigSet);
  server.on("/get/status", HTTP_GET, handleStatusGet);
//   // this will be called for each packet received
  server.onNotFound([]() {                             // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      handleNotFound(); // otherwise, respond with a 404 (Not Found) error
  });
  if (MDNS.begin(hostname)) {
    // //Serial.println("MDNS responder started"); 
  }
  OTASetup();
  
  auto now = millis();
  while(millis()-now<5000)
  {
    ArduinoOTA.handle();
    delay(10);
  }

  //Serial.println("trying to load config");
  time_t bootdur = (rtc.now()-DateTime()).totalseconds();
  loadConfig();
  rtc.adjust(DateTime(rtc.now().unixtime()+bootdur));
  //Serial.println("Done loading config");
  server.begin();

}


static int countStartupSafeguard = 0;
static int last_res = 0;
int startupguard()
{
  if (countStartupSafeguard<200)
  {
    
    if(millis() - last_res > 40)
    {
      last_res = millis();
      countStartupSafeguard++;
    }
    return 0;
  }
  return 1;
}

void determineAction()
{
  time_t tnow = getCurrentTimeInSeconds();
  if(tnow>todayOpenTime&&!highendstopstatus&&tnow<todayCloseTime)
  {
    openDoor();
  }
  if(tnow>todayCloseTime&&!lowendstopstatus)
  {
    closeDoor();
    // if(!highendstopstatus && !lowendstopstatus)
    // {
    //   openDoor();
    //   while(!highendstopstatus)
    //   {
    //     handleMotorMove();
    //     updateEndstopStatus();
    //   }
    // }
    // if(highendstopstatus)
    // {
    //   closeDoor();
    // }

  }
  
}
unsigned long lastDetAction = 0;
void loop() {
  ArduinoOTA.handle();
  // if (startupguard()==0)
  //   return;
  //server.handleClient();
  MDNS.update();
  // updatelocaltm();
  // static int last_result_time = 0;
  // if(curing && millis() > curing_end_time)
  // {
  //   ledson=LOW;
  //   digitalWrite(LEDPIN, ledson);
  //   motoren=HIGH;
  //   digitalWrite(MOTORENPIN, motoren);
  //   curing = false;
  // }
  //delay(10);
  updateEndstopStatus();
  auto now = millis();
  if(now-lastDetAction>1000)
  {
  determineAction();
  lastDetAction=millis();
  }
  if (moveon && !lowendstopstatus && !highendstopstatus)
  {
    handleMotorMove();
    bool lowtemp = lowendstopstatus;
    bool hightemp = highendstopstatus;
    updateEndstopStatus();
    if (lowtemp!=lowendstopstatus || hightemp!=highendstopstatus)
    {
      moveon=false;
      whatisthedoordoing="Door arrived at endstop!";
      if(lowendstopstatus){
      // {
      //   motoren=LOW;
        digitalWrite(MOTORENPIN, !motoren);
      }
    }
  }

  server.handleClient();

}


void OTASetup()
{
  ArduinoOTA.setHostname("root");
  ArduinoOTA.setPassword(otapw);
  ArduinoOTA.setPort(8266);
  ArduinoOTA.onStart([]() {
    // //Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    // //Serial.println("\nEnd");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    // //Serial.printf("Error[%u]: ", error);
    // if (error == OTA_AUTH_ERROR) //Serial.println("Auth Failed");
    // else if (error == OTA_BEGIN_ERROR) //Serial.println("Begin Failed");
    // else if (error == OTA_CONNECT_ERROR) //Serial.println("Connect Failed");
    // else if (error == OTA_RECEIVE_ERROR) //Serial.println("Receive Failed");
    // else if (error == OTA_END_ERROR) //Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  // //Serial.println("OTA ready"); 

}