// Growatt Solar Inverter to MQTT
// Repo: https://github.com/nygma2004/growatt2mqtt
// author: Csongor Varga, csongor.varga@gmail.com
// 1 Phase, 2 string inverter version such as MIN 3000 TL-XE, MIC 1500 TL-X

// Libraries:
// - FastLED by Daniel Garcia
// - ModbusMaster by Doc Walker
// - ArduinoOTA
// - SoftwareSerial
// - IotWebConf
// Hardware:
// - Wemos D1 mini
// - RS485 to TTL converter: https://www.aliexpress.com/item/1005001621798947.html
// - To power from mains: Hi-Link 5V power supply (https://www.aliexpress.com/item/1005001484531375.html), fuseholder and 1A fuse, and varistor

#if defined(ESP8266)
    #define HARDWARE "ESP8266"
    #include <ESP8266WiFi.h>          // Wifi connection
    #include <ESP8266WebServer.h>     // Web server for general HTTP response
    #include <ESP8266HTTPUpdateServer.h>
#elif defined(ESP32)
    #define HARDWARE "ESP32"
    #include "WiFi.h"
    #include <WebServer.h>
    #include <IotWebConfESP32HTTPUpdateServer.h>
#endif

#ifdef ARDUINO_WT32_ETH01
  #include <ETH.h>
#endif

#include <IotWebConf.h>
#include <IotWebConfUsing.h>
#include <IotWebConfOptionalGroup.h>
#include <PubSubClient.h>         // MQTT support
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <FastLED.h>
#include "globals.h"
#include "settings.h"
#include "growattInterface.h"
#include "HttpServerData.h"

#if defined(ESP8266)
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
#elif defined(ESP32)
WebServer server(80);
HTTPUpdateServer httpUpdater;
#endif
WiFiClient espClient;
PubSubClient mqtt(espClient);

CRGB leds[NUM_LEDS];
growattIF growattInterface(MAX485_RE_NEG, MAX485_DE, MAX485_RX, MAX485_TX);

uint32_t timediff(uint32_t t1, uint32_t t2);


// ---- IotWebConf -----
#ifdef ESP8266
String ChipId = String(ESP.getChipId(), HEX);
#elif ESP32
String ChipId = String((uint32_t)ESP.getEfuseMac(), HEX);
#endif

String thingName = "growatt2mqtt"+ ChipId;
const char wifiInitialApPassword[] = "growatt2mqttPassword";

void handleRoot();    // handle web requests on "/"
void configSaved();   // config saved callback
bool connectAp(const char* apName, const char* password);
void connectWifi(const char* ssid, const char* password);
bool formValidator(iotwebconf::WebRequestWrapper* webRequestWrapper);
DNSServer dnsServer;

// IotWebConf Settings:
#define STRING_LEN 128
#define NUMBER_LEN 32

char WifiStaticIpValue[STRING_LEN];
char WifiStaticNetmaskValue[STRING_LEN];
char WifiStaticGatewayValue[STRING_LEN];
IPAddress WifiIpAddress;
IPAddress WifiGateway;
IPAddress WifiNetmask;

char EthStaticIpValue[STRING_LEN];
char EthStaticNetmaskValue[STRING_LEN];
char EthStaticGatewayValue[STRING_LEN];
IPAddress EthIpAddress;
IPAddress EthGateway;
IPAddress EthNetmask;

char mqttServerValue[STRING_LEN];
char mqttUserNameValue[STRING_LEN];
char mqttUserPasswordValue[STRING_LEN];
char mqttTopicRootValue[STRING_LEN];

IotWebConf iotWebConf(thingName.c_str(), &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);


iotwebconf::OptionalParameterGroup StaticWifiGroup = iotwebconf::OptionalParameterGroup("wifiStaticIpgroup", "Static Wifi IP", false);
iotwebconf::TextParameter wifiStaticIpParam = iotwebconf::TextParameter("Static Wifi IP address", "wifiStaticIpid", WifiStaticIpValue, STRING_LEN);
iotwebconf::TextParameter wifiStaticNetmaskParam = iotwebconf::TextParameter("Static Wifi netmask", "wifiStaticNetmaskid", WifiStaticNetmaskValue, STRING_LEN);
iotwebconf::TextParameter wifiStaticGatewayParam = iotwebconf::TextParameter("Static Wifi Gateway", "wifiStaticGatewayid", WifiStaticGatewayValue, STRING_LEN);

