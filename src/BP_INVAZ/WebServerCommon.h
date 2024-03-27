#ifndef _WebServerCommon_H_
#define _WebServerCommon_H_

#include <Arduino.h>
#include <WebServer.h>


#define WEBSERVER_SEND_BUFFER_SIZE 512

class WebServerCommon : public WebServer {
    
    private:
        char WebserverSendBuffer[WEBSERVER_SEND_BUFFER_SIZE] = {0};
        int WebserverSendBufferLength = 0;
    
    public:
        WebServerCommon(IPAddress addr, int port = 80) : WebServer(addr, port) {}
        WebServerCommon(int port = 80) : WebServer(port) {}
        ~WebServerCommon() {}
        
        void webServer_bufferContentFlush();
        void webServer_bufferContentAddChar(const char value[]);
        void webServer_bufferContentAddInt(int value);
        void webServer_bufferContentAddFloat(float value);
        void webServer_bufferContentAddJavascriptSetElementChecked(const char elementId[]);
        void webServer_bufferContentAddJavascriptSetElementValue(const char elementId[], char value[]);
        void webServer_bufferContentAddJavascriptSetElementInnerHTML(const char elementId[], char value[]);
        
        String webServer_getArgValue(String argname);
        String webServer_argsToStr();
        void webServer_handleFileUpload();
        bool webServer_isIP(String str);

};


#endif
