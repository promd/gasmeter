// Based on work found at https://blog.gwarg.de/2021/12/22/gaszaehler-an-openhab-per-mqtt/

#include <Arduino.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h> 

#include <ESP8266WebServer.h>

#include <time.h>

// #############################################
// WLAN-Zugang
// #############################################
#define wifi_ssid "promd"
#define wifi_password "0798793680031737"

// #############################################
// MQTT-Server (Openhab)
// #############################################
#define mqtt_server "192.168.88.99"

// #############################################
// NTP-Client
// #############################################
#define MY_NTP_SERVER "de.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"   

time_t now;
//tm tm;
String TimeStamp;

ESP8266WebServer server(80);
String htmltext;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

int Digital_Eingang = D2; // Binärer Wert

int lastDigital  = 0;
int digital_temp = 0;
int daily_count_gas = 0;

void setup_wifi();
void reconnect();
void handleCss();
void handle_OnData();
void handle_OnConnect();
void handle_NotFound();
String SendHTML(String addtext);

WiFiUDP ntpUDP;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionally you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void setup ()
{
  pinMode (Digital_Eingang, INPUT);
       
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  
  timeClient.begin();
  
  server.on("/", handle_OnData);
  server.on("/f.css", handleCss);
  server.onNotFound(handle_NotFound);
  server.begin();
  
  Serial.println("HTTP server started");
}
  
void loop ()
{
  if (!client.connected()) {
    reconnect();
  }

  timeClient.update();
  server.handleClient();

  // Um Null Uhr den Zähler wieder zurück setzen!
  if (timeClient.getHours() == 0 && timeClient.getMinutes() == 0 && timeClient.getSeconds() == 0) {
    Serial.println ("Counter reset!");
    daily_count_gas = 0;
  }

  int Digital;
    
  //Aktuelle Werte werden ausgelesen, auf den Spannungswert konvertiert...
  Digital = digitalRead (Digital_Eingang);
  // Write data if pulse detected
  if (!Digital && Digital != lastDigital) {
    Serial.printf ("Counter increased: %d\n",daily_count_gas);
    daily_count_gas++;
    client.publish("gas/digital", String(daily_count_gas).c_str(), true);
  }
  lastDigital = Digital;

  TimeStamp = timeClient.getFormattedTime();
  
  htmltext = "";
  htmltext += "<h2>Uhrzeit</h2>"; 
  htmltext += "<p><b>" + String(TimeStamp) + "</b></p>";
  htmltext += "<h2>Werte</h2>"; 
  htmltext += "<p>Digital: <b>" + String(Digital) + "</b></p>";
  htmltext += "<h2>Zähler</h2>"; 
  htmltext += "<p>heute: <b>" + String(daily_count_gas * 0.01) + "m³</b></p>";
  
  delay (200);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.print(WiFi.localIP());
  Serial.println("");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "gasmeter";
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
   }
}

void handle_OnData() {
  server.send(200, "text/html", SendHTML(htmltext)); 
}
void handle_OnConnect() {
  server.send(200, "text/html", SendHTML("Boot")); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(String addtext){
  String ptr = "";
  ptr += "<!DOCTYPE html PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN'>\n";
  ptr += "<html lang='de'>\n";
  ptr += "<head>\n";
  ptr += "<title>Gaszaehler Webserver</title>\n";
  ptr += "<link rel='stylesheet' type='text/css' href='/f.css'>\n";
  ptr += "<meta content='text/html; charset=UTF-8' http-equiv='content-type'>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>Gaszaehler Webserver</h1>\n";
  ptr += addtext;
  ptr += "</body>\n";
  ptr += "</html>";
  
  return ptr;
}

void handleCss()
{
  String message = "";
  message += "*{font-family:sans-serif}\n";
  message += "body{margin:10px;width:300px}\n";
  message += "h1, h2{color:white;background:#8FCE00;text-align:center}\n";
  message += "h1{font-size:1.2em;margin:1px;padding:5px}\n";
  message += "h2{font-size:1.0em}\n";
  message += "h3{font-size:0.9em}\n";
  message += "a{text-decoration:none;color:dimgray;text-align:center}\n";
  message += ".small{font-size:0.6em}\n";
  message += ".value{font-size:1.8em;text-align:center;line-height:50%}\n";
  message += "footer p, .infodaten p{font-size:0.7em;color:dimgray;background:silver;";
  message += "text-align:center;margin-bottom:5px}\n";
  message += "nav{background-color:silver;margin:1px;padding:5px;font-size:0.8em}\n";
  message += "nav a{color:white;padding:10px;text-decoration:none}\n";
  message += "nav a:hover{text-decoration:underline}\n";
  message += "nav p{margin:0px;padding:0px}\n";
  message += ".btn{background-color:#C0C0C0;color:dimgray;text-decoration:none;";
  message += "border-style:solid;border-color:dimgray}\n";
  message += ".on, .off{margin-top:0;margin-bottom:0.2em;margin-left:3em;";
  message += "font-size:1.4em;background-color:#C0C0C0;";
  message += "border-style:solid;width:5em;height:1.5em;text-decoration:none;text-align:center}\n";
  message += ".on{border-color:green}\n)";
  server.send(200, "text/css", message);
}
