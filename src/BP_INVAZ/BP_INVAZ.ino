// HistoR - Embedded system for receiving audio streams on a historic radio receiver
// PINS: 32 and 33 -> capacitor meter; 25 and 26 (+ GND) -> audio output
// Author: F3lda (Karel Jirgl) 2024

#include <WiFi.h> // https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/wifi.html
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h> // https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
#include <nvs_flash.h>

#include "HistoRWebPages.h"
#include "AudioTask.h"
#include "WebServerCommon.h"



/* PREFERENCES */
const char AppName[] = "HistoRinvaz";
Preferences preferences;





/* WIFI */
IPAddress         apIP(10, 10, 10, 1);      // Private network for server
IPAddress         netMsk(255, 255, 255, 0); // Net Mask
WebServerCommon   webServer(80);            // HTTP server
const byte        DNS_PORT = 53;            // Capture DNS requests on port 53
DNSServer         dnsServer;                // Create the DNS object

char WIFIssid[64] = "";
char WIFIpassword[64] = "";
char APssid[64] = "HistoRinvaz";
char APpassword[64] = "12345678";
bool APactive = true;

unsigned long WIFIlastConnectTryTimestamp = 0;
unsigned long WIFIlastConnectTryNumber = 0;
unsigned int WIFIstatus = WL_IDLE_STATUS;
char WIFIlastConnectedIP[64] = "(none)";





/* AUDIO */
char AudioLastInternetURL[256] = {0};
char AudioCurrentlyPlayingDescription[256] = {0};
char AudioFrequencySpan = 50;
char AudioVolume = 10;





/* AUDIO TASK - running on audio core */
char AudioArtist[128] = {0};
char AudioTitle[128] = {0};
void audio_id3data(const char *info){  //id3 metadata
    Serial.print("id3data     ");Serial.println(info); // audio file Artist: ; Title: ;
    char *data = strchr(info, ' ');
    if (data != NULL) {
        data++;
        if (info[0] == 'A' && info[1] == 'r' && info[2] == 't' && data-info == 8) {
            if (data[0] == '\0') {
                strcpy(AudioArtist, " ");
            } else {
                strncpy(AudioArtist, data, 127); Serial.print("Artist=");Serial.println(data);
            }
        }
        if (info[0] == 'T' && info[1] == 'i' && info[2] == 't' && data-info == 7) {
            if (data[0] == '\0') {
                strcpy(AudioTitle, " ");
            } else {
                strncpy(AudioTitle, data, 127); Serial.print("Title=");Serial.println(data);
            }
        }
    }
}
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);// radio station
    if (info[0] == '\0') {
        strcpy(AudioArtist, " ");
    } else {
        strncpy(AudioArtist, info, 127);
    }
}
void audio_showstreamtitle(const char *info){
    Serial.print("streamtitle ");Serial.println(info);// radio info
    if (info[0] == '\0') {
        strcpy(AudioTitle, " ");
    } else {
        strncpy(AudioTitle, info, 127);
    }
}
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
    Serial.print("eof_mp3() is running on core ");
    Serial.println(xPortGetCoreID());
}
void audioStartStop(bool audioisrunning){
    Serial.print("audioisrunning    ");Serial.println(audioisrunning);
    Serial.print("audioStartStop() is running on core ");
    Serial.println(xPortGetCoreID());
}
/* AUDIO TASK - end */





/* CAPACITY METER */
// Source: https://wordpress.codewrite.co.uk/pic/2014/01/25/capacitance-meter-mk-ii/
const int OUT_PIN = 32;
const int IN_PIN = 33;
// Capacitance between IN_PIN and Ground
// Stray capacitance value will vary from board to board.
// Calibrate this value using known capacitor.
const float IN_STRAY_CAP_TO_GND = 25.00;
const float IN_CAP_TO_GND = IN_STRAY_CAP_TO_GND;
const int MAX_ADC_VALUE = 4095;
// functions
void capMeterInit();
float capMeterGetValue();
// value
unsigned long CAPlastTimestamp = 0.0;
float CAPlastValue = 0.0;
bool CAPinSpan = false;