iotwebconf::OptionalParameterGroup StaticEthGroup = iotwebconf::OptionalParameterGroup("ethStaticIpgroup", "Static ETH IP", false);
iotwebconf::TextParameter ethStaticIpParam = iotwebconf::TextParameter("Static ETH IP address", "ethStaticIpid", EthStaticIpValue, STRING_LEN);
iotwebconf::TextParameter ethStaticNetmaskParam = iotwebconf::TextParameter("Static ETH netmask", "ethStaticNetmaskid", EthStaticNetmaskValue, STRING_LEN);
iotwebconf::TextParameter ethStaticGatewayParam = iotwebconf::TextParameter("Static ETH Gateway", "ethStaticGatewayid", EthStaticGatewayValue, STRING_LEN);

IotWebConfParameterGroup mqttGroup = IotWebConfParameterGroup("mqttgroup", "MQTT configuration");
IotWebConfTextParameter mqttServerParam = IotWebConfTextParameter("MQTT server", "mqttServerid", mqttServerValue, STRING_LEN);
IotWebConfTextParameter mqttUserNameParam = IotWebConfTextParameter("MQTT user", "mqttUserid", mqttUserNameValue, STRING_LEN);
IotWebConfPasswordParameter mqttUserPasswordParam = IotWebConfPasswordParameter("MQTT password", "mqttPassid", mqttUserPasswordValue, STRING_LEN);
IotWebConfTextParameter mqttTopicRootParam = IotWebConfTextParameter("MQTT Topic Root", "mqttTopicRootid", mqttTopicRootValue, STRING_LEN, "growatt");


iotwebconf::OptionalGroupHtmlFormatProvider optionalGroupHtmlFormatProvider;

void IotWebConfSetup()
{
  StaticWifiGroup.addItem(&wifiStaticIpParam);
  StaticWifiGroup.addItem(&wifiStaticNetmaskParam);
  StaticWifiGroup.addItem(&wifiStaticGatewayParam);
  StaticEthGroup.addItem(&ethStaticIpParam);
  StaticEthGroup.addItem(&ethStaticNetmaskParam);
  StaticEthGroup.addItem(&ethStaticGatewayParam);
  mqttGroup.addItem(&mqttServerParam);
  mqttGroup.addItem(&mqttUserNameParam);
  mqttGroup.addItem(&mqttUserPasswordParam);
  mqttGroup.addItem(&mqttTopicRootParam);


  iotWebConf.setStatusPin(STATUS_LED); 
  iotWebConf.setConfigPin(CONFIG_PIN);
  iotWebConf.addParameterGroup(&StaticWifiGroup);
#ifdef ARDUINO_WT32_ETH01
  iotWebConf.addParameterGroup(&StaticEthGroup);  // only available for WT32_ETH01 board
#endif
  iotWebConf.addParameterGroup(&mqttGroup);
  iotWebConf.setHtmlFormatProvider(&optionalGroupHtmlFormatProvider);
  iotWebConf.setConfigSavedCallback(&configSaved);
  //iotWebConf.setApConnectionHandler(&connectAp);
  iotWebConf.setWifiConnectionHandler(&connectWifi);
  iotWebConf.setFormValidator(&formValidator);

  iotWebConf.setupUpdateServer(
    [](const char* updatePath) { httpUpdater.setup(&server, updatePath); },
    [](const char* userName, char* password) { httpUpdater.updateCredentials(userName, password); });

  // -- Initializing the configuration.
  iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.on("/request", handleDataRequest);
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  Serial.println("IotWebConfReady.");

  if(StaticEthGroup.isActive())
  {
    // use static ETH address 
  }

}

bool formValidator(iotwebconf::WebRequestWrapper* webRequestWrapper)
{
  Serial.println("Validating form.");
  bool valid = true;

  if (StaticWifiGroup.isActive()) {  
    if (!WifiIpAddress.fromString(webRequestWrapper->arg(wifiStaticIpParam.getId())))
    {
      wifiStaticIpParam.errorMessage = "Please provide a valid IP address!";
      valid = false;
    }
    if (!WifiNetmask.fromString(webRequestWrapper->arg(wifiStaticNetmaskParam.getId())))
    {
      wifiStaticNetmaskParam.errorMessage = "Please provide a valid netmask!";
      valid = false;
    }
    if (!WifiGateway.fromString(webRequestWrapper->arg(wifiStaticGatewayParam.getId())))
    {
      wifiStaticGatewayParam.errorMessage = "Please provide a valid gateway address!";
      valid = false;
    }
  }
  return valid;
}

