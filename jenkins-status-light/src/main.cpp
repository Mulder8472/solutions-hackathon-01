/*
*  This sketch reads build job status from configurable jenkins job.
*
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid     = "";
const char* password = "";
const char* jenkinsparameter = "/lastBuild/api/json?tree=result";
const char* HTML_START = "<html><head><title>Jenkins Status Light</title></head><body><h1>Jenkins Status Light</h1>";
const char* HTML_END = "</body></html>";
const char* HTML_FORM_CONFIG = "<form action=\"\" method=\"get\"><table><colgroup><colwidth=\"200\"><colwidth=\"300\"></colgroup><tr><td><label for=\"host\">Host:</label></td><td><input name=\"host\" id=\"host\" value=\"$host\"></td></tr><tr><td><label for=\"jobname\">Jenkins job name:</label></td><td><input name=\"jobname\" id=\"jobname\" value=\"$jobname\"></td></tr><tr><td><label for=\"updateinterval\">Update interval (seconds):</label></td><td><input type=\"number\" name=\"updateinterval\" id=\"updateinterval\" value=\"$updateinterval\"></td></tr></table><div><button>Update</button></div></form>";
const char* HTML_LINK_CONFIGURE = "<a href=\"/configure\">Configuration</a>";
const char* HTML_LINK_MAIN_PAGE = "<a href=\"/\">Main page</a>";
const unsigned long UPDATE_INTERVAL_SEC = 30; // update interval of Jenkins job status in seconds

// current variables for jenkins access
String host = "jenkins.mono-project.com";
String jobname = "test-mono-mainline-codecoverage";
String url; // will be filled by buildUrl() with host, jobname and jenkinsparameter
byte currentJobStatus = 0;
String currentJobStatusText;
unsigned long updateIntervalSec = UPDATE_INTERVAL_SEC;

unsigned long lastUpdateMillis = 0; // millis of the last job update

// Jenkins status constants
const String STATUS_TEXT_SUCCESS = "SUCCESS";
const String STATUS_TEXT_UNSTABLE = "UNSTABLE";
const String STATUS_TEXT_FAILED = "FAILURE";
const String STATUS_TEXT_UNKNWON = "UNKNOWN";
const byte STATUS_UNKNOWN = 0;
const byte STATUS_SUCCESS = 1;
const byte STATUS_UNSTABLE = 2;
const byte STATUS_FAILED = 3;

ESP8266WebServer server(80);   //Web server object. Will be listening in port 80 (default for HTTP)

// extract the status of the job and return one of the status constants
byte extractStatus(String json) {
  // search for "result":"..."
  int startPos = json.indexOf("\"result\":\"");
  if (startPos < 0) {
    Serial.println("startPos of result not found");
    return STATUS_UNKNOWN; // not found
  }
  startPos = startPos + 10; // jump to value of result

  // evaluate the status
  int endPos = json.indexOf("\"", startPos);
  if (endPos < 0) {
    Serial.println("endPos of result not found");
    return STATUS_UNKNOWN; // not found
  }

  String statusText = json.substring(startPos, endPos);

  // convert to byte status constants
  if (STATUS_TEXT_SUCCESS.equals(statusText)) {
    currentJobStatusText = statusText;
    return STATUS_SUCCESS;
  } else if (STATUS_TEXT_UNSTABLE.equals(statusText)) {
    currentJobStatusText = statusText;
    return STATUS_UNSTABLE;
  } else if (STATUS_TEXT_FAILED.equals(statusText)) {
    currentJobStatusText = statusText;
    return STATUS_FAILED;
  } else {
    currentJobStatusText = STATUS_TEXT_UNKNWON;
    Serial.println("Invalid status text");
    return STATUS_UNKNOWN;
  }
}

void buildUrl() {
  url = "/job/" + jobname + jenkinsparameter;
}

void updateJenkinsJobState() {
  Serial.print("connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClientSecure client;
  const int httpPort = 443;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  Serial.println("Requesting URL: " + url);

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
    currentJobStatus = extractStatus(json);
    Serial.println("Returned status: " + String(currentJobStatus));
  }

  Serial.println();
  Serial.println("closing connection");

  lastUpdateMillis = millis();
}

// HTTP webserver handler
void handleConfigureUrl() {
  bool reloadJenkinsStatus = false;
  for (int i = 0; i < server.args(); i++) {
    // handle url parameter
    if (server.argName(i).equals("host")) {
      host = server.arg(i);
      reloadJenkinsStatus = true;
    } else if (server.argName(i).equals("jobname")) {
      jobname = server.arg(i);
      buildUrl();
      reloadJenkinsStatus = true;
    } else if (server.argName(i).equals("updateinterval")) {
      updateIntervalSec = server.arg(i).toInt();
    }
  }

  // build formular page with current settings
  String message = String(HTML_START);
  message += String(HTML_FORM_CONFIG);
  message.replace("$host", host);
  message.replace("$jobname", jobname);
  message.replace("$updateinterval", String(updateIntervalSec));
  message += HTML_LINK_MAIN_PAGE;
  message += HTML_END;
  server.send(200, "text/html", message);       //Response to the HTTP request

  if (reloadJenkinsStatus) {
    updateJenkinsJobState();
  }
}

// HTTP webserver handler
void handleMainUrl() {
  String message= String(HTML_START) + "Host: " + host + "<br>Job name: " + jobname + "<br><br>Status: ";
  String color = "black";
  if (STATUS_TEXT_SUCCESS.equals(currentJobStatusText)) {
    color = "green";
  } else if (STATUS_TEXT_UNSTABLE.equals(currentJobStatusText)) {
    color = "orange";
  } else if (STATUS_TEXT_FAILED.equals(currentJobStatusText)) {
    color = "red";
  } else {
    color = "black";
  }
  message += "<p style=\"color:" + color + "\";>" + currentJobStatusText + "</p>";
  message += HTML_LINK_CONFIGURE;
  message += HTML_END;

  server.send(200, "text/html", message);       //Response to the HTTP request
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

  server.on("/configure", handleConfigureUrl); //Associate the handler function to the pat
  server.on("/", handleMainUrl);
  server.begin();
  Serial.println("Server listening");

  buildUrl();
}

// main loop
void loop() {
  delay(100);

  server.handleClient();    //Handling of incoming requests

  // check if new update from job is needed
  unsigned long currentMillis = millis();
  unsigned long secondsSinceLastUpdate = (currentMillis - lastUpdateMillis) / 1000;

  if (lastUpdateMillis == 0 || secondsSinceLastUpdate > updateIntervalSec) {
    updateJenkinsJobState();
  }
}
