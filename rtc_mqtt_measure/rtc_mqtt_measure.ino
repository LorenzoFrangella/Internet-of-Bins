#include <DS3231.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#define DS3231_I2C_ADDR             0x68
#define DS3231_TEMPERATURE_ADDR     0x11


const char* ssid     = "FRITZ!Box 7530 FJ";
const char* password = "15744599820340939846";
const char* mqtt_server = "broker.emqx.io";

int tempC;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

float DS3231_get_treg()
{
    int rv;  // Reads the temperature as an int, to save memory
//  float rv;
    
    uint8_t temp_msb, temp_lsb;
    int8_t nint;

    Wire.beginTransmission(DS3231_I2C_ADDR);
    Wire.write(DS3231_TEMPERATURE_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_I2C_ADDR, 2);
    temp_msb = Wire.read();
    temp_lsb = Wire.read() >> 6;

    if ((temp_msb & 0x80) != 0)
        nint = temp_msb | ~((1 << 8) - 1);      // if negative get two's complement
    else
        nint = temp_msb;

    rv = 0.25 * temp_lsb + nint;

    return rv;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

RTClib myRTC2;
DS3231 myRTC;

bool century = false;
bool h12Flag;
bool pmFlag;

void setup() {
	// Start the serial port
	Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  timeClient.begin();
  timeClient.setTimeOffset(7200);
	// Start the I2C interface
	Wire.begin();


  for (int i=0; i<5; i++){
      delay(1000);
      Serial.print(myRTC.getYear(), DEC);
      Serial.print("-");
      Serial.print(myRTC.getMonth(century), DEC);
      Serial.print("-");
      Serial.print(myRTC.getDate(), DEC);
      Serial.print(" ");
      Serial.print(myRTC.getHour(h12Flag, pmFlag), DEC); //24-hr
      Serial.print(":");
      Serial.print(myRTC.getMinute(), DEC);
      Serial.print(":");
      Serial.println(myRTC.getSecond(), DEC);
  }
}

void loop() {
  timeClient.update();

  String formattedTime = timeClient.getFormattedTime();
  
  unsigned long ntp_now = timeClient.getEpochTime();
  
  Serial.println(ntp_now);
   
  
  Serial.print("Formatted Time: ");
  Serial.println(formattedTime);
  

  DateTime now = myRTC2.now();
  unsigned long rtc_now = now.unixtime();
  
  Serial.println(rtc_now);

  Serial.print("The time difference is of: ");
  Serial.println(ntp_now-rtc_now);

  /* 
  Code part to print the actual time from the rtc
  Serial.print(myRTC.getYear(), DEC);
  Serial.print("-");
  Serial.print(myRTC.getMonth(century), DEC);
  Serial.print("-");
  Serial.print(myRTC.getDate(), DEC);
  Serial.print(" ");
  Serial.print(myRTC.getHour(h12Flag, pmFlag), DEC); //24-hr
  Serial.print(":");
  Serial.print(myRTC.getMinute(), DEC);
  Serial.print(":");
  Serial.println(myRTC.getSecond(), DEC);
  */
  tempC = DS3231_get_treg();
  String msg = "";
  msg.concat(formattedTime);
  msg+=",";
  msg+=ntp_now;
  msg+=",";
  msg+=ntp_now-rtc_now;
  msg+=",";
  msg+= tempC;
  Serial.println("message sent");
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  Serial.print("Sensor Temperature is ");
  Serial.println(tempC);
  

  
  client.publish("lorenzo/esp8622/test", msg.c_str());

  delay(10000);

  
}