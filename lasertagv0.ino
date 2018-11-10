/*
    lasertag game by Jim de Groot
    as per 28 okt 2018 under development
    currently V0.1
    available under the GNU General Public License V3.0
    for more information on the license please check the LICENSE.TXT file
*/

//**********************************************************************************************************************************************************************************************
//**********************************************************************************************************************************************************************************************
//********************************************************************* Global Variables *******************************************************************************************************
//**********************************************************************************************************************************************************************************************
//**********************************************************************************************************************************************************************************************

//Display driver
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//ESP8266 libraries
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <WebSocketsServer.h>

//IR libraries
#ifndef UNIT_TEST
#include <Arduino.h>
#endif
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

// wifi global variables
ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

ESP8266WebServer server(80);       // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81

File fsUploadFile;                 // a File variable to temporarily store the received file

const char *ssid = "lasertag"; // The name of the Wi-Fi network that will be created
const char *password = "pewpewpew";   // The password required to connect to it, leave blank for an open network

const char *OTAName = "ESP8266";           // A name and a password for the OTA service
const char *OTAPassword = "esp8266";

const char* mdnsName = "laser"; // Domain name for the mDNS responder

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

//Pin declarations
const byte IR_LED_Pin = 4; //IR Transmitter
const byte IR_Receive_Pin = 14; //IR receiver
const byte Trigger_Pin = 3; //Trigger switch
const byte Reload_Pin = 6; //Reload switch
const byte SpecAmmoButtonPin = 5;


//Game settings
byte playerno; // playernumber (1-12)
byte teamno; // teamnumber (blue = 1; red = 2) 
byte Ammo_Status; //Amount of ammo left
byte Clip_size; //give the start ammount of each clip
byte Life_Points; // Amount of lifes
const int Unit_Address = 1; //change the address for each unit
//bool Special_Ammo_Flag; //special fire mode (future use)
//byte Special_Ammo_Status;
byte sendCode;
int firerate = 1000; // rate of fire in full auto mode (mS)
int reload_rate = 10000; //how quickly can there be reloaded (mS)


//received data
byte rec_playerno;
byte rec_teamno;

//setup startup parameter flags
bool startflag = false;
bool Online_Offline; //Login to server or without server
bool sendCodeCalculation = false;

//global variables for debouncing or buttonreadings
long previoustime_fire;
long previoustime_reload;


//IR library
IRsend irsend(IR_LED_Pin);
IRrecv irrecv(IR_Receive_Pin);
decode_results incoming;
int ReceivedInt;
void setup() 
{
  //pinmode declerations
  pinMode(IR_LED_Pin, OUTPUT);
  pinMode(IR_Receive_Pin, INPUT);
  pinMode(Trigger_Pin, INPUT);
  pinMode(Reload_Pin, INPUT);
  pinMode(SpecAmmoButtonPin, INPUT);

  //lcd initializing
  lcd.begin();
  lcd.backlight();
  //some promo material
  lcd.setCursor(0, 0);
  lcd.println("lasertag v1.0");
  lcd.println("by Jim de Groot");
  delay(4000);
  lcd.clear();
}

void loop() 
{
  while (startflag == false) { //run sort of setup for starting without using setup function
    startup();
  }
  if (Online_Offline == true)
  {
  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();                      // run the server
  ArduinoOTA.handle();                        // listen for OTA events
  }

    if (sendCodeCalculation == false) //calculate txcode once
    {
        sendCode = (playerno + (teamno * 16)); // get a hex formatted no. like this 0x1(teamno 1)c(player 12)
    sendCodeCalculation = true;
    }

	button_read();
	
	}


void startup()
{
  bool flag = false; //print selection screen once
  if (flag == false) {
    lcd.setCursor(0, 0);
    lcd.println(">online mode");
    lcd.print("offline mode");
    flag = true;
  }

  bool displayflag = false; //local flag for determining what position the lcd has
  bool displayselect = digitalRead(Reload_Pin);  //read the input button to make cycle between screens
   bool dispayselect = digitalRead(Trigger_Pin); //read the input button to make selection
    
  if (displayselect == HIGH && displayflag == false ) //scroll to offline mode
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.println(">offline mode");
    lcd.print("online mode");
    bool displayflag = true;
  }

  else if (displayselect == HIGH && displayflag == true) //scroll to online mode
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.println(">online mode");
    lcd.print("offline mode");
    bool displayflag = true;
  }

 
 else if (displayselect == HIGH && displayflag == false) //selection button pressed going to online mode.
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.println("online selected");
    lcd.print("please wait...");
    delay(2000);
    startflag = true; //get out of the startup loop
    Online_Offline = true; //set flag for online mode so online loop goes into effect
    online(); // go to online function
  }

  else if (displayselect == HIGH && displayflag == true) //selection button pressed going to offline mode.
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.println("offline selected");
    lcd.print("please wait...");
    delay(1500);
    startflag = true; //set flag to exit startup while loop
    Online_Offline = false; //set flag to not enter the online loop
    offline(); //continue to offline setup parameters.
  }
}



