#include "WebServerCommon.h"

void WebServerCommon::webServer_bufferContentFlush()
{
    webServer_bufferContentAddChar("");
}

void WebServerCommon::webServer_bufferContentAddChar(const char value[])
{
    int value_length = strlen(value);
    if (WebserverSendBufferLength + value_length >= WEBSERVER_SEND_BUFFER_SIZE || value[0] == '\0') {
        this->sendContent(WebserverSendBuffer);
        WebserverSendBuffer[0] = '\0';
        WebserverSendBufferLength = 0;
    }
    WebserverSendBufferLength += value_length;
    strcat(WebserverSendBuffer, value);
    WebserverSendBuffer[WebserverSendBufferLength] = '\0';
}

void WebServerCommon::webServer_bufferContentAddInt(int value)
{
    char intvalue[32] = {0};
    snprintf(intvalue, 32, "%d", value);
    webServer_bufferContentAddChar(intvalue);
}

void WebServerCommon::webServer_bufferContentAddJavascriptSetElementChecked(const char elementId[])
{
    webServer_bufferContentAddChar("document.getElementById('");
    webServer_bufferContentAddChar(elementId);
    webServer_bufferContentAddChar("').checked = true;\n");
}

void WebServerCommon::webServer_bufferContentAddJavascriptSetElementValue(const char elementId[], char value[])
{
    webServer_bufferContentAddChar("document.getElementById('");
    webServer_bufferContentAddChar(elementId);
    webServer_bufferContentAddChar("').value = \"");
    webServer_bufferContentAddChar(value);
    webServer_bufferContentAddChar("\";\n");
}



String WebServerCommon::webServer_getArgValue(String argname)
{
    for (int i=0; i < this->args(); i++) {
        if (this->argName(i) == argname) {
            return this->arg(i);
        }
    }
    return "";
}

String WebServerCommon::webServer_argsToStr()
{
    String message = "HTTP_REQUEST\n\n";
    message += "URI: ";
    message += this->uri();
    message += "\nMethod: ";
    message += (this->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += this->args();
    message += "\n";
    
    for (uint8_t i = 0; i < this->args(); i++) {
        message += "- " + this->argName(i) + ": " + this->arg(i) + "\n";
    }
    
    return message;
}

void WebServerCommon::webServer_handleFileUpload()
{
    HTTPUpload& upload = this->upload();

    if (upload.status == UPLOAD_FILE_START) {
        Serial.print("UPLOAD_FILE_START\nFIle name: ");
        Serial.println(upload.name);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        Serial.print("UPLOAD_FILE_WRITE\nTotal size: ");
        Serial.println(upload.totalSize);
        Serial.print("Curr size: ");
        Serial.println(upload.currentSize);
        Serial.print("Curr buffer: ");
        upload.buf[upload.currentSize]=0;
        Serial.println((char*)upload.buf);
    } else if (upload.status == UPLOAD_FILE_END) {
        Serial.print("UPLOAD_FILE_END\nTotal size: ");
        Serial.println(upload.totalSize);
        Serial.print("Curr size: ");
        Serial.println(upload.currentSize);
        upload.buf[upload.totalSize]=0;
        Serial.println((char*)upload.buf);
        
        this->send(200, "text/plain", (char*)upload.buf);
        this->client().stop();
    }
}

bool WebServerCommon::webServer_isIP(String str) {
    for (int i = 0; i < str.length(); i++) {
        int c = str.charAt(i);
        if (!(c == '.' || (c >= '0' && c <= '9'))) {
            return false;
        }
    }
    return true;
}
