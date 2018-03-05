/*
*  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
*
*  You need to get streamId and privateKey at data.sparkfun.com and paste them
*  below. Or just customize this script to talk to other HTTP servers.
*
*  https://jenkins.mono-project.com/job/test-linker-mainline/lastBuild/api/json
*/

#include <ESP8266WiFi.h>

const char* ssid     = "";
const char* password = "";

//const char* host = "ci.apache.org";
//const char* url = "/json/builders/activemq-site-production";
const char* host = "jenkins.mono-project.com";
// const char* url = "/job/test-linker-mainline/lastBuild/api/json";
const char* url = "/job/test-mono-mainline-codecoverage/lastBuild/api/json";

// Jenkins status constants
const String STATUS_TEXT_SUCCESS = "SUCCESS";
const String STATUS_TEXT_UNSTABLE = "UNSTABLE";
const String STATUS_TEXT_FAILED = "FAILURE";
const byte STATUS_SUCCESS = 1;
const byte STATUS_UNSTABLE = 2;
const byte STATUS_FAILED = 3;

// extract the status of the job and return one of the status constants
byte extractStatus(String json) {
  // remove all spaces
  json.replace(" ", "");

  // search for "result":"..."
  int startPos = json.indexOf("\"result\":\"");
  if (startPos < 0) {
    Serial.println("startPos of result not found");
    return -1; // not found
  }
  startPos = startPos + 10; // jump to value of result

  // evaluate the status
  int endPos = json.indexOf("\"", startPos);
  if (endPos < 0) {
    Serial.println("endPos of result not found");
    return -1; // not found
  }

  String statusText = json.substring(startPos, endPos);

  // convert to byte status constants
  if (STATUS_TEXT_SUCCESS.equals(statusText)) {
    return STATUS_SUCCESS;
  } else if (STATUS_TEXT_UNSTABLE.equals(statusText)) {
    return STATUS_UNSTABLE;
  } else if (STATUS_TEXT_FAILED.equals(statusText)) {
    return STATUS_FAILED;
  } else {
    Serial.println("Invalid status text");
    return -1;
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

int value = 0;

void loop() {
  delay(5000);
  ++value;

  Serial.print("connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClientSecure client;
  const int httpPort = 443;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }


  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
              "Accept: text/html, application/xhtml+xml, */*" + "\r\n"
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  if (client.available()) {
    String json = client.readString();
    byte status = extractStatus(json);
    Serial.println("Returned status: " + String(status));
  }

  Serial.println();
  Serial.println("closing connection");
}
