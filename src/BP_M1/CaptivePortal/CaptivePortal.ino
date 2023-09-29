#include <WiFi.h> // https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/wifi.html
#include <DNSServer.h>
#include <WebServer.h>

#include <Preferences.h> // https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/

#include <SPI.h>
#include <SD.h>
#include <FS.h>

#include "HistoRWebPages.h"

#include "AudioTask.h"




// SD card - Digital I/O used
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18



const char AppName[] = "HistoR";

Preferences preferences;


// knihovny pro LCD přes I2C
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// nastavení adresy I2C (0x27 v mém případě),
// a dále počtu znaků a řádků LCD, zde 20x4
LiquidCrystal_I2C lcd(0x27, 16, 2); // SDA -> D21; SCL -> D22; VCC -> VIN (5V)
//https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/



/* WIFI */
IPAddress         apIP(10, 10, 10, 1);      // Private network for server
IPAddress         netMsk(255, 255, 255, 0); // Net Mask
WebServer         webServer(80);            // HTTP server
const byte        DNS_PORT = 53;            // Capture DNS requests on port 53
DNSServer         dnsServer;                // Create the DNS object

char WIFIssid[64] = "DodikovyInternety";
char WIFIpassword[64] = "dodikovachyse84";

char APssid[64] = "Stodola"; // default = AppName
char APpassword[64] = "12345678";

bool APactive = true;

unsigned long WIFIlastConnectTry = 0;
unsigned long WIFIconnectTryNumber = 0;
unsigned int WIFIstatus = WL_IDLE_STATUS;



/* AUDIO */
char AudioCurrentlyPlayingDescription[256] = {0};
char AudioSelectedSource[32] = "MP_SDcard";
char AudioLastPlayedTrack[256] = {0};
bool AudioRandomPlay = false;
char AudioLastInternetURL[256] = {0};
char AudioLastRadioFrequency[8] = {0};
bool AudioAutoPlay = false;

bool AudioFMtransActive = false;
char AudioFMtransFrequency[8] = {0};
bool AudioAMtransActive = false;
char AudioAMtransFrequency[8] = {0};


/* AUDIO TASK */
char AudioArtist[128] = {0};
char AudioTitle[128] = {0};
void audio_id3data(const char *info){  //id3 metadata
    Serial.print("id3data     ");Serial.println(info); // audio file Artist: ; Title: ;
    char *data = strchr(info, ' ');
    if (data != NULL) {
        data++;
        if (info[0] == 'A' && info[1] == 'r' && info[2] == 't' && data-info == 8) {
            strncpy(AudioArtist, data, 127); Serial.print("Artist=");Serial.println(data);
        }
        if (info[0] == 'T' && info[1] == 'i' && info[2] == 't' && data-info == 7) {
            strncpy(AudioTitle, data, 127); Serial.print("Title=");Serial.println(data);
        }
    }
}
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);// radio station
    strncpy(AudioArtist, info, 127);
}
void audio_showstreamtitle(const char *info){
    Serial.print("streamtitle ");Serial.println(info);// radio info
    strncpy(AudioTitle, info, 127);
}
/* AUDIO TASK - end */
bool AudioPlayerCreateDescription() {
    if (AudioArtist[0] != '\0' && AudioTitle[0] != '\0') {
        strcpy(AudioCurrentlyPlayingDescription, AudioArtist);
        strcat(AudioCurrentlyPlayingDescription, " - ");
        strcat(AudioCurrentlyPlayingDescription, AudioTitle);
        strcpy(AudioArtist, "");
        strcpy(AudioTitle, "");
        // TODO save to preferences
        return true;
    }
    return false;
}




void webServer_sendContentJavascriptSetElement(const char elementId[], char value[])
{
  webServer.sendContent(F("document.getElementById('"));
  webServer.sendContent(elementId);
  webServer.sendContent(F("').value = '"));
  webServer.sendContent(value);
  webServer.sendContent(F("';\n"));
}

String webServer_getArgValue(String argname) 
{
    for (int i=0; i < webServer.args(); i++) {
        if (webServer.argName(i) == argname){
            return webServer.arg(i);
        }
    }
    return "";
}

