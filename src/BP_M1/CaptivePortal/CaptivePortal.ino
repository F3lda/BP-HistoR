#include <WiFi.h> // https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/wifi.html
#include <DNSServer.h>
#include <WebServer.h>

#include <Preferences.h> // https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/

#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <SPIFFS.h>

#include "HistoRWebPages.h"

#include "AudioTask.h"


#define BP_ESP_SLAVE_ID 8

#define SDCARD_MAX_PATH_LENGTH 256



// SD card - Digital I/O used
#define SD_CS          5
#define SPI_MOSI      19
#define SPI_MISO      23
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

char WIFIssid[64] = "ACMOTO";
char WIFIpassword[64] = "vnuci321";

char APssid[64] = "HistoR"; // default = AppName
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
bool AudioRepeatAll = false;
bool AudioRepeatOne = false;
char AudioLastInternetURL[256] = {0};
char AudioBluetoothName[256] = {0};
char AudioLastRadioFrequency[8] = {0};
bool AudioAutoPlay = false;
char AudioVolume = 10; // TODO save UI preferences

bool AudioFMtransActive = false;
char AudioFMtransFrequency[8] = {0};
bool AudioAMtransActive = false;
char AudioAMtransFrequency[8] = {0};


bool SDcardAlbumPlaying = false;
char SDcardNextTrackPath[512] = {0};
void AudioPlayerSDcardPlayTrack(void *parameter)
{
    SDcardAlbumPlaying = true;

    if(parameter != NULL){
        strcpy(SDcardNextTrackPath, (const char *)parameter);
    } else {
        //delay(3000);
    }
    Serial.print("Music player: Playing new track: ");
    Serial.println(SDcardNextTrackPath);

    audioStopSong();
    audioConnecttoSD(SDcardNextTrackPath);

    strcpy(AudioSelectedSource, "MP_SDcard"); // set SDcard audio source
    strcpy(AudioLastPlayedTrack, SDcardNextTrackPath);
    strcpy(AudioCurrentlyPlayingDescription, SDcardNextTrackPath);

    preferences.begin("my-app", false);
    preferences.putBytes("AU_SOURCE", AudioSelectedSource, 32);
    preferences.putBytes("AU_LAST_TRACK", AudioLastPlayedTrack, 255);
    preferences.end();

    SDcardNextTrackPath[0] = '\0';
}




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
void AudioPlayerSDcardFindTrack(fs::FS &fs, const char * dirname, const char * filename){

    Serial.print("AudioPlayerSDcardFindTrack() is running on core ");
    Serial.println(xPortGetCoreID());

    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    bool lastTrackFound = false;

    File file = root.openNextFile();
    while(file){

        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());

            if(lastTrackFound == true) {
                strcpy(SDcardNextTrackPath, dirname);
                strcat(SDcardNextTrackPath, "/");
                strcat(SDcardNextTrackPath, file.name());
                root.close();
                return;
            }
            if(strcmp(filename, file.name()) == 0 && lastTrackFound == false) {
                lastTrackFound = true;
            }
        }
        file = root.openNextFile();
    }

    SDcardAlbumPlaying = false;
    Serial.println("Music player: end of album/folder!");

    root.close();
}
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
    Serial.print("eof_mp3() is running on core ");
    Serial.println(xPortGetCoreID());

    Serial.print("SDcardAlbumPlaying: ");
    Serial.println(SDcardAlbumPlaying);

    if(SDcardAlbumPlaying == true) {//audioisrunning == false &&
        char AudioLastPlayedSDcardNextTrackPath[256] = {0};
        strcpy(AudioLastPlayedSDcardNextTrackPath, AudioLastPlayedTrack);
        int i = strlen(AudioLastPlayedSDcardNextTrackPath);
        for(; i > 0; i--) {
            if(AudioLastPlayedSDcardNextTrackPath[i] == '/') {
                AudioLastPlayedSDcardNextTrackPath[i] = '\0';
                i = -1;
            }
        }
        if(i == 0) {
            AudioLastPlayedSDcardNextTrackPath[1] = '\0';
        }

        AudioPlayerSDcardFindTrack(SD, AudioLastPlayedSDcardNextTrackPath, info);
    }

}
void audioStartStop(bool audioisrunning){
    Serial.print("audioisrunning    ");Serial.println(audioisrunning);
    Serial.print("audioStartStop() is running on core ");
    Serial.println(xPortGetCoreID());
}
/* AUDIO TASK - end */