void offline() //function for setting up the gameparameters when offline mode is used
{
  bool offlineflag = false; //set flag to loop the first offline menu only
  bool secondmenuflag = false; //go into the second offline menu flag
  while (offlineflag == false)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.println("select team");
    lcd.print(">blue");

    bool teamflag = false;
    bool displayflag = digitalRead(Reload_Pin);
    bool selectflag = digitalRead(Trigger_Pin);
    if (displayflag == HIGH && teamflag == false)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.println("select team");
      lcd.print(">red");
      teamflag = true;
    }
    else if (displayflag == HIGH && teamflag == true)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.println("select team");
      lcd.print(">red");
      teamflag = false;
    }


    else if (selectflag == HIGH && teamflag == false)
    {
      teamno = 1;
      offlineflag = true;
      secondmenuflag = true;

    }

    else if (selectflag == HIGH && teamflag == true)
    {
      teamno = 2;
      offlineflag = true;
      secondmenuflag = true;
    }

  }

  while (secondmenuflag = true) // go into the second menu
  {
    bool displayflag = digitalRead(Reload_Pin);
    bool selectflag = digitalRead(Trigger_Pin);
    byte i = 1;
    long previoustime;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.println("player? (1-12)");
    if (displayflag == HIGH && millis() - previoustime > 100)
    {
      lcd.setCursor(1, 0);
      lcd.print(i);
      previoustime = millis();
      i++;
      if (i = 12)
      {
        i = 1;
      }
    }
    if (selectflag == HIGH)
    {
      playerno = i;
    }
  }
}


void button_read(){ //read button states
	
	if(digitalRead(Trigger_Pin) == HIGH && millis() - previoustime_fire >= firerate && Ammo_Status > 0) //read triggerpin if button is pressed and ammo is available firing signal will be send.
	{
		trigger();
		previoustime_fire = millis();
	}

	if (digitalRead(Reload_Pin) == HIGH && millis() - previoustime_reload >=  reload_rate) //reload the clip give the gun the amount of bullets defined in Clip_size variable
	{
		Ammo_Status = Clip_size;
    previoustime_reload = millis();
	}
	
}
	
void trigger() // function to fire the led
{
  for(byte i=0; i<3 ; i++)
  {
	  irsend.sendSony(sendCode, 12);
  }
}

void received()
{
ReceivedInt = irrecv.decode(&incoming);
irrecv.resume();

}

void decode()
{
	rec_playerno = (ReceivedInt % 16); //take the modulo of the incoming signal (i.e. 18 % 16 = 2) 
	rec_teamno = ((ReceivedInt - rec_playerno)/16); //subtract playerno from incoming signal 18 - 2 divide it by 16 to get teamnumber
}








void online() //function to start WiFi
{
Serial.begin(115200);        // Start the Serial communication to send messages to the computer
  
  delay(10);
  
  Serial.println("\r\n");

  startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  
  startOTA();                  // Start the OTA service
  
  startSPIFFS();               // Start the SPIFFS and list all contents

  startWebSocket();            // Start a WebSocket server
  
  startMDNS();                 // Start the mDNS responder

  startServer();               // Start a HTTP server with a file read handler and an upload handler
  
}



void startWiFi() { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started\r\n");

  wifiMulti.addAP("ssid_from_AP_1", "your_password_for_AP_1");   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

  Serial.println("Connecting");
  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.println("\r\n");
  if(WiFi.softAPgetStationNum() == 0) {      // If the ESP is connected to an AP
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());             // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  } else {                                   // If a station is connected to the ESP SoftAP
    Serial.print("Station connected to ESP8266 AP");
  }
  Serial.println("\r\n");
}

void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
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
  Serial.println("OTA ready\r\n");
}

void startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
    }
    Serial.printf("\n");
  }
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", ""); 
  }, handleFileUpload);                       // go to 'handleFileUpload'

  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
                                              // and check if the file exists

  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}

void handleNotFound(){ // if the requested file or page doesn't exist, return a 404 not found error
  if(!handleFileRead(server.uri())){          // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleFileUpload(){ // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  String path;
  if(upload.status == UPLOAD_FILE_START){
    path = upload.filename;
    if(!path.startsWith("/")) path = "/"+path;
    if(!path.endsWith(".gz")) {                          // The file server always prefers a compressed version of a file 
      String pathWithGz = path+".gz";                    // So if an uploaded file is not compressed, the existing compressed
      if(SPIFFS.exists(pathWithGz))                      // version of that file must be deleted (if it exists)
         SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/success.html");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);
      }
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}