void webServer_printArgs()
{
      String message = "Number of args received:";
      message += webServer.args();            //Get number of parameters
      message += "\n";                            //Add a new line
      
      for (int i = 0; i < webServer.args(); i++) {
      
          message += "Arg n#" + (String)i + " –> ";   //Include the current iteration value
          message += webServer.argName(i) + ": ";     //Get the name of the parameter
          message += webServer.arg(i) + "\n";              //Get the value of the parameter
      
      }
      Serial.println(message);
}

bool isIP(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (!(c == '.' || (c >= '0' && c <= '9'))) {
      return false;
    }
  }
  return true;
}


bool I2CaddressIsActive(byte address) {
    Wire.begin();
    Wire.beginTransmission(address);
    return !Wire.endTransmission();
}


void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  
    Serial.print("listDir() is running on core ");
    Serial.println(xPortGetCoreID());
    
    Serial.printf("Listing directory: %s\n", dirname);
    webServer.sendContent(F("Listing directory: "));
    webServer.sendContent(dirname);
    webServer.sendContent(F("\n"));

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        webServer.sendContent(F("Failed to open directory\n"));
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        webServer.sendContent(F("Not a directory\n"));
        return;
    }

    webServer.sendContent(F("<table><tr><th>Type</th><th>Name</th><th>Size</th></tr>\n"));
    
    if(dirname[0] == '/' && dirname[1] != '\0'){
        int slen = strlen(dirname);
        char stemp[slen+1] = {0};
        strcpy(stemp, dirname);
        int i = slen;
        for(; i > 0; i--) {
            if(stemp[i] == '/') {
                stemp[i] = '\0';
                i = -1;
            }
        }
        if(i == 0) {
            stemp[1] = '\0';
        }
        Serial.print("UP dir: ");
        Serial.println(stemp);
        webServer.sendContent(F("<td>"));
        webServer.sendContent(FSH(folderImage));
        webServer.sendContent(F("DIR</td><td><a href='./SDSELECT?PATH="));
        webServer.sendContent(stemp);
        webServer.sendContent(F("'>..</a></td><td></td>\n"));
    }
    

    File file = root.openNextFile();
    while(file){
        webServer.sendContent(F("<tr>\n"));
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            webServer.sendContent(F("<td>"));
            webServer.sendContent(FSH(folderImage));
            webServer.sendContent(F("DIR</td><td><a href='./SDSELECT?PATH="));
            webServer.sendContent(dirname);
            if(dirname[0] == '/' && dirname[1] != '\0'){
                webServer.sendContent(F("/"));
            }
            webServer.sendContent(file.name());
            webServer.sendContent(F("'>"));
            Serial.println(file.name());
            webServer.sendContent(file.name());
            webServer.sendContent(F("</a></td><td></td>\n"));
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            webServer.sendContent(F("<td>"));
            webServer.sendContent(FSH(fileImage));
            webServer.sendContent(F("FILE</td><td><a href='./SDSELECT?PATH="));
            webServer.sendContent(dirname);
            webServer.sendContent(F("&PLAY="));
            webServer.sendContent(dirname);
            if(dirname[0] == '/' && dirname[1] != '\0'){
                webServer.sendContent(F("/"));
            }
            webServer.sendContent(file.name());
            webServer.sendContent(F("'>"));
            Serial.print(file.name());
            webServer.sendContent(file.name());
            Serial.print("  SIZE: ");
            webServer.sendContent(F("</a></td><td>"));
            Serial.println(file.size());
            webServer.sendContent(String(file.size()));
            webServer.sendContent(F("</td>\n"));
        }
        webServer.sendContent(F("</tr>\n"));
        file = root.openNextFile();
    }
}


