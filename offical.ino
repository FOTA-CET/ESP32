#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

//Define host and wifi
const char* host = "esp32";
const char* ssid = "Xiaomi 13";
const char* password = "ahihihihi";

WebServer server(80);

enum class CONNECT_CMD {
  CONNECT = 65
};

bool connectWifi(IPAddress& ip) {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED);
  ip = WiFi.localIP();
  return true;
}

bool configServer(uint32_t& fileSize) {
  /*Get size of file*/
  server.on("/fileSize", HTTP_POST, [&fileSize]() {
    if (server.hasArg("fileSize")) {
      fileSize = server.arg("fileSize").toInt();
      Serial.println(fileSize);
      server.send(200);
      return true;
    } else {
      server.send(400);
      return false;
    }
  });

  /*Handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {}, [&fileSize]() { 
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
        return false;
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      size_t writeSize = Update.write(upload.buf, upload.currentSize);
      if (writeSize == upload.currentSize) {
        double percent = ((double)upload.totalSize / (double)fileSize) * 100;
        Serial.printf("Writing: %.2f", percent);
        Serial.println("%");
        return false;
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        Serial.println("100%");
        server.send(200);
      } else {
        Update.printError(Serial);
        return false;
      }
    }
  });
  server.begin();
  return true;
}

void setup() {
  Serial.begin(115200);
}

void loop() {
  static uint32_t fileSize;
  if (Serial.available() > 0) {
    int c = Serial.read();
    if (c == (int)CONNECT_CMD::CONNECT) {
      IPAddress ip;
      if (connectWifi(ip)) {
        Serial.println(ip);
      }
      if(configServer(fileSize)) {
        Serial.println("Successfully to config server");
      } else {
        Serial.println("Failed to config server");
      }
    }
  }
  server.handleClient();
}