bool connectAp(const char* apName, const char* password)
{
  // -- Custom AP settings
  return WiFi.softAP(apName, password, 4);
}
void connectWifi(const char* ssid, const char* password)
{
  if(StaticWifiGroup.isActive()) // Use static Wifi address
  {
    WifiIpAddress.fromString(String(WifiStaticIpValue));
    WifiNetmask.fromString(String(WifiStaticNetmaskValue));
    WifiGateway.fromString(String(WifiStaticGatewayValue));

    if (!WiFi.config(WifiIpAddress, WifiGateway, WifiNetmask)) {
      Serial.println("STA Failed to configure");
    }
    Serial.print("ip: ");
    Serial.println(WifiIpAddress);
    Serial.print("gw: ");
    Serial.println(WifiGateway);
    Serial.print("net: ");
    Serial.println(WifiNetmask);
  }
  WiFi.begin(ssid, password);
}



/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }

  struct growattIF::modbus_input_registers modbusdata =  growattInterface.getInputRegisters();

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send_P(200, "text/html", index_html_top);

  String s = "<h1>Growatt2mqtt</h1>";
  s += "<h2>Status</h2>";

  s += "<table><tr>";

  // ------------------- modbus data ---------------
  /*  int status;
      float solarpower, pv1voltage, pv1current, pv1power, pv2voltage, pv2current, pv2power, outputpower, gridfrequency, gridvoltage;
      float energytoday, energytotal, totalworktime, pv1energytoday, pv1energytotal, pv2energytoday, pv2energytotal, opfullpower;
      float tempinverter, tempipm, tempboost;
      int ipf, realoppercent, deratingmode, faultcode, faultbitcode, warningbitcode;
  */
  s += "<td>Growatt Status:</td>";
  s += "<td><span id=\"growatt_status\">"+ String(modbusdata.status) + "</span></td>"; 
  s += "</tr><tr>";
  s += "<td>solarpower:</td>";
  s += "<td><span id=\"growatt_solarpower\">"+ String(modbusdata.solarpower) + "</span></td>"; 
  s += "</tr><tr>";
  s += "<td>pv1voltage:</td>";
  s += "<td><span id=\"growatt_pv1voltage\">"+ String(modbusdata.pv1voltage) + "</span></td>"; 
  s += "</tr><tr>";
  s += "<td>pv1current:</td>";
  s += "<td><span id=\"growatt_pv1current\">"+ String(modbusdata.pv1current) + "</span></td>"; 
  s += "</tr><tr>";
  s += "<td>pv1power:</td>";
  s += "<td><span id=\"growatt_pv1power\">"+ String(modbusdata.pv1power) + "</span></td>"; 
  s += "</tr><tr>";
  s += "<td>pv2voltage:</td>";
  s += "<td><span id=\"growatt_pv2voltage\">"+ String(modbusdata.pv2voltage) + "</span></td>"; 
  s += "</tr><tr>";
  s += "<td>pv2current:</td>";
  s += "<td><span id=\"growatt_pv2current\">"+ String(modbusdata.pv2current) + "</span></td>"; 
  s += "</tr><tr>";
  s += "<td>pv2power:</td>";
  s += "<td><span id=\"growatt_pv2power\">"+ String(modbusdata.pv2power) + "</span></td>"; 
  s += "</tr><tr>";
  s += "<td>outputpower:</td>";
  s += "<td><span id=\"growatt_outputpower\">"+ String(modbusdata.outputpower) + "</span></td>"; 
  s += "</tr><tr>";
  s += "<td>gridfrequency:</td>";
  s += "<td><span id=\"growatt_gridfrequency\">"+ String(modbusdata.gridfrequency) + "</span></td>"; 
  s += "</tr><tr>";
  s += "<td>gridvoltage:</td>";
  s += "<td><span id=\"growatt_gridvoltage\">"+ String(modbusdata.gridvoltage) + "</span></td>"; 
  s += "</tr><tr>";

  // ------------------- System things ---------------
  s += "<td>Wifi SSID:</td>";
  s += "<td><span id=\"wifiSSID\">"+ String(WiFi.SSID()) + "</span></td>"; 
  s += "</tr><tr>";
  s += "<td>Wifi RSSI:</td>";
  s += "<td><span id=\"wifiRSSI\">"+ String(WiFi.RSSI()) + "</span></td>"; 
  s += "</tr><tr>";
  s += "<td>Wifi IP:</td>";
  s += "<td><span id=\"wifiIP\">"+ String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]) + "</span></td>";