void setup() {
    /* SETUP USB SERIAL */
    Serial.begin(9600);
    while(!Serial);
    Serial.println("START");
    Serial.print("setup() is running on core ");
    Serial.println(xPortGetCoreID());
    Serial.println("--------------------");
    

    /* PREFERENCES */ //TODO reset button
    /* // completely remove non-volatile storage (nvs) 
     * #include <nvs_flash.h>
    nvs_flash_erase(); // erase the NVS partition and...
    nvs_flash_init(); // initialize the NVS partition.
    */
    preferences.begin("my-app", false);// Note: Namespace name is limited to 15 chars.
    //preferences.clear();// Remove all preferences under the opened namespace
    preferences.getBytes("WIFISSID", WIFIssid, 64);
    preferences.getBytes("WIFIPASSWORD", WIFIpassword, 64);
    preferences.getBytes("APSSID", APssid, 64);
    preferences.getBytes("APPASSWORD", APpassword, 64);
    APactive = preferences.getBool("APACTIVE", APactive);
    preferences.end();


    /* SD CARD */
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI.setFrequency(1000000);
    if(!SD.begin(SD_CS)){
        Serial.println("Card Mount Failed");
    } else {
        Serial.println("Card Mount OK");
    }

    uint8_t cardType = SD.cardType();
    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
    } else {
        Serial.print("SD Card Type: ");
        if(cardType == CARD_MMC){
            Serial.println("MMC");
        } else if(cardType == CARD_SD){
            Serial.println("SDSC");
        } else if(cardType == CARD_SDHC){
            Serial.println("SDHC");
        } else {
            Serial.println("UNKNOWN");
        }
    }
    Serial.printf("SD Card Size: %lluMB\n", SD.cardSize() / (1024 * 1024));
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
    Serial.println("--------------------");


    /* AUDIO */
    audioInit();
    delay(1500);
    audioSetVolume(15);
    Serial.print("Current volume is: ");
    Serial.println(audioGetVolume());
    Serial.println("--------------------");


        
    /* LCD Display */
    if(I2CaddressIsActive(0x27)) {
        Serial.println("LCD display - start");
        // inicializace LCD
        lcd.begin();
        // zapnutí podsvícení
        lcd.backlight();
        
        lcd.setCursor(0, 1);
        lcd.print(AudioCurrentlyPlayingDescription);
        lcd.setCursor(0, 0);
        lcd.print("IP:");
        lcd.print(WiFi.localIP());
        Serial.println("LCD display");
    } else {
        Serial.println("LCD display - error: not found!");
    }
    Serial.println("--------------------");




    /* WIFI SETUP */
    // set both access point and station, AP without password is open
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIP, apIP, netMsk);
    if (APactive) {
        Serial.println("AP is Active!");
        Serial.println(APssid);
        Serial.println(APpassword);
        WiFi.softAP(APssid, APpassword);
    } else {
        Serial.println("AP is Deactivated!");
        WiFi.softAPdisconnect(true);
    }
    delay(500);
    Serial.println("--------------------");
    Serial.print("AP SSID: ");
    Serial.println(APssid);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("--------------------");



    /* DNS SERVER */
    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);
    // HTTPS doesn't work because of redirecting, certificate is always invalid



    /* WEB SERVER */
    // replay to all requests with same page
    webServer.onNotFound([]() {
        String requestUri = webServer.uri(); //.toCharArray(requestUri, sizeof(requestUri) + 1);
        if (webServer.args() > 0) {
          requestUri += "?";
          for (int i=0; i<webServer.args(); i++) {
            requestUri += webServer.argName(i);
            requestUri += "=";
            requestUri += webServer.arg(i);
            if (i+1<webServer.args()) {
               requestUri += "&";
            }
          }
        }
        Serial.println("requestUri: "+requestUri);
        Serial.print("Host: ");
        Serial.println(webServer.hostHeader());
        if(!isIP(webServer.hostHeader()) && webServer.hostHeader() != "histor.local") {
            //webServer.sendHeader("Connection", "keep-alive");
            webServer.sendHeader("Location", "http://histor.local/", true);
            webServer.send(302, "text/plain", "");
            webServer.client().stop();
            return;
        }
  
  
        webServer.setContentLength(CONTENT_LENGTH_UNKNOWN); // https://www.esp8266.com/viewtopic.php?p=73204
        // here begin chunked transfer
        webServer.send(200, "text/html", "");
        webServer.sendContent(FSH(HistoRHomePage));
        webServer.sendContent(F("<script>\n"));
        webServer_sendContentJavascriptSetElement("WIFI_SSID", WIFIssid);
        webServer_sendContentJavascriptSetElement("WIFI_PASSWORD", WIFIpassword);
        webServer_sendContentJavascriptSetElement("AP_SSID", APssid);
        webServer_sendContentJavascriptSetElement("AP_PASSWORD", APpassword);
        if (APactive) {webServer.sendContent(F("document.getElementById('AP_ACTIVE').checked = true;\n"));}
        webServer.sendContent(F("document.getElementById('"));
        webServer.sendContent(AudioSelectedSource);
        webServer.sendContent(F("').checked = true;\n"));
        if (AudioCurrentlyPlayingDescription[0] != '\0') {webServer_sendContentJavascriptSetElement("Mplayer", AudioCurrentlyPlayingDescription);}
        webServer.sendContent(F("</script>\n"));
        webServer.sendContent(F("")); // this tells web client that transfer is done
        webServer.client().stop();
    });
    
    webServer.on("/API/", HTTP_GET, []() {
        webServer_printArgs();
        
        preferences.begin("my-app", false);
  
        if (webServer.hasArg("CMD")) {
            String cmd = webServer_getArgValue("CMD");
            char temp[64] = {0};
            if (cmd == "WIFI") {
                if (webServer.hasArg("SSID")) {
                    webServer_getArgValue("SSID").toCharArray(temp, 63);
                    preferences.putBytes("WIFISSID", temp, 64);
                    strcpy(WIFIssid, temp);
                }
                if (webServer.hasArg("PASSWORD")) {
                    webServer_getArgValue("PASSWORD").toCharArray(temp, 63);
                    preferences.putBytes("WIFIPASSWORD", temp, 64);
                    strcpy(WIFIpassword, temp);
                }
            } else if (cmd == "AP") {
                if (webServer.hasArg("ACTIVE")) {
                    cmd = webServer_getArgValue("ACTIVE");
                    if (cmd == "true") {
                        preferences.putBool("APACTIVE", true);
                        APactive = true;
                    } else if (cmd == "false") {
                        preferences.putBool("APACTIVE", false);
                        APactive = false;
                    }
                }
                if (webServer.hasArg("SSID")) {
                    webServer_getArgValue("SSID").toCharArray(temp, 63);
                    preferences.putBytes("APSSID", temp, 64);
                    strcpy(APssid, temp);
                }
                if (webServer.hasArg("PASSWORD")) {
                    webServer_getArgValue("PASSWORD").toCharArray(temp, 63);
                    preferences.putBytes("APPASSWORD", temp, 64);
                    strcpy(APpassword, temp);
                }
            } else if (cmd == "RESTART") {
                Serial.println("RESTART!!!");
                preferences.end();
                webServer.send(200, "text/plain", "RESTART OK!");
                webServer.client().stop();
  
                delay(1500);
                ESP.restart();
            } else if (cmd == "MPSELECT") {
                String cmd2 = webServer_getArgValue("CMD2");
                String source = webServer_getArgValue("MPselected");
                
                if(cmd2 == "SAVEPLAY" || cmd2 == "SAVE") { // SAVE
                    char temp_str[256] = {0};
                    if (webServer.hasArg("MPselected")) {
                        webServer_getArgValue("MPselected").toCharArray(temp_str, 31);
                        strcpy(AudioSelectedSource, temp_str);
                        // TODO save to preferences
                    }
                    if (webServer.hasArg("INT_URL")) {
                        webServer_getArgValue("INT_URL").toCharArray(temp_str, 255);
                        strcpy(AudioLastInternetURL, temp_str);
                        // TODO save to preferences
                    }
                }
                
                if(cmd2 == "SAVEPLAY") { // PLAY
                    AudioCurrentlyPlayingDescription[0] = '\0';
                    if (source == "MP_SDcard") {
                        strcpy(AudioCurrentlyPlayingDescription, AudioLastPlayedTrack);
                        audioConnecttoSD(AudioLastPlayedTrack);
                    } else if (source == "MP_Internet") {
                        strcpy(AudioCurrentlyPlayingDescription, AudioLastPlayedTrack);
                        audioConnecttohost(AudioLastInternetURL);
                    } else if (source == "MP_Bluetooth") {
                    
                    } else if (source == "MP_Radio") {
                    
                    }
                }

                if(cmd2 == "STOP") { // STOP
                    if (source == "MP_SDcard") {
                        audioStopSong();
                    } else if (source == "MP_Internet") {
                        audioStopSong();
                    } else if (source == "MP_Bluetooth") {
                    
                    } else if (source == "MP_Radio") {
                    
                    }
                }
            
            } else if (cmd == "TRANS") { // transmitters
            
            
            } else if (cmd == "DESC") { // Currently playing description
              
                Serial.print("DESC: ");
                Serial.println(AudioCurrentlyPlayingDescription);
                preferences.end();
                webServer.send(200, "text/plain", AudioCurrentlyPlayingDescription);
                webServer.client().stop();
            
            }
        }
  
        preferences.end();
        
        webServer.send(200, "text/plain", "OK");       //Response to the HTTP request
        webServer.client().stop();
    });



    /*webServer.on("/STOPAP", HTTP_GET, []() {
      
        Serial.println("STOP AP!");
        WiFi.softAPdisconnect(true);
        
        webServer.send(200, "text/html", "STOP AP!");
        webServer.client().stop();
    });


    webServer.on("/STARTAP", HTTP_GET, []() {
      
        Serial.println("START AP!");
        WiFi.softAP(APssid, APpassword);
  
        webServer.send(200, "text/html", "START AP!");
        webServer.client().stop();
    });*/



    
    webServer.on("/SDSELECT", HTTP_GET, []() {
        
        if (webServer.hasArg("PLAY")) {
            char path[256] = {0};
            webServer_getArgValue("PLAY").toCharArray(path, 255);
            Serial.print("PLAY AUDIO: ");
            Serial.println(path);
            audioConnecttoSD(path);
            strcpy(AudioSelectedSource, "MP_SDcard"); // set SDcard audio source
            strcpy(AudioLastPlayedTrack, path);
            strcpy(AudioCurrentlyPlayingDescription, path);
            
            // redirect
            webServer.sendHeader("Location", "./", true);
            webServer.send(302, "text/plain", "");
            webServer.client().stop();
            return;
        }

        char path[256] = {0};
        if (webServer.hasArg("PATH")) {
            webServer_getArgValue("PATH").toCharArray(path, 255);
        } else {
            path[0] = '/';
            path[1] = '\0';
        }
        
        webServer.setContentLength(CONTENT_LENGTH_UNKNOWN); // https://www.esp8266.com/viewtopic.php?p=73204
        // here begin chunked transfer
        webServer.send(200, "text/html", "");
        
        listDir(SD, path, 0);

        webServer.sendContent(F("</table><br><br><a href='./'>close</a>\n"));
        webServer.sendContent(F("")); // this tells web client that transfer is done
        webServer.client().stop();
    });



    
    webServer.on("/WIFISCAN", HTTP_GET, []() {

        webServer.setContentLength(CONTENT_LENGTH_UNKNOWN); // https://www.esp8266.com/viewtopic.php?p=73204
        // here begin chunked transfer
        webServer.send(200, "text/html", "");

    
        Serial.println("Scan start");
        webServer.sendContent(F("Scan start\n"));
    
        // WiFi.scanNetworks will return the number of networks found.
        int n = WiFi.scanNetworks();
        Serial.println("Scan done");
        webServer.sendContent(F("Scan done<br>\n"));
        if (n == 0) {
            Serial.println("no networks found");
            webServer.sendContent(F("no networks found\n"));
        } else {
            Serial.print(n);
            webServer.sendContent(String(n));
            Serial.println(" networks found");
            webServer.sendContent(F(" networks found\n"));
            Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
            webServer.sendContent(F("<table><tr><th>Nr</th><th>SSID</th><th>RSSI</th><th>CH</th><th>Encryption</th></tr>\n<tr><td>"));
            for (int i = 0; i < n; ++i) {
                // Print SSID and RSSI for each network found
                Serial.printf("%2d",i + 1);
                webServer.sendContent(String(i + 1));
                Serial.print(" | ");
                webServer.sendContent(F("</td><td>"));
                Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
                webServer.sendContent(WiFi.SSID(i).c_str());
                Serial.print(" | ");
                webServer.sendContent(F("</td><td>"));
                Serial.printf("%4d", WiFi.RSSI(i));
                webServer.sendContent(String(WiFi.RSSI(i)));
                Serial.print(" | ");
                webServer.sendContent(F("</td><td>"));
                Serial.printf("%2d", WiFi.channel(i));
                webServer.sendContent(String(WiFi.channel(i)));
                Serial.print(" | ");
                webServer.sendContent(F("</td><td>"));
                switch (WiFi.encryptionType(i))
                {
                case WIFI_AUTH_OPEN:
                    Serial.print("open");
                    webServer.sendContent(F("open"));
                    break;
                case WIFI_AUTH_WEP:
                    Serial.print("WEP");
                    webServer.sendContent(F("WEP"));
                    break;
                case WIFI_AUTH_WPA_PSK:
                    Serial.print("WPA");
                    webServer.sendContent(F("WPA"));
                    break;
                case WIFI_AUTH_WPA2_PSK:
                    Serial.print("WPA2");
                    webServer.sendContent(F("WPA2"));
                    break;
                case WIFI_AUTH_WPA_WPA2_PSK:
                    Serial.print("WPA+WPA2");
                    webServer.sendContent(F("WPA+WPA2"));
                    break;
                case WIFI_AUTH_WPA2_ENTERPRISE:
                    Serial.print("WPA2-EAP");
                    webServer.sendContent(F("WPA2-EAP"));
                    break;
                case WIFI_AUTH_WPA3_PSK:
                    Serial.print("WPA3");
                    webServer.sendContent(F("WPA3"));
                    break;
                case WIFI_AUTH_WPA2_WPA3_PSK:
                    Serial.print("WPA2+WPA3");
                    webServer.sendContent(F("WPA2+WPA3"));
                    break;
                case WIFI_AUTH_WAPI_PSK:
                    Serial.print("WAPI");
                    webServer.sendContent(F("WAPI"));
                    break;
                default:
                    Serial.print("unknown");
                    webServer.sendContent(F("unknown"));
                }
                Serial.println();
                webServer.sendContent(F("</td></tr>\n"));
                delay(10);
            }
        }
        Serial.println("");
        webServer.sendContent(F("</table>\n"));
    
        // Delete the scan result to free memory for code below.
        WiFi.scanDelete();

  
        webServer.sendContent(F("<br><br><a href='javascript:window.close();'>close</a>\n"));
        webServer.sendContent(F("")); // this tells web client that transfer is done
        webServer.client().stop();
    });
  
    webServer.begin();
}