bool AudioPlayerCreateDescription() {
    if (strcmp(AudioSelectedSource, "MP_SDcard") == 0 || strcmp(AudioSelectedSource, "MP_Internet") == 0) {
        if (AudioArtist[0] != '\0' && AudioTitle[0] != '\0') {
            strcpy(AudioCurrentlyPlayingDescription, AudioArtist);
            strcat(AudioCurrentlyPlayingDescription, " - ");
            strcat(AudioCurrentlyPlayingDescription, AudioTitle);
            strcpy(AudioArtist, "");
            strcpy(AudioTitle, "");
        } else if (strcmp(AudioCurrentlyPlayingDescription, "SDcard          ") != 0 && strcmp(AudioSelectedSource, "MP_SDcard") == 0 && SDcardAlbumPlaying == false) {
            strcpy(AudioCurrentlyPlayingDescription, "SDcard          "); // spaces because of LCD display
        } else {
            return false;
        }
    } else if(strcmp(AudioSelectedSource, "MP_Bluetooth") == 0) {
        if (strcmp(AudioCurrentlyPlayingDescription, "Bluetooth       ") != 0) {
            strcpy(AudioCurrentlyPlayingDescription, "Bluetooth       "); // spaces because of LCD display
        } else {
            return false;
        }
    } else if(strcmp(AudioSelectedSource, "MP_Radio") == 0) {
        if (strcmp(AudioCurrentlyPlayingDescription, "Radio           ") != 0) {
            strcpy(AudioCurrentlyPlayingDescription, "Radio           "); // spaces because of LCD display
        } else {
            return false;
        }
    }
    // replace double quotes for javascript
    for(int i = 0; i < strlen(AudioCurrentlyPlayingDescription); i++){
        if (AudioCurrentlyPlayingDescription[i] == '"'){
            AudioCurrentlyPlayingDescription[i] = '\'';
        }
    }
    // save to preferences
    preferences.begin("my-app", false);
    preferences.putBytes("AU_DESCRIPTION", AudioCurrentlyPlayingDescription, 256);
    preferences.end();
    return true;
}

void AudioPlayerStopAllSources() {
    // Stop SDcard + Internet
    SDcardAlbumPlaying = false;
    audioStopSong();
    // Stop Bluetooth
    Wire.beginTransmission(BP_ESP_SLAVE_ID);
    Wire.write(0);
    Wire.write(0);
    Wire.write("BO");
    Wire.write(0);
    Wire.endTransmission();
    // Stop radio
    //TODO
}



#define WEBSERVER_SEND_BUFFER_SIZE 512
char WebserverSendBuffer[WEBSERVER_SEND_BUFFER_SIZE] = {0};
int WebserverSendBufferLength = 0;
void webServer_bufferContentFlush()
{
    webServer_bufferContentAddChar("");
}

void webServer_bufferContentAddChar(const char value[])
{
    int value_length = strlen(value);
    if (WebserverSendBufferLength + value_length >= WEBSERVER_SEND_BUFFER_SIZE || value[0] == '\0') {
        webServer.sendContent(WebserverSendBuffer);
        WebserverSendBuffer[0] = '\0';
        WebserverSendBufferLength = 0;
    }
    WebserverSendBufferLength += value_length;
    strcat(WebserverSendBuffer, value);
    WebserverSendBuffer[WebserverSendBufferLength] = '\0';
}

void webServer_bufferContentAddInt(int value)
{
    char intvalue[32] = {0};
    sprintf(intvalue, "%d", value);
    webServer_bufferContentAddChar(intvalue);
}

void webServer_bufferContentAddJavascriptSetElementChecked(const char elementId[])
{
    webServer_bufferContentAddChar("document.getElementById('");
    webServer_bufferContentAddChar(elementId);
    webServer_bufferContentAddChar("').checked = true;\n");
}