#ifdef ARDUINO_WT32_ETH01 
  s += "</tr><tr>";
  s += "<td>Eth IP:</td>";
  s += "<td><span id=\"ethIP\">"+ String(ETH.localIP()[0]) + "." + String(ETH.localIP()[1]) + "." + String(ETH.localIP()[2]) + "." + String(ETH.localIP()[3]) + "</span></td>";
#endif
  s += "</tr></table>";

  s += "<br><br>";
  s += "Go to <a href='config'>configure page</a> to change values. <br>";
  s += "Firmware Version: ";
  s += buildversion;
  s += " - ";
  //s += "Date: " ;
  s += __DATE__;
  s += " ";
  s += __TIME__;

  server.sendContent(s);
  server.sendContent_P(index_html_bottom);
  server.sendContent("");
}

void handleDataRequest() 
{
  String value = "";   
  if(server.hasArg("voltage"))
  {
    //value = String(systemStatus.output.voltage);
  }
  else if(server.hasArg("outputCurrent"))
  {
    //value = String(systemStatus.output.current);
  }
  else if(server.hasArg("outputPower"))
  {
    //value = String(systemStatus.output.power);
  }
  else if(server.hasArg("frequency"))
  {
    //value = String(systemStatus.output.frequency);
  }
  else if(server.hasArg("outputPowerfactor"))
  {
    //value = String(systemStatus.output.pf);
  }
  else if(server.hasArg("outputPwm"))
  {
    //value = String(systemStatus.pwm);
  }
  else if(server.hasArg("solarPvPower"))
  {
    //value = String(systemStatus.solar.pvPower);
  }
  else if(server.hasArg("solarOutputPower"))
  {
    //value = String(systemStatus.solar.outputPower);
  }
  else if(server.hasArg("solarBatteryPower"))
  {
    //value = String(systemStatus.solar.batteryPower);
  }
  else if(server.hasArg("solarBatterySoc"))
  {
    //value = String(systemStatus.solar.batterySoc);
  }
  else if(server.hasArg("predictedPower"))
  {
    //value = String(systemStatus.prediction.power);
  }
  else if(server.hasArg("systemMode"))
  {
    //value = modeToString(systemStatus.mode);
  }
  else if(server.hasArg("wifiRSSI"))
  {
    value = String(WiFi.RSSI());
  }
  else if(server.hasArg("wifiSSID"))
  {
    value = String(WiFi.SSID());
  }
  else
  {
    value = "unknown arg: ";
    value += String(server.args());
    value += " - ";
    value += server.arg(1);

  }
  server.send(200, "text/plane", value);
}

void configSaved()
{
  Serial.println("Configuration was updated.");
}


void ReadInputRegisters() {
  char json[1024];
  char topic[80];

  leds[0] = CRGB::Yellow;
  FastLED.show();
  uint8_t result;


  result = growattInterface.ReadInputRegisters(json);
  if (result == growattInterface.Success) {
    leds[0] = CRGB::Green;
    FastLED.show();
    lastRGB = millis();
    ledoff = true;

#ifdef DEBUG_SERIAL
    Serial.println(result);
#endif
    sprintf(topic, "%s/data", mqttTopicRootValue);
    mqtt.publish(topic, json);
    Serial.println("Data MQTT sent");

  } else if (result != growattInterface.Continue) {
    leds[0] = CRGB::Red;
    FastLED.show();
    lastRGB = millis();
    ledoff = true;

    Serial.print(F("Error: "));
    String message = growattInterface.sendModbusError(result);
    Serial.println(message);
    char topic[80];
    sprintf(topic, "%s/error", mqttTopicRootValue);
    mqtt.publish(topic, message.c_str());
  }
}

void ReadHoldingRegisters() {
  char json[1024];
  char topic[80];

  leds[0] = CRGB::Yellow;
  FastLED.show();
  uint8_t result;

  result = growattInterface.ReadHoldingRegisters(json);
  if (result == growattInterface.Success)   {
    leds[0] = CRGB::Green;
    FastLED.show();
    lastRGB = millis();
    ledoff = true;

#ifdef DEBUG_SERIAL
    Serial.println(json);
#endif
    sprintf(topic, "%s/settings", mqttTopicRootValue);
    mqtt.publish(topic, json);
    Serial.println("Setting MQTT sent");
    // Set the flag to true not to read the holding registers again
    holdingregisters = true;

  } else if (result != growattInterface.Continue) {
    leds[0] = CRGB::Red;
    FastLED.show();
    lastRGB = millis();
    ledoff = true;

    Serial.print(F("Error: "));
    String message = growattInterface.sendModbusError(result);
    Serial.println(message);
    char topic[80];
    sprintf(topic, "%s/error", mqttTopicRootValue);
    mqtt.publish(topic, message.c_str());
    iotWebConf.delay(5);
  }
}

