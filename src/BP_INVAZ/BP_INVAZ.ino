#include <WiFi.h> // https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/wifi.html
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h> // https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/

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





/* AUDIO */
char AudioCurrentlyPlayingDescription[256] = {0};
char AudioLastInternetURL[256] = {0};
bool AudioAutoPlay = false;
char AudioVolume = 10;





/* AUDIO TASK - audio core */
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





void setup() {
    /* SETUP USB SERIAL */
    Serial.begin(9600);
    while(!Serial);
    Serial.println("START");
    Serial.print("setup() is running on core ");
    Serial.println(xPortGetCoreID());
    Serial.println("--------------------");


    /* PREFERENCES */ //TODO reset button - add API button to settings -> reset to default settings
    /* // completely remove non-volatile storage (nvs)
     * #include <nvs_flash.h>
    nvs_flash_erase(); // erase the NVS partition and...
    nvs_flash_init(); // initialize the NVS partition.
    */
    preferences.begin(AppName, false);// Note: Namespace name is limited to 15 chars.
    //preferences.clear();// Remove all preferences under the opened namespace
    // WIFI
    preferences.getBytes("WIFISSID", WIFIssid, 64);
    preferences.getBytes("WIFIPASSWORD", WIFIpassword, 64);
    preferences.getBytes("APSSID", APssid, 64);
    preferences.getBytes("APPASSWORD", APpassword, 64);
    APactive = preferences.getBool("APACTIVE", APactive);
    // Audio
    preferences.getBytes("AU_DESCRIPTION", AudioCurrentlyPlayingDescription, 256);
    preferences.getBytes("AU_LAST_URL", AudioLastInternetURL, 256);
    AudioAutoPlay = preferences.getBool("AU_AUTOPLAY", AudioAutoPlay);
    AudioVolume = preferences.getChar("AU_VOLUME", AudioVolume);
    preferences.end();





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
        // WIFI
        webServer.webServer_bufferContentAddJavascriptSetElementValue("WIFI_SSID", WIFIssid);
        webServer.webServer_bufferContentAddJavascriptSetElementValue("WIFI_PASSWORD", WIFIpassword);
        webServer.webServer_bufferContentAddJavascriptSetElementValue("AP_SSID", APssid);
        webServer.webServer_bufferContentAddJavascriptSetElementValue("AP_PASSWORD", APpassword);
        if (APactive) {webServer.webServer_bufferContentAddJavascriptSetElementChecked("AP_ACTIVE");}
        // Audio
        if (AudioCurrentlyPlayingDescription[0] != '\0') {webServer.webServer_bufferContentAddJavascriptSetElementValue("Mplayer", AudioCurrentlyPlayingDescription);}
        if (AudioLastInternetURL[0] != '\0') {webServer.webServer_bufferContentAddJavascriptSetElementValue("INT_URL", AudioLastInternetURL);}
        if (AudioAutoPlay) {webServer.webServer_bufferContentAddJavascriptSetElementChecked("MP_AUTO");}
        char str[5]; sprintf(str, "%d", (int)AudioVolume); webServer.webServer_bufferContentAddJavascriptSetElementValue("MP_VOLUME", str);

        webServer.webServer_bufferContentAddChar("</script>\n");

        webServer.webServer_bufferContentFlush();


        webServer.sendContent(F("")); // this tells web client that transfer is done
        webServer.client().stop();

        Serial.println("\nHome Page Loaded!\n");
    });


    webServer.on("/API/", HTTP_GET, []() {
        webServer.webServer_printArgs();

        preferences.begin(AppName, false);

        if (webServer.hasArg("CMD")) {
            String cmd = webServer.webServer_getArgValue("CMD");
            char temp[64] = {0};
            if (cmd == "WIFI") {
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
            } else if (cmd == "RESTART") {
                Serial.println("RESTART!!!");
                preferences.end();
                webServer.send(200, "text/plain", "RESTART OK!");
                webServer.client().stop();
                delay(1500);
                ESP.restart();
            } else if (cmd == "DESC") { // Currently playing description
                if (AudioArtist[0] != '\0' && AudioTitle[0] != '\0') {
                    strcpy(AudioCurrentlyPlayingDescription, AudioArtist);
                    strcat(AudioCurrentlyPlayingDescription, " - ");
                    strcat(AudioCurrentlyPlayingDescription, AudioTitle);
                    strcpy(AudioArtist, "");
                    strcpy(AudioTitle, "");
                }
        
                // save to preferences
                //preferences.begin(AppName, false);
                preferences.putBytes("AU_DESCRIPTION", AudioCurrentlyPlayingDescription, 256);
                //preferences.end();



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

    //TODO autoplay
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
            if (WIFIstatus == WL_CONNECTED) {// TODO save last IP and show it in settings
                WIFIlastConnectTryNumber = 0;
                Serial.println("WL_CONNECTED");
                Serial.println("--------------------");
                Serial.print("Connected to: ");
                Serial.println(WiFi.SSID());
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
                Serial.println("--------------------");
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


    yield(); //https://github.com/espressif/arduino-esp32/issues/4348#issuecomment-1463206670
}
