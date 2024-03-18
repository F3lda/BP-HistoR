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
    sprintf(intvalue, "%d", value);
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
        if (this->argName(i) == argname){
            return this->arg(i);
        }
    }
    return "";
}

void WebServerCommon::webServer_printArgs()
{
      String message = "Number of args received:";
      message += this->args();            //Get number of parameters
      message += "\n";                            //Add a new line

      for (int i = 0; i < this->args(); i++) {

          message += "Arg n#" + (String)i + " –> ";   //Include the current iteration value
          message += this->argName(i) + ": ";     //Get the name of the parameter
          message += this->arg(i) + "\n";              //Get the value of the parameter

      }
      Serial.println(message);
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