// This is the 1 second timer callback function
void timerCallback() {
  seconds++;

  // Query the modbus device
  if (seconds % UPDATE_MODBUS == 0) {
    //ReadInputRegisters();
    if (!holdingregisters) {
      // Read the holding registers
      ReadHoldingRegisters();
    } else {
      // Read the input registers
      ReadInputRegisters();
    }
  }

  // Send RSSI and uptime status
  if (seconds % UPDATE_STATUS == 0) {
    // Send MQTT update
    if (strlen(mqttServerValue) != 0) {
      char topic[80];
      char value[300];
      sprintf(value, "{\"rssi\": %d, \"uptime\": %lu, \"ssid\": \"%s\", \"ip\": \"%d.%d.%d.%d\", \"clientid\":\"%s\", \"version\":\"%s\"}", WiFi.RSSI(), uptime, WiFi.SSID().c_str(), WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3], thingName.c_str(), buildversion);
      sprintf(topic, "%s/%s", mqttTopicRootValue, "status");
      mqtt.publish(topic, value);
      Serial.println(F("MQTT status sent"));
    }
  }
}

// MQTT reconnect logic
void reconnect() {
  //String mytopic;
    Serial.print("Attempting MQTT connection...");
    Serial.print(F("Client ID: "));
    Serial.println(thingName.c_str());
    // Attempt to connect
    char topic[80];
    sprintf(topic, "%s/%s", mqttTopicRootValue, "connection");
    mqtt.setServer(mqttServerValue , 1883);
    if (mqtt.connect(thingName.c_str(), mqttUserNameValue, mqttUserPasswordValue, topic, 1, true, "offline")) { //last will
      Serial.println(F("connected"));
      // ... and resubscribe
      mqtt.publish(topic, "online", true);
      sprintf(topic, "%s/write/#", mqttTopicRootValue);
      mqtt.subscribe(topic);
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(mqtt.state());
    }
}