void webServer_bufferContentAddJavascriptSetElementValue(const char elementId[], char value[])
{
    webServer_bufferContentAddChar("document.getElementById('");
    webServer_bufferContentAddChar(elementId);
    webServer_bufferContentAddChar("').value = \"");
    webServer_bufferContentAddChar(value);
    webServer_bufferContentAddChar("\";\n");
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

void WebServerPrintArgs()
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

void WebserverListDir(fs::FS &fs, const char * dirname)
{
    Serial.print("listDir() is running on core ");
    Serial.println(xPortGetCoreID());


    webServer_bufferContentAddChar("<!DOCTYPE html><html><head><title>HistoR - Music player - select track</title></head><body>");


    Serial.printf("Listing directory: %s\n", dirname);
    webServer_bufferContentAddChar("Listing directory: ");
    webServer_bufferContentAddChar(dirname);
    webServer_bufferContentAddChar("\n");

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        webServer_bufferContentAddChar("Failed to open directory\n");
        webServer_bufferContentFlush();
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        root.close();
        webServer_bufferContentAddChar("Not a directory\n");
        webServer_bufferContentFlush();
        return;
    }

    webServer_bufferContentAddChar("<table><tr><th>Type</th><th>Name</th><th>Size</th></tr>\n");

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
        webServer_bufferContentAddChar("<td><img title=\"Folder\" width=\"20px\" style=\"margin-bottom:-4px\" src=\"./IMG?FOLDER=\" />DIR</td><td><a href='./SDSELECT?PATH=");
        webServer_bufferContentAddChar(stemp);
        webServer_bufferContentAddChar("'>..</a></td><td></td>\n");
    }


    File file = root.openNextFile();
    while(file){
        webServer_bufferContentAddChar("<tr>\n");
        if(file.isDirectory()){
            //Serial.print("  DIR : ");
            webServer_bufferContentAddChar("<td><img title=\"Folder\" width=\"20px\" style=\"margin-bottom:-4px\" src=\"./IMG?FOLDER=\" />DIR</td><td><a href='./SDSELECT?PATH=");
            webServer_bufferContentAddChar(dirname);
            if(dirname[0] == '/' && dirname[1] != '\0'){
                webServer_bufferContentAddChar("/");
            }
            webServer_bufferContentAddChar(file.name());
            webServer_bufferContentAddChar("'>");
            //Serial.println(file.name());
            webServer_bufferContentAddChar(file.name());
            webServer_bufferContentAddChar("</a></td><td></td>\n");
        } else {
            //Serial.print("  FILE: ");
            webServer_bufferContentAddChar("<td><img title=\"File\" width=\"20px\" style=\"margin-bottom:-4px\" src=\"./IMG?DOCUMENT=\" />FILE</td><td><a href='./SDSELECT?PATH=");
            webServer_bufferContentAddChar(dirname);
            webServer_bufferContentAddChar("&PLAY=");
            webServer_bufferContentAddChar(dirname);
            if(dirname[0] == '/' && dirname[1] != '\0'){
                webServer_bufferContentAddChar("/");
            }
            webServer_bufferContentAddChar(file.name());
            webServer_bufferContentAddChar("'>");
            //Serial.print(file.name());
            webServer_bufferContentAddChar(file.name());
            //Serial.print("  SIZE: ");
            webServer_bufferContentAddChar("</a></td><td>");
            //Serial.println(file.size());
            webServer_bufferContentAddInt(file.size());
            webServer_bufferContentAddChar("</td>\n");
        }
        webServer_bufferContentAddChar("</tr>\n");
        file = root.openNextFile();
    }

    root.close();


    webServer_bufferContentAddChar("</table><br><br><a href='./'>close</a>\n");

    webServer_bufferContentAddChar("</body></html>\n");


    webServer_bufferContentFlush();
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
    // WIFI
    preferences.getBytes("WIFISSID", WIFIssid, 64);
    preferences.getBytes("WIFIPASSWORD", WIFIpassword, 64);
    preferences.getBytes("APSSID", APssid, 64);
    preferences.getBytes("APPASSWORD", APpassword, 64);
    APactive = preferences.getBool("APACTIVE", APactive);
    // Audio
    preferences.getBytes("AU_DESCRIPTION", AudioCurrentlyPlayingDescription, 256);
    preferences.getBytes("AU_SOURCE", AudioSelectedSource, 32);
    preferences.getBytes("AU_LAST_TRACK", AudioLastPlayedTrack, 256);
    AudioRandomPlay = preferences.getBool("AU_RANDOM_PLAY", AudioRandomPlay);
    AudioRepeatAll = preferences.getBool("AU_REPEAT_ALL", AudioRepeatAll);
    AudioRepeatOne = preferences.getBool("AU_REPEAT_ONE", AudioRepeatOne);
    preferences.getBytes("AU_LAST_URL", AudioLastInternetURL, 256);
    preferences.getBytes("AU_BT_NAME", AudioBluetoothName, 256);
    preferences.getBytes("AU_LAST_RADIO", AudioLastRadioFrequency, 8);
    AudioAutoPlay = preferences.getBool("AU_AUTOPLAY", AudioAutoPlay);
    AudioVolume = preferences.getChar("AU_VOLUME", AudioVolume);
    // Transmitters
    AudioFMtransActive = preferences.getBool("AU_FM_ACTIVE", AudioFMtransActive);
    preferences.getBytes("AU_FM_FREQ", AudioFMtransFrequency, 8);
    AudioAMtransActive = preferences.getBool("AU_AM_ACTIVE", AudioAMtransActive);
    preferences.getBytes("AU_AM_FREQ", AudioAMtransFrequency, 8);
    preferences.end();





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





    /* SD CARD */
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI.setFrequency(1000000);
    if(!SD.begin(SD_CS)){
        Serial.println("Card Mount Failed");
        if(I2CaddressIsActive(0x27)) {
            lcd.setCursor(0, 1);
            lcd.print("Card Mount Fail!");
        }
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
    audioSetVolume(AudioVolume); // 0...21
    Serial.print("Current volume is: ");
    Serial.println(audioGetVolume());
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
        Serial.print("WEBSERVER is running on core ");
        Serial.println(xPortGetCoreID());

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
        webServer.send(200, "text/html", "<!--- DOCUMENT START --->");


        webServer.sendContent(FSH(HistoRHomePage));
        webServer_bufferContentAddChar("<script>\n");
        // WIFI
        webServer_bufferContentAddJavascriptSetElementValue("WIFI_SSID", WIFIssid);
        webServer_bufferContentAddJavascriptSetElementValue("WIFI_PASSWORD", WIFIpassword);
        webServer_bufferContentAddJavascriptSetElementValue("AP_SSID", APssid);
        webServer_bufferContentAddJavascriptSetElementValue("AP_PASSWORD", APpassword);
        if (APactive) {webServer_bufferContentAddJavascriptSetElementChecked("AP_ACTIVE");}
        // Audio
        if (AudioCurrentlyPlayingDescription[0] != '\0') {webServer_bufferContentAddJavascriptSetElementValue("Mplayer", AudioCurrentlyPlayingDescription);}
        Serial.println(AudioSelectedSource);
        webServer_bufferContentAddJavascriptSetElementChecked(AudioSelectedSource);
        if (AudioRandomPlay) {webServer_bufferContentAddJavascriptSetElementChecked("MP_RANDOM");}
        if (AudioRepeatAll) {webServer_bufferContentAddJavascriptSetElementChecked("MP_REPEAT_ALL");}
        if (AudioRepeatOne) {webServer_bufferContentAddJavascriptSetElementChecked("MP_REPEAT_ONE");}
        if (AudioLastInternetURL[0] != '\0') {webServer_bufferContentAddJavascriptSetElementValue("INT_URL", AudioLastInternetURL);}
        if (AudioBluetoothName[0] != '\0') {webServer_bufferContentAddJavascriptSetElementValue("BT_NAME", AudioBluetoothName);}
        if (AudioLastRadioFrequency[0] != '\0') {webServer_bufferContentAddJavascriptSetElementValue("R_FREQ", AudioLastRadioFrequency);}
        if (AudioAutoPlay) {webServer_bufferContentAddJavascriptSetElementChecked("MP_AUTO");}
        char str[5]; sprintf(str, "%d", (int)AudioVolume); webServer_bufferContentAddJavascriptSetElementValue("MP_VOLUME", str);

        if (AudioFMtransActive) {webServer_bufferContentAddJavascriptSetElementChecked("FM_ACTIVE");}
        if (AudioFMtransFrequency[0] != '\0') {webServer_bufferContentAddJavascriptSetElementValue("FM_FREQ", AudioFMtransFrequency);}
        if (AudioAMtransActive) {webServer_bufferContentAddJavascriptSetElementChecked("AM_ACTIVE");}
        //if (AudioAMtransFrequency[0] != '\0') {webServer_bufferContentAddJavascriptSetElementValue("", AudioAMtransFrequency);}

        webServer_bufferContentAddChar("</script>\n");

        webServer_bufferContentFlush();


        webServer.sendContent(F("")); // this tells web client that transfer is done
        webServer.client().stop();

        Serial.println("\nHome Page Loaded!\n");
    });

    webServer.on("/API/", HTTP_GET, []() {
        WebServerPrintArgs();

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

                Wire.beginTransmission(BP_ESP_SLAVE_ID);
                Wire.write(0);
                Wire.write(0);
                Wire.write("ER");
                Wire.endTransmission();
                delay(10);

                delay(1500);
                ESP.restart();
            } else if (cmd == "MPSELECT") {
                String cmd2 = webServer_getArgValue("CMD2");
                String source = webServer_getArgValue("MPselected");

                if(cmd2 == "SAVEPLAY" || cmd2 == "SAVE") { // SAVE
                    char temp_str[256] = {0};
                    String bin = "";
                    // SOURCE
                    if (webServer.hasArg("MPselected")) {
                        webServer_getArgValue("MPselected").toCharArray(temp_str, 31);
                        preferences.putBytes("AU_SOURCE", temp_str, 31);
                        if (strcmp(AudioSelectedSource, temp_str) != 0) { // source changed -> stop all
                            AudioPlayerStopAllSources();
                        }
                        strcpy(AudioSelectedSource, temp_str);
                    }
                    // Random play
                    if (webServer.hasArg("MP_RANDOM")) {
                        bin = webServer_getArgValue("MP_RANDOM");
                        if (bin == "true") {
                            preferences.putBool("AU_RANDOM_PLAY", true);
                            AudioRandomPlay = true;
                        } else if (bin == "false") {
                            preferences.putBool("AU_RANDOM_PLAY", false);
                            AudioRandomPlay = false;
                        }
                    }
                    // Repeat all
                    if (webServer.hasArg("MP_REPEAT_ALL")) {
                        bin = webServer_getArgValue("MP_REPEAT_ALL");
                        if (bin == "true") {
                            preferences.putBool("AU_REPEAT_ALL", true);
                            AudioRepeatAll = true;
                        } else if (bin == "false") {
                            preferences.putBool("AU_REPEAT_ALL", false);
                            AudioRepeatAll = false;
                        }
                    }
                    // Repeat one
                    if (webServer.hasArg("MP_REPEAT_ONE")) {
                        bin = webServer_getArgValue("MP_REPEAT_ONE");
                        if (bin == "true") {
                            preferences.putBool("AU_REPEAT_ONE", true);
                            AudioRepeatOne = true;
                        } else if (bin == "false") {
                            preferences.putBool("AU_REPEAT_ONE", false);
                            AudioRepeatOne = false;
                        }
                    }
                    // Internet URL
                    if (webServer.hasArg("INT_URL")) {
                        webServer_getArgValue("INT_URL").toCharArray(temp_str, 255);
                        preferences.putBytes("AU_LAST_URL", temp_str, 255);
                        strcpy(AudioLastInternetURL, temp_str);
                    }
                    // Bluettoth name
                    if (webServer.hasArg("BT_NAME")) {
                        webServer_getArgValue("BT_NAME").toCharArray(temp_str, 255);
                        preferences.putBytes("AU_BT_NAME", temp_str, 255);
                        strcpy(AudioBluetoothName, temp_str);
                    }
                    // Radio frequency
                    if (webServer.hasArg("R_FREQ")) {
                        webServer_getArgValue("R_FREQ").toCharArray(temp_str, 8);
                        preferences.putBytes("AU_LAST_RADIO", temp_str, 8);
                        strcpy(AudioLastRadioFrequency, temp_str);
                    }
                    // Autoplay
                    if (webServer.hasArg("MP_AUTO")) {
                        bin = webServer_getArgValue("MP_AUTO");
                        if (bin == "true") {
                            preferences.putBool("AU_AUTOPLAY", true);
                            AudioAutoPlay = true;
                        } else if (bin == "false") {
                            preferences.putBool("AU_AUTOPLAY", false);
                            AudioAutoPlay = false;
                        }
                    }
                    // Audio Volume
                    if (webServer.hasArg("MP_VOLUME")) {
                        webServer_getArgValue("MP_VOLUME").toCharArray(temp_str, 3);
                        int vol = 0;
                        sscanf(temp_str, "%d", &vol);
                        if (AudioVolume != (char) vol) {
                            AudioVolume = (char) vol;
                            preferences.putChar("AU_VOLUME", AudioVolume);
                            audioSetVolume(vol);
                        }
                    }
                }

                if(cmd2 == "SAVEPLAY") { // PLAY
                    AudioCurrentlyPlayingDescription[0] = '\0';
                    if (source == "MP_SDcard") {
                        AudioPlayerSDcardPlayTrack(AudioLastPlayedTrack);
                    } else if (source == "MP_Internet") {
                        audioStopSong();
                        strcpy(AudioCurrentlyPlayingDescription, AudioLastInternetURL);
                        audioConnecttohost(AudioLastInternetURL);
                    } else if (source == "MP_Bluetooth") {
                        Wire.beginTransmission(BP_ESP_SLAVE_ID);
                        Wire.write(0);
                        Wire.write(0);
                        Wire.write("BN");
                        Wire.write(AudioBluetoothName);
                        Wire.endTransmission();
                        delay(10);
                        Wire.beginTransmission(BP_ESP_SLAVE_ID);
                        Wire.write(0);
                        Wire.write(0);
                        Wire.write("BO");
                        Wire.write(1);
                        Wire.endTransmission();
                    } else if (source == "MP_Radio") {
                        Wire.beginTransmission(BP_ESP_SLAVE_ID);
                        Wire.write(0);
                        Wire.write(0);
                        Wire.write("RV");
                        Wire.write(15);
                        Wire.endTransmission();
                        delay(10);

                        char strTemp[10] = {0};
                        int int1 = 997;
                        int int2 = 0;
                        sscanf(AudioLastRadioFrequency, "%d.%d",&int1,&int2);
                        sprintf(strTemp, "%d%d", int1, int2);
                        Serial.println(strTemp);
                        sscanf(strTemp, "%d", &int1);
                        Serial.print("Radio Frequency: ");
                        Serial.println(int1);

                        Wire.beginTransmission(BP_ESP_SLAVE_ID);
                        Wire.write(0);
                        Wire.write(0);
                        Wire.write("RT");
                        Wire.write((byte)(int1-900));
                        Wire.endTransmission();
                    }
                }

                if(cmd2 == "STOP") { // STOP
                    if (source == "MP_SDcard") {
                        SDcardAlbumPlaying = false;
                        audioStopSong();
                    } else if (source == "MP_Internet") {
                        audioStopSong();
                    } else if (source == "MP_Bluetooth") {
                        Wire.beginTransmission(BP_ESP_SLAVE_ID);
                        Wire.write(0);
                        Wire.write(0);
                        Wire.write("BO");
                        Wire.write(0);
                        Wire.endTransmission();
                    } else if (source == "MP_Radio") {
                        Wire.beginTransmission(BP_ESP_SLAVE_ID);
                        Wire.write(0);
                        Wire.write(0);
                        Wire.write("RV");
                        Wire.write(0);
                        Wire.endTransmission();
                    }
                }

            } else if (cmd == "TRANS") { // transmitters
                // FM active
                if (webServer.hasArg("FM_ACTIVE")) {
                    cmd = webServer_getArgValue("FM_ACTIVE");
                    if (cmd == "true") {
                        preferences.putBool("AU_FM_ACTIVE", true);
                        AudioFMtransActive = true;
                    } else if (cmd == "false") {
                        preferences.putBool("AU_FM_ACTIVE", false);
                        AudioFMtransActive = false;
                    }
                }
                // FM frequency
                if (webServer.hasArg("FM_FREQ")) {
                    char temp_str[8] = {0};
                    webServer_getArgValue("FM_FREQ").toCharArray(temp_str, 8);
                    preferences.putBytes("AU_FM_FREQ", temp_str, 8);
                    strcpy(AudioFMtransFrequency, temp_str);
                }
                // AM active
                if (webServer.hasArg("AM_ACTIVE")) {
                    cmd = webServer_getArgValue("AM_ACTIVE");
                    if (cmd == "true") {
                        preferences.putBool("AU_AM_ACTIVE", true);
                        AudioAMtransActive = true;
                    } else if (cmd == "false") {
                        preferences.putBool("AU_AM_ACTIVE", false);
                        AudioAMtransActive = false;
                    }
                }

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




    webServer.on("/RESTART", HTTP_GET, []() {

        Serial.println("RESTART OK!");

        webServer.send(200, "text/html", "RESTART OK!\n<br><br><a href='./'>Home Page</a>");
        webServer.client().stop();

        delay(1000);
        ESP.restart();
    });





    webServer.on("/SDSELECT", HTTP_GET, []() {

        Serial.print("SDSELECT is running on core ");
        Serial.println(xPortGetCoreID());


        if (webServer.hasArg("PLAY")) {
            char path[256] = {0};
            webServer_getArgValue("PLAY").toCharArray(path, 255);
            Serial.print("PLAY AUDIO: ");
            Serial.println(path);

            AudioPlayerStopAllSources();
            AudioPlayerSDcardPlayTrack(path);

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
        webServer.send(200, "text/html", "<!--- DOCUMENT START --->");



        WebserverListDir(SD, path);



        webServer.sendContent(F("")); // this tells web client that transfer is done
        webServer.client().stop();
    });




    webServer.on("/WIFISCAN", HTTP_GET, []() {

        webServer.setContentLength(CONTENT_LENGTH_UNKNOWN); // https://www.esp8266.com/viewtopic.php?p=73204
        // here begin chunked transfer
        webServer.send(200, "text/html", "");

        webServer_bufferContentAddChar("<!DOCTYPE html><html><head><title>HistoR - WIFI SCANNER</title></head><body>");

        Serial.println("Scan start");
        webServer_bufferContentAddChar("Scan start\n");

        // WiFi.scanNetworks will return the number of networks found.
        int n = WiFi.scanNetworks();
        Serial.println("Scan done");
        webServer_bufferContentAddChar("Scan done<br>\n");
        if (n == 0) {
            Serial.println("no networks found");
            webServer_bufferContentAddChar("no networks found\n");
        } else {
            Serial.print(n);
            webServer_bufferContentAddInt(n);
            Serial.println(" networks found");
            webServer_bufferContentAddChar(" networks found\n");
            Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
            webServer_bufferContentAddChar("<table><tr><th>Nr</th><th>SSID</th><th>RSSI</th><th>CH</th><th>Encryption</th></tr>\n");
            for (int i = 0; i < n; ++i) {
                webServer_bufferContentAddChar("<tr><td>");
                // Print SSID and RSSI for each network found
                Serial.printf("%2d",i + 1);
                webServer_bufferContentAddInt(i + 1);
                Serial.print(" | ");
                webServer_bufferContentAddChar("</td><td>");
                Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
                webServer_bufferContentAddChar(WiFi.SSID(i).c_str());
                Serial.print(" | ");
                webServer_bufferContentAddChar("</td><td>");
                Serial.printf("%4d", WiFi.RSSI(i));
                webServer_bufferContentAddInt(WiFi.RSSI(i));
                Serial.print(" | ");
                webServer_bufferContentAddChar("</td><td>");
                Serial.printf("%2d", WiFi.channel(i));
                webServer_bufferContentAddInt(WiFi.channel(i));
                Serial.print(" | ");
                webServer_bufferContentAddChar("</td><td>");
                switch (WiFi.encryptionType(i))
                {
                case WIFI_AUTH_OPEN:
                    Serial.print("open");
                    webServer_bufferContentAddChar("open");
                    break;
                case WIFI_AUTH_WEP:
                    Serial.print("WEP");
                    webServer_bufferContentAddChar("WEP");
                    break;
                case WIFI_AUTH_WPA_PSK:
                    Serial.print("WPA");
                    webServer_bufferContentAddChar("WPA");
                    break;
                case WIFI_AUTH_WPA2_PSK:
                    Serial.print("WPA2");
                    webServer_bufferContentAddChar("WPA2");
                    break;
                case WIFI_AUTH_WPA_WPA2_PSK:
                    Serial.print("WPA+WPA2");
                    webServer_bufferContentAddChar("WPA+WPA2");
                    break;
                case WIFI_AUTH_WPA2_ENTERPRISE:
                    Serial.print("WPA2-EAP");
                    webServer_bufferContentAddChar("WPA2-EAP");
                    break;
                case WIFI_AUTH_WPA3_PSK:
                    Serial.print("WPA3");
                    webServer_bufferContentAddChar("WPA3");
                    break;
                case WIFI_AUTH_WPA2_WPA3_PSK:
                    Serial.print("WPA2+WPA3");
                    webServer_bufferContentAddChar("WPA2+WPA3");
                    break;
                case WIFI_AUTH_WAPI_PSK:
                    Serial.print("WAPI");
                    webServer_bufferContentAddChar("WAPI");
                    break;
                default:
                    Serial.print("unknown");
                    webServer_bufferContentAddChar("unknown");
                }
                Serial.println();
                webServer_bufferContentAddChar("</td></tr>\n");
                delay(10);
            }
        }
        Serial.println("");
        webServer_bufferContentAddChar("</table>\n");

        // Delete the scan result to free memory for code below.
        WiFi.scanDelete();


        webServer_bufferContentAddChar("<br><br><a href='javascript:window.close();'>close</a>\n");

        webServer_bufferContentAddChar("</body></html>\n");

        webServer_bufferContentFlush();



        webServer.sendContent(F("")); // this tells web client that transfer is done
        webServer.client().stop();
    });


    webServer.on("/IMG", HTTP_GET, []() {

        if (webServer.hasArg("DOCUMENT")) {
            webServer.send_P(200, "image/png", (const char*)document_png, document_png_len);
            webServer.client().stop();
        } else if (webServer.hasArg("FOLDER")) {
            webServer.send_P(200, "image/png", (const char*)folder_png, folder_png_len);
            webServer.client().stop();
        }
    });


    webServer.begin();

    //TODO autoplay
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
                APactive = true;
                preferences.putBool("APACTIVE", true);

                lcd.setCursor(0, 0);
                lcd.print("AP: ");
                lcd.print(APssid);
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
                APactive = false;
                preferences.putBool("APACTIVE", false);

                lcd.setCursor(0, 0);
                lcd.print("IP:");
                lcd.print(WiFi.localIP());
            }
        }
    } else {
        Serial.println("WiFi shield not found!");
    }

    if (AudioPlayerCreateDescription()) { // if description changed
        lcd.setCursor(0, 1);
        lcd.print(AudioCurrentlyPlayingDescription);
        lcd.setCursor(0, 0); // TODO check if AP or IP
        if (APactive) {
            lcd.print("AP: ");
            lcd.print(APssid);
        } else {
            lcd.print("IP:");
            lcd.print(WiFi.localIP());
        }
        Serial.println("LCD display");
    }

    if (SDcardNextTrackPath[0] != '\0') { // play next track
        AudioPlayerSDcardPlayTrack(NULL);
    }

    // TODO DISPLAY on CHANGE function -> two vars -> display line1 and line2
    // IP: + IP long -> remove IP:
    // line2 long text -> move text to left
    // replace lcd.print -> lcd_line1 strcpy()



    static unsigned long Timer1000 = 0;
    if ((millis() > (Timer1000 + 1000) || Timer1000 == 0)) {

        // Send
        static byte x = 0;
        Wire.beginTransmission(BP_ESP_SLAVE_ID); // transmit to device #8
        Wire.write("x is ");        // sends five bytes
        Wire.write(x++);              // sends one byte
        Wire.endTransmission();    // stop transmitting

        // Receive
        Wire.requestFrom(BP_ESP_SLAVE_ID, 19);    // request 9 bytes from peripheral device #8
        Serial.print((char)Wire.read());
        Serial.print((char)Wire.read());
        Serial.print((char)Wire.read());
        Serial.print((int)Wire.read()); // "OFF", "ON"
        Serial.print(",");
        Serial.print((int)Wire.read()); // "Disconnected", "Connected"
        Serial.print(",");
        Serial.print((int)Wire.read()); // "Stopped", "Started"
        Serial.print(",");
        Serial.print((int)Wire.read()); // volume (range 0 - 255) (0 - 127)
        Serial.print((char)Wire.read());
        Serial.print((char)Wire.read());
        Serial.print((char)Wire.read());
        Serial.print((int)Wire.read());
        Serial.print(",");
        Serial.print((int)(((int)Wire.read())+900));
        Serial.print(",");
        Serial.print((int)Wire.read());
        Serial.print((char)Wire.read());
        Serial.print((char)Wire.read());
        Serial.print((char)Wire.read());
        Serial.print((char)Wire.read());
        Serial.print((int)Wire.read(), HEX); // FM transmitter status
        Serial.print((char)Wire.read());
        Serial.println();

        Timer1000 = millis();
    }

    yield(); //https://github.com/espressif/arduino-esp32/issues/4348#issuecomment-1463206670
}
