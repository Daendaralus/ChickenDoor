#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <stdio.h>
#include <FS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <wifipw.h>
#include <time.h>
#define RELAY_PIN 16

ESP8266WebServer server(80);

int LEDPIN=5;


int ledson = LOW;
bool curing = false;
int curing_end_time = 0;


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
time_t localtm;
float leftover;
tm openTimeOrigin;
tm closeTimeOrigin;
time_t todayCloseTime;
time_t todayOpenTime;
int openOffset = 0;
int closeOffset = 0;
unsigned long lastTimeUpdate;

void updatelocaltm()
{
  unsigned long now = millis();
  if(now<lastTimeUpdate)
  {
    leftover+= ULONG_MAX-lastTimeUpdate;
    lastTimeUpdate=0;
  }
  unsigned long delta = now-lastTimeUpdate;
  if(delta+leftover>1000)
  {
    double seconds= (delta+leftover)/1000;
    double secondsFull;
    double leftover = modf(seconds, &secondsFull);
    localtm+= secondsFull;
  }
  lastTimeUpdate = millis();
}

void setlocaltm(tm time)
{
  localtm = mktime(&time);
}

void updateDoorTimes()
{
  auto local = gmtime(&localtm);
  todayCloseTime = sunset(*local, gmtime(&localtm))+(closeOffset*60);
  todayOpenTime = sunrise(*local, gmtime(&localtm))+(openOffset*60);
}




// ENDSTOP
int ENDSTOPLOWPIN = -1;
int ENDSTOPHIGHPIN = -1;
int highendstopstatus = 0;
int lowendstopstatus = 0;

void updateEndstopStatus()
{
  highendstopstatus = digitalRead(ENDSTOPHIGHPIN);
  lowendstopstatus = digitalRead(ENDSTOPLOWPIN);
}


// MOTOR
int motoren = HIGH;
int moveright= HIGH;
int moveon = LOW;
int DIRPIN=12;
int STEPPIN=14;
int MOTORENPIN=16;
int targetrpm = 100;
int stepspersecond = 100;
int stepsperrev = 100;
unsigned long motorLastStep = 0;
int targetstepinterval = 0;

void updateMotorSteps()
{
  stepspersecond = targetrpm*stepsperrev;
  targetstepinterval = 1000/stepspersecond;
}

void doStep()
{
  if(!motoren)
  {
    digitalWrite(STEPPIN, HIGH);
    delay(1);
    digitalWrite(STEPPIN, LOW);
  }
  motorLastStep = millis();
}

void handleMotorMove()
{
  unsigned long difference = millis()-motorLastStep;
  float numberSteps = difference/targetstepinterval;
  updateMotorSteps();
  for(int x =0; x<int(floor(numberSteps));++x)
  {
    doStep();
  }
}

void setMotorDirection(bool left=true)
{
  digitalWrite(DIRPIN, moveright&left);
}

void setMotorRPM(int rpm)
{
  targetrpm=rpm;
  updateMotorSteps();
}

String readFile(String path) { // send the right file to the client (if it exists)
  //if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = "text/html";            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    auto text = file.readString();
    file.close();
    //size_t sent = server.streamFile(file, contentType); // And send it to the client
    //file.close();                                       // Then close the file again
    return text;
  }
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

  if(path.endsWith("/")) path += "index.html";           // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  if(SPIFFS.exists(path)){  // If the file exists, either as a compressed archive, or normal                                       // Use the compressed version
	File file = SPIFFS.open(path, "r");
    auto s = file.size();
    server.send_P(200, contentType.c_str(), file.readString().c_str(), s);
    //size_t sent = server.streamFile(file, contentType);    // Send it to the client
    //Serial.print(string_format("wrote %d bytes", sent));
    file.close();                                          // Close the file again
    return true;
  }
  return false;                                          // If the file doesn't exist, return false
}
void OTASetup();
bool writeFile(String text, String path)
{
  int bufsize= text.length()*sizeof(char);
  FSInfo info;
  SPIFFS.info(info);

  unsigned int freebytes = info.totalBytes-info.usedBytes;
  return true;
}
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
  if(server.hasArg("startcure"))
  {
    String seconds =  server.arg("duration");
    curing_end_time = millis() + (atoi(seconds.c_str())*1000);
    curing = true;
    ledson=HIGH;
    digitalWrite(LEDPIN, ledson);
    motoren=LOW;
    digitalWrite(MOTORENPIN, motoren);
  }
    if(server.hasArg("toggleled"))
  {
        ledson=!ledson;
    digitalWrite(LEDPIN, ledson);

  }
      if(server.hasArg("toggletable"))
  {
    motoren=!motoren;
    digitalWrite(MOTORENPIN, motoren);
  }

  server.send(202);
}



boolean ConnectWifi()
{
  boolean state = true;
  int i = 0;
  String ss = ssid;
  String pw = password;
  WiFi.mode(WIFI_STA);
  Serial.println("trying....");
  WiFi.begin(ss, pw);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20){
      state = false;
      break;
    }
    i++;
  }
  if (state){
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Connection failed.");
  }

  return state;
}


void setup() {
  WiFi.hostname(hostname);
  pinMode(DIRPIN, OUTPUT);
  digitalWrite(DIRPIN, moveright);
  pinMode(MOTORENPIN, OUTPUT);
  digitalWrite(MOTORENPIN, motoren);
  pinMode(STEPPIN, OUTPUT);
  digitalWrite(STEPPIN, LOW);
    pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);
  // put your setup code here, to run once:
	Serial.begin(9600);
  Serial.println("test!");
	//ConnectWifi();
  WiFi.softAP(ssid, password);
// 	//WiFi.forceSleepBegin();
	auto res = SPIFFS.begin();
  server.on("/set/config", HTTP_PUT, handleConfigSet);

//   // this will be called for each packet received
  server.onNotFound([]() {                             // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      handleNotFound(); // otherwise, respond with a 404 (Not Found) error
  });
  if (MDNS.begin(hostname)) {
    Serial.println("MDNS responder started"); 
  }
  OTASetup();
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

void loop() {
  ArduinoOTA.handle();
  if (startupguard()==0)
    return;
  server.handleClient();

  MDNS.update();
  static int last_result_time = 0;
  if(curing && millis() > curing_end_time)
  {
    ledson=LOW;
    digitalWrite(LEDPIN, ledson);
    motoren=HIGH;
    digitalWrite(MOTORENPIN, motoren);
    curing = false;
  }

  server.handleClient();

}


void OTASetup()
{
  ArduinoOTA.setHostname("root");
  ArduinoOTA.setPassword(otapw);
  ArduinoOTA.setPort(8266);
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready"); 

}