void setup() {
  FastLED.addLeds<LED_TYPE, RGBLED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness( BRIGHTNESS );
  leds[0] = CRGB::Pink;
  FastLED.show();

  Serial.begin(SERIAL_RATE);
  Serial.println(F("\nGrowatt Solar Inverter to MQTT Gateway"));
  // Init outputs, RS485 in receive mode
  pinMode(STATUS_LED, OUTPUT);


  IotWebConfSetup();

  // Initialize some variables
  uptime = 0;
  seconds = 0;


  //init ETH:
#ifdef ARDUINO_WT32_ETH01
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
  //ETH.macAddress(mac);
  if(StaticEthGroup.isActive())
  {
    EthIpAddress.fromString(String(EthStaticIpValue));
    EthNetmask.fromString(String(EthStaticNetmaskValue));
    EthGateway.fromString(String(EthStaticGatewayValue));
    ETH.config(EthIpAddress, EthGateway, EthNetmask);
  }
#endif

  // Set up the Modbus line
  growattInterface.initGrowatt();
  Serial.println("Modbus connection is set up");


  // Set up the MQTT server connection
  if (strlen(mqttServerValue) != 0) {
    mqtt.setServer(mqttServerValue, 1883);
    mqtt.setBufferSize(1024);
    mqtt.setCallback(callback);
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(thingName.c_str());

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  leds[0] = CRGB::Black;
  FastLED.show();

  iotWebConf.goOffLine();
}

// Callback from MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  // Convert the incoming byte array to a string

 
  uint8_t result;
  for (unsigned int i = 0; i < length; i++) {        // each char to upper
    payload[i] = toupper(payload[i]);
  }
  payload[length] = '\0';               // Null terminator used to terminate the char array
  String message = (char*)payload;

  char expectedTopic[40];

  sprintf(expectedTopic, "%s/write/getSettings", mqttTopicRootValue);
  if (strcmp(expectedTopic, topic) == 0) {
    if (strcmp((char *)payload, "ON") == 0) {
      holdingregisters = false;
    }
  }

  sprintf(expectedTopic, "%s/write/setEnable", mqttTopicRootValue);
  if (strcmp(expectedTopic, topic) == 0) {
    char json[50];
    char topic[80];

    if (strcmp((char *)payload, "ON") == 0) {
      growattInterface.writeRegister(growattInterface.regOnOff, 1);
      iotWebConf.delay(5);
      sprintf(json, "{ \"enable\":%d}", growattInterface.readRegister(growattInterface.regOnOff));
      sprintf(topic, "%s/settings", mqttTopicRootValue);
      mqtt.publish(topic, json);
    } else if (strcmp((char *)payload, "OFF") == 0) {
      growattInterface.writeRegister(growattInterface.regOnOff, 0);
      iotWebConf.delay(5);
      sprintf(json, "{ \"enable\":%d}", growattInterface.readRegister(growattInterface.regOnOff));
      sprintf(topic, "%s/settings", mqttTopicRootValue);
      mqtt.publish(topic, json);
    }
  }

  sprintf(expectedTopic, "%s/write/setMaxOutput", mqttTopicRootValue);
  if (strcmp(expectedTopic, topic) == 0) {
    char json[50];
    char topic[80];

    result = growattInterface.writeRegister(growattInterface.regMaxOutputActive, message.toInt());
    if (result == growattInterface.Success) {
      holdingregisters = false;
    } else {
      sprintf(json, "last trasmition has faild with: %s", growattInterface.sendModbusError(result).c_str());
      sprintf(topic, "%s/error", mqttTopicRootValue);
      mqtt.publish(topic, json);
    }
  }

  sprintf(expectedTopic, "%s/write/setStartVoltage", mqttTopicRootValue);
  if (strcmp(expectedTopic, topic) == 0) {
    char json[50];
    char topic[80];

    result = growattInterface.writeRegister(growattInterface.regStartVoltage, (message.toInt() * 10)); //*10 transmit with one digit after decimal place
    if (result == growattInterface.Success) {
      holdingregisters = false;
    } else {
      sprintf(json, "last trasmition has faild with: %s", growattInterface.sendModbusError(result).c_str());
      sprintf(topic, "%s/error", mqttTopicRootValue);
      mqtt.publish(topic, json);
    }
  }

#ifdef useModulPower
  sprintf(expectedTopic, "%s/write/setModulPower", mqttTopicRootValue);
  if (strcmp(expectedTopic, topic) == 0) {
    char json[50];
    char topic[80];

    growattInterface.writeRegister(growattInterface.regOnOff, 0);
    iotWebConf.delay(500);

    result = growattInterface.writeRegister(growattInterface.regModulPower, int(strtol(message.c_str(), NULL, 16)));
    iotWebConf.delay(500);
    growattInterface.writeRegister(growattInterface.regOnOff, 1);
    iotWebConf.delay(1500);

    if (result == growattInterface.Success) {
      holdingregisters = false;
    } else {
      sprintf(json, "last trasmition has faild with: %s", growattInterface.sendModbusError(result).c_str());
      sprintf(topic, "%s/error", mqttTopicRootValue);
      mqtt.publish(topic, json);
    }
  }
#endif

#ifdef DEBUG_SERIAL
  Serial.print(F("Message arrived on topic: ["));
  Serial.print(topic);
  Serial.print(F("], "));
  Serial.println(message);
#endif
}

void loop() {
  static int ota_once = 0;

  iotWebConf.doLoop();

  if (WiFi.status() == WL_CONNECTED) {
    // On the first time Wifi is connected, setup OTA
    if (ota_once == 0) {
      ArduinoOTA.begin();
      ota_once = 1;
    } else {
      ArduinoOTA.handle();
    }
  }

  // Handle MQTT connection/reconnection
  if (strlen(mqttServerValue) != 0) {
    if (!mqtt.connected()) {
      static unsigned long reconnectTimer = 0;
      if(timediff(millis(),reconnectTimer ) > 5000  )
      {
        reconnectTimer = millis();
        reconnect();
      }
    }
    mqtt.loop();
  }

  // Uptime calculation
  if (millis() - lastTick >= 60000) {
    lastTick = millis();
    uptime++;
  }


  if (ledoff && (millis() - lastRGB >= RGBSTATUSDELAY)) {
    ledoff = false;
    leds[0] = CRGB::Black;
    FastLED.show();
  }

  // generate 1sec timer:
  static unsigned long timemarker = 0;
  if(timediff(millis(),timemarker ) > 1000  )
  {
    timemarker = millis();
    timerCallback();

  }
}

uint32_t timediff(uint32_t t1, uint32_t t2)
{
    int32_t d = (int32_t)t1 - (int32_t)t2;
	if(d < 0) d = -d;
    return (uint32_t) d;
}