void setup() {
    /* SETUP USB SERIAL */
    Serial.begin(9600);
    while(!Serial);
    Serial.println("START");
    Serial.print("setup() is running on core ");
    Serial.println(xPortGetCoreID());
    Serial.println("--------------------");



    /* PREFERENCES */
    preferences.begin(AppName, false);// Note: Namespace name is limited to 15 chars.
    //preferences.clear();// Remove all preferences under the opened namespace
    // WIFI
    preferences.getBytes("WIFISSID", WIFIssid, 64);
    preferences.getBytes("WIFIPASSWORD", WIFIpassword, 64);
    preferences.getBytes("APSSID", APssid, 64);
    preferences.getBytes("APPASSWORD", APpassword, 64);
    APactive = preferences.getBool("APACTIVE", APactive);
    // Audio
    preferences.getBytes("Pdesc", AudioCurrentlyPlayingDescription, 256);
    preferences.getBytes("Purl", AudioLastInternetURL, 256);
    AudioFrequencySpan = preferences.getChar("Pfspan", AudioFrequencySpan);
    AudioVolume = preferences.getChar("Pvolume", AudioVolume);
    preferences.end();





    /* CAPACITY METER */
    capMeterInit();





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

        String message = webServer.webServer_argsToStr();
        Serial.println(message);

        // redirect host
        if(!webServer.webServer_isIP(webServer.hostHeader()) && webServer.hostHeader() != "histor.local") {
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
        webServer.webServer_bufferContentAddChar("<script>\n");


        // Audio
        if (AudioCurrentlyPlayingDescription[0] != '\0') {webServer.webServer_bufferContentAddJavascriptSetElementInnerHTML("Pdesc", AudioCurrentlyPlayingDescription);}
        if (AudioCurrentlyPlayingDescription[0] == '\0' && AudioLastInternetURL[0] != '\0') {webServer.webServer_bufferContentAddJavascriptSetElementInnerHTML("Pdesc", AudioLastInternetURL);}
        char str[5]; sprintf(str, "%d", (int)AudioFrequencySpan); webServer.webServer_bufferContentAddJavascriptSetElementValue("Pfspan", str);
        sprintf(str, "%d", (int)AudioVolume); webServer.webServer_bufferContentAddJavascriptSetElementValue("Pvolume", str);


        // Streams
        preferences.begin(AppName, false);
        int streamsCount = preferences.getInt("streamsCount", 0);
        Serial.print("streamsCount: ");
        Serial.println(streamsCount);
        int streamsFreq[streamsCount+1] = {0};
        char streamsUrl[streamsCount+1][256] = {0};
        if (streamsCount > 0) { // https://forum.arduino.cc/t/storing-an-int-array-using-preferences-esp32/1216037/2
            preferences.getBytes("streamsFreq", &streamsFreq, sizeof(streamsFreq));
            preferences.getBytes("streamsUrl", streamsUrl, sizeof(streamsUrl));
        }
        preferences.end();
        
        for(int i = streamsCount-1; i >= 0; i--) {
            webServer.webServer_bufferContentAddChar("addStream(");
            webServer.webServer_bufferContentAddInt(streamsFreq[i]);
            webServer.webServer_bufferContentAddChar(", '");
            webServer.webServer_bufferContentAddChar(streamsUrl[i]);
            webServer.webServer_bufferContentAddChar("');\n");
        }
        if (streamsCount == 0) {
            webServer.webServer_bufferContentAddChar("addStream(150,'https://ice5.abradio.cz/hitvysocina128.mp3');\n");
        }

        
        // WIFI
        webServer.webServer_bufferContentAddJavascriptSetElementInnerHTML("WIFI_IP", WIFIlastConnectedIP);
        webServer.webServer_bufferContentAddJavascriptSetElementValue("WIFI_SSID", WIFIssid);
        webServer.webServer_bufferContentAddJavascriptSetElementValue("WIFI_PASSWORD", WIFIpassword);
        webServer.webServer_bufferContentAddJavascriptSetElementValue("AP_SSID", APssid);
        webServer.webServer_bufferContentAddJavascriptSetElementValue("AP_PASSWORD", APpassword);
        if (APactive) {webServer.webServer_bufferContentAddJavascriptSetElementChecked("AP_ACTIVE");}


        webServer.webServer_bufferContentAddChar("</script>\n");
        webServer.webServer_bufferContentFlush();

        webServer.sendContent(F("")); // this tells web client that transfer is done
        webServer.client().stop();
    });


    webServer.on("/API", HTTP_POST, []() {
        webServer.webServer_argsToStr();

        preferences.begin(AppName, false);

        if (webServer.hasArg("CMD")) {
            String cmd = webServer.webServer_getArgValue("CMD");
            char temp[64] = {0};
            if (cmd == "PLAYER") {
                if (webServer.hasArg("Pfspan")) {
                    AudioFrequencySpan = webServer.webServer_getArgValue("Pfspan").toInt();
                    preferences.putChar("Pfspan", AudioFrequencySpan);
                }
                if (webServer.hasArg("Pvolume")) {
                    AudioVolume = webServer.webServer_getArgValue("Pvolume").toInt();
                    preferences.putChar("Pvolume", AudioVolume);
                    // change volume
                    audioSetVolume(AudioVolume); // 0...21
                }
            } else if (cmd == "STREAMS") {
                int streamsCount = 0;
                int streamsFreq[(webServer.args()-1)/2] = {0};
                char streamsUrl[(webServer.args()-1)/2][256] = {0};
                for(int i = 1; i < webServer.args(); i++) {
                    if (webServer.argName(i) == "Sfreq[]") {
                        // FREQ
                        if ((webServer.arg(i) == "" || webServer.arg(i).toInt() == 0) && webServer.arg(i+1) == "") { // if freq is empty -> skip Freq and next URL arg
                            i++;
                        } else {
                            streamsFreq[streamsCount] = webServer.arg(i).toInt();
                            streamsCount++;
                        }
                    } else if (webServer.argName(i) == "Surl[]") {
                        // URL
                        webServer.arg(i).toCharArray(streamsUrl[streamsCount-1], 255);
                    }
                }
                preferences.putInt("streamsCount", streamsCount);
                preferences.putBytes("streamsFreq", (byte*)(&streamsFreq), (streamsCount*sizeof(streamsFreq[0])));
                preferences.putBytes("streamsUrl", (byte*)(streamsUrl), (streamsCount*sizeof(streamsUrl[0])));
            } else if (cmd == "WIFI") {
                if (webServer.hasArg("SSID")) {
                    webServer.webServer_getArgValue("SSID").toCharArray(temp, 63);
                    preferences.putBytes("WIFISSID", temp, 64);
                    strcpy(WIFIssid, temp);
                }
                if (webServer.hasArg("PASSWORD")) {
                    webServer.webServer_getArgValue("PASSWORD").toCharArray(temp, 63);
                    preferences.putBytes("WIFIPASSWORD", temp, 64);
                    strcpy(WIFIpassword, temp);
                }

                preferences.end();
                webServer.send(200, "text/plain", "WIFI settings saved!");
                webServer.client().stop();
                return;
            } else if (cmd == "AP") {
                if (webServer.hasArg("ACTIVE")) {
                    cmd = webServer.webServer_getArgValue("ACTIVE");
                    if (cmd == "true") {
                        preferences.putBool("APACTIVE", true);
                        APactive = true;
                    } else if (cmd == "false") {
                        preferences.putBool("APACTIVE", false);
                        APactive = false;
                    }
                }
                if (webServer.hasArg("SSID")) {
                    webServer.webServer_getArgValue("SSID").toCharArray(temp, 63);
                    preferences.putBytes("APSSID", temp, 64);
                    strcpy(APssid, temp);
                }
                if (webServer.hasArg("PASSWORD")) {
                    webServer.webServer_getArgValue("PASSWORD").toCharArray(temp, 63);
                    preferences.putBytes("APPASSWORD", temp, 64);
                    strcpy(APpassword, temp);
                }

                preferences.end();
                webServer.send(200, "text/plain", "AP settings saved!");
                webServer.client().stop();
                return;
            } else if (cmd == "RESTART") {
                Serial.println("RESTART!!!");
                
                preferences.end();
                webServer.send(200, "text/plain", "RESTART OK!");
                webServer.client().stop();
                delay(1500);
                ESP.restart();
                return;
            } else if (cmd == "ERASE") {
                Serial.println("ERASE!!!");

                // completely remove non-volatile storage (nvs)
                nvs_flash_erase(); // erase the NVS partition and...
                nvs_flash_init(); // initialize the NVS partition.

                preferences.end();
                webServer.send(200, "text/plain", "ERASE OK!");
                webServer.client().stop();
                delay(1500);
                ESP.restart();
                return;
            } else if (cmd == "DESC") { 
                // currently playing description
                if (AudioArtist[0] != '\0' && AudioTitle[0] != '\0') {
                    snprintf(AudioCurrentlyPlayingDescription, 256, "%s - %s", AudioArtist, AudioTitle) < 0 ? printf("%c", '\0') : 0; // ignore truncation warning
                    // save to preferences
                    preferences.putBytes("Pdesc", AudioCurrentlyPlayingDescription, 256);
                    strcpy(AudioArtist, "");
                    strcpy(AudioTitle, "");
                }


                // SEND VALUES - description and current frequency
                webServer.setContentLength(CONTENT_LENGTH_UNKNOWN); // https://www.esp8266.com/viewtopic.php?p=73204
                // here begin chunked transfer
                webServer.send(200, "application/json", "");


                webServer.webServer_bufferContentAddChar("{\"desc\":\"");
                if (AudioCurrentlyPlayingDescription[0] != '\0') {
                    webServer.webServer_bufferContentAddChar(AudioCurrentlyPlayingDescription);
                } else {
                    webServer.webServer_bufferContentAddChar(AudioLastInternetURL);
                }
                webServer.webServer_bufferContentAddChar("\", \"freq\":\"");
                webServer.webServer_bufferContentAddFloat(CAPlastValue);
                webServer.webServer_bufferContentAddChar("\", \"inSpan\":\"");
                if (CAPinSpan) {
                    webServer.webServer_bufferContentAddChar("IN SPAN");
                }
                webServer.webServer_bufferContentAddChar("\"}");


                preferences.end();
                webServer.webServer_bufferContentFlush();
                webServer.sendContent(F("")); // this tells web client that transfer is done
                webServer.client().stop();
                return;
            }
        }

        preferences.end();

        webServer.send(200, "text/plain", "");       //Response to the HTTP request
        webServer.client().stop();
    });


    webServer.on("/WIFISCAN", HTTP_GET, []() {

        webServer.setContentLength(CONTENT_LENGTH_UNKNOWN); // https://www.esp8266.com/viewtopic.php?p=73204
        // here begin chunked transfer
        webServer.send(200, "text/html", "");

        webServer.webServer_bufferContentAddChar("<!DOCTYPE html><html><head><title>HistoR - WIFI SCANNER</title></head><body>");

        Serial.println("Scan start");
        webServer.webServer_bufferContentAddChar("Scan start\n");

        // WiFi.scanNetworks will return the number of networks found.
        int n = WiFi.scanNetworks();
        Serial.println("Scan done");
        webServer.webServer_bufferContentAddChar("Scan done<br>\n");
        if (n == 0) {
            Serial.println("no networks found");
            webServer.webServer_bufferContentAddChar("no networks found\n");
        } else {
            Serial.print(n);
            webServer.webServer_bufferContentAddInt(n);
            Serial.println(" networks found");
            webServer.webServer_bufferContentAddChar(" networks found\n");
            Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
            webServer.webServer_bufferContentAddChar("<table><tr><th>Nr</th><th>SSID</th><th>RSSI</th><th>CH</th><th>Encryption</th></tr>\n");
            for (int i = 0; i < n; ++i) {
                webServer.webServer_bufferContentAddChar("<tr><td>");
                // Print SSID and RSSI for each network found
                Serial.printf("%2d",i + 1);
                webServer.webServer_bufferContentAddInt(i + 1);
                Serial.print(" | ");
                webServer.webServer_bufferContentAddChar("</td><td>");
                Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
                webServer.webServer_bufferContentAddChar(WiFi.SSID(i).c_str());
                Serial.print(" | ");
                webServer.webServer_bufferContentAddChar("</td><td>");
                Serial.printf("%4d", WiFi.RSSI(i));
                webServer.webServer_bufferContentAddInt(WiFi.RSSI(i));
                Serial.print(" | ");
                webServer.webServer_bufferContentAddChar("</td><td>");
                Serial.printf("%2d", WiFi.channel(i));
                webServer.webServer_bufferContentAddInt(WiFi.channel(i));
                Serial.print(" | ");
                webServer.webServer_bufferContentAddChar("</td><td>");
                switch (WiFi.encryptionType(i))
                {
                case WIFI_AUTH_OPEN:
                    Serial.print("open");
                    webServer.webServer_bufferContentAddChar("open");
                    break;
                case WIFI_AUTH_WEP:
                    Serial.print("WEP");
                    webServer.webServer_bufferContentAddChar("WEP");
                    break;
                case WIFI_AUTH_WPA_PSK:
                    Serial.print("WPA");
                    webServer.webServer_bufferContentAddChar("WPA");
                    break;
                case WIFI_AUTH_WPA2_PSK:
                    Serial.print("WPA2");
                    webServer.webServer_bufferContentAddChar("WPA2");
                    break;
                case WIFI_AUTH_WPA_WPA2_PSK:
                    Serial.print("WPA+WPA2");
                    webServer.webServer_bufferContentAddChar("WPA+WPA2");
                    break;
                case WIFI_AUTH_WPA2_ENTERPRISE:
                    Serial.print("WPA2-EAP");
                    webServer.webServer_bufferContentAddChar("WPA2-EAP");
                    break;
                case WIFI_AUTH_WPA3_PSK:
                    Serial.print("WPA3");
                    webServer.webServer_bufferContentAddChar("WPA3");
                    break;
                case WIFI_AUTH_WPA2_WPA3_PSK:
                    Serial.print("WPA2+WPA3");
                    webServer.webServer_bufferContentAddChar("WPA2+WPA3");
                    break;
                case WIFI_AUTH_WAPI_PSK:
                    Serial.print("WAPI");
                    webServer.webServer_bufferContentAddChar("WAPI");
                    break;
                default:
                    Serial.print("unknown");
                    webServer.webServer_bufferContentAddChar("unknown");
                }
                Serial.println();
                webServer.webServer_bufferContentAddChar("</td></tr>\n");
                delay(10);
            }
        }
        Serial.println("");
        webServer.webServer_bufferContentAddChar("</table>\n");

        // Delete the scan result to free memory for code below.
        WiFi.scanDelete();


        webServer.webServer_bufferContentAddChar("<br><br><a href='javascript:window.close();'>close</a>\n");

        webServer.webServer_bufferContentAddChar("</body></html>\n");

        webServer.webServer_bufferContentFlush();



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
}



void loop() {
    dnsServer.processNextRequest();
    webServer.handleClient();
    
    if (WiFi.status() != WL_NO_SHIELD) {
        if (WiFi.status() != WL_CONNECTED && (millis() > (WIFIlastConnectTryTimestamp + 60000) || WIFIlastConnectTryTimestamp == 0) && WIFIlastConnectTryNumber < 2) {
            Serial.println("Connecting to WIFI...");
            WiFi.disconnect();
            WiFi.begin(WIFIssid, WIFIpassword);
            WIFIlastConnectTryTimestamp = millis();
            WIFIlastConnectTryNumber++;
            if (WIFIlastConnectTryNumber == 2) {
                Serial.println("WIFI connection lost/failed!");
                Serial.println("START AP!");
                if (APssid[0] == '\0' || strlen(APssid) < 8) {
                    strcpy(APssid, AppName);
                }
                Serial.println(APssid);
                Serial.println(APpassword);
                WiFi.softAP(APssid, APpassword);
                APactive = true;
                preferences.begin(AppName, false);
                preferences.putBool("APACTIVE", true);
                preferences.end();
            }
        }
        if(WiFi.status() != WIFIstatus){
            WIFIstatus = WiFi.status();
            Serial.println("WIFI status changed: "+String(WIFIstatus));
            if (WIFIstatus == WL_CONNECTED) {
                WIFIlastConnectTryNumber = 0;
                Serial.println("WL_CONNECTED");
                Serial.println("--------------------");
                Serial.print("Connected to: ");
                Serial.println(WiFi.SSID());
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
                Serial.println("--------------------");
                WiFi.localIP().toString().toCharArray(WIFIlastConnectedIP, 63);
                /* don't disable AP automatically after wifi is connected
                 * APactive = false;
                preferences.begin(AppName, false);
                preferences.putBool("APACTIVE", false);
                preferences.end();*/
            }
        }
    } else {
        Serial.println("WiFi shield not found!");
    }



    // get radio capacitor value (get current frequency)
    if (millis()-CAPlastTimestamp >= 500) {
        CAPlastValue = capMeterGetValue();
        CAPlastTimestamp = millis();

        
        // start radio on current freq -> check frequency span with frequency from preferences and play new stream or stop old one
        // get frequencies
        preferences.begin(AppName, false);
        int streamsCount = preferences.getInt("streamsCount", 0);
        Serial.print("streamsCount: ");
        Serial.println(streamsCount);
        int streamsFreq[streamsCount+1] = {0};
        char streamsUrl[streamsCount+1][256] = {0};
        if (streamsCount > 0) { // https://forum.arduino.cc/t/storing-an-int-array-using-preferences-esp32/1216037/2
            preferences.getBytes("streamsFreq", &streamsFreq, sizeof(streamsFreq));
            preferences.getBytes("streamsUrl", streamsUrl, sizeof(streamsUrl));
        }
        preferences.end();


        CAPinSpan = false;
        static int notInSpanCount = 0;
        for(int i = streamsCount-1; i >= 0; i--) {
            // check if frequency is in span
            if ((streamsFreq[i]-(int)((int)AudioFrequencySpan/2)) < CAPlastValue && CAPlastValue < (streamsFreq[i]+(int)((int)AudioFrequencySpan/2))) {
                Serial.print("IN SPAN: ");
                Serial.println(streamsFreq[i]);

                CAPinSpan = true;

                if (strcmp(AudioLastInternetURL, streamsUrl[i]) != 0) {
                    Serial.print("STATION CHANGED: ");
                    Serial.println(streamsUrl[i]);
                    
                    strcpy(AudioLastInternetURL, streamsUrl[i]);
                    
                    // START URL RADIO on current frequency
                    audioStopSong();
                    audioConnecttohost(AudioLastInternetURL);
                }
            }
        }
        if (CAPinSpan) {
            notInSpanCount = 0;
        } else {
            notInSpanCount++;
        }
        if (notInSpanCount == 10) {
            Serial.println("LONG TIME NOT IN SPAN: stopAudio");
            
            // STOP RADIO
            audioStopSong();
        }
    }

    

    yield(); //https://github.com/espressif/arduino-esp32/issues/4348#issuecomment-1463206670
}


void capMeterInit()
{
    pinMode(OUT_PIN, OUTPUT);
    //digitalWrite(OUT_PIN, LOW);  // This is the default state for outputs
    pinMode(IN_PIN, OUTPUT);
    //digitalWrite(IN_PIN, LOW);
}


float capMeterGetValue()
{
    //Capacitor under test between OUT_PIN and IN_PIN
    //Rising high edge on OUT_PIN
    pinMode(IN_PIN, INPUT);
    digitalWrite(OUT_PIN, HIGH);
    int val = analogRead(IN_PIN);
    digitalWrite(OUT_PIN, LOW);

    if (val < (int)(MAX_ADC_VALUE*0.976)) // 97,6 % = 0.976
    {
        //Low value capacitor
        //Clear everything for next measurement
        pinMode(IN_PIN, OUTPUT);
  
        //Calculate and print result
  
        float capacitance = (float)val * IN_CAP_TO_GND / (float)(MAX_ADC_VALUE - val);
  
  
  
        #define CapMemSize 10
        static float capMem[CapMemSize] = {0.0};
        
        float capAvg = capacitance;
        for(int i = 0; i < CapMemSize-1; i++){
            capAvg += capMem[i];
            capMem[i] = capMem[i+1];
        }
        capMem[CapMemSize-1] = capacitance;
        capAvg /= CapMemSize;
        
  
  
        Serial.print(F("Capacitance Value = "));
        Serial.print(capacitance, 3);
        Serial.print(F(" pF ("));
        Serial.print(val);
        Serial.print(F(") ["));
        Serial.print(capAvg);
        Serial.println(F("] "));

        return capAvg;
    }
    return -1.0;
}