void loop() {
    dnsServer.processNextRequest();
    webServer.handleClient();
    if (WiFi.status() != WL_NO_SHIELD) {
        if (WiFi.status() != WL_CONNECTED && (millis() > (WIFIlastConnectTry + 60000) || WIFIlastConnectTry == 0) && WIFIconnectTryNumber < 2) {
            Serial.println("Connecting to WIFI...");
            WiFi.disconnect();
            WiFi.begin(WIFIssid, WIFIpassword);
            WIFIlastConnectTry = millis();
            WIFIconnectTryNumber++;
            if (WIFIconnectTryNumber == 2) {
                Serial.println("WIFI connection lost/failed!");
                Serial.println("START AP!");
                if (APssid[0] == '\0' || strlen(APssid) < 8) {
                    strcpy(APssid, AppName);
                }
                Serial.println(APssid);
                Serial.println(APpassword);
                WiFi.softAP(APssid, APpassword);
            }
        }
        if(WiFi.status() != WIFIstatus){
            WIFIstatus = WiFi.status();
            Serial.println("WIFI status changed: "+String(WIFIstatus));
            if (WIFIstatus == WL_CONNECTED) {
                WIFIconnectTryNumber = 0;
                Serial.println("WL_CONNECTED");
                Serial.println("--------------------");
                Serial.print("Connected to: ");
                Serial.println(WiFi.SSID());
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
                Serial.println("--------------------");
                
                lcd.setCursor(0, 0);
                lcd.print("IP:");
                lcd.print(WiFi.localIP());
            }
        }
    } else {
        Serial.println("WiFi shield not found!");
    }

    if (AudioPlayerCreateDescription()) {
        lcd.setCursor(0, 1);
        lcd.print(AudioCurrentlyPlayingDescription);
        lcd.setCursor(0, 0);
        lcd.print("IP:");
        lcd.print(WiFi.localIP());
        Serial.println("LCD display");
    }
}
