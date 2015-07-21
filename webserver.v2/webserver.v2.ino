//#include <enc28j60.h>
//#include <EEPROM.h>
#include <EEPROMex.h>
#include <EtherCard.h>
#include <net.h>
#include <OneWire.h> 
#include <DallasTemperature.h>

int DS18B20_Bus_Top = 2; //DS18S20 Signal pin on digital 2
int DS18B20_Bus_Btm = 3;
//relay definition
#define FAN1 6
#define FAN2 5
#define COMP 8
#define RES0 9
#define OFF LOW
#define ON HIGH
// LED to control output
int ledPin = 7;
int ledState = LOW;  

#define ETH_CS_PIN  10

//#define DEBUG_MODE
//Temperature chip i/o
OneWire ds(DS18B20_Bus_Top); 
DallasTemperature sensors(&ds);

#define STATIC 0  // set to 1 to disable DHCP (adjust myip/gwip values below)
// ethernet interface ip address
static byte myip[] = { 10,10,10,10 };
// gateway ip address
static byte gwip[] = { 10,0,0,1 };
// ethernet mac address 
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

// Sensor addresses
DeviceAddress Probe01 = { 0x28, 0xFB, 0x90, 0xC3, 0x04, 0x00, 0x00, 0x0C }; //Temperature 1
DeviceAddress Probe02 = { 0x28, 0xCC, 0x92, 0x40, 0x04, 0x00, 0x00, 0xB6 };
DeviceAddress Probe03 = { 0x28, 0x4D, 0x8D, 0x40, 0x04, 0x00, 0x00, 0x78 };
DeviceAddress Probe04 = { 0x28, 0x9A, 0x80, 0x40, 0x04, 0x00, 0x00, 0xD5 };
//DeviceAddress Probe05 = { 0x28, 0xE1, 0xC7, 0x40, 0x04, 0x00, 0x00, 0x0D };

           // ledState used to set the LED
long previousMillis_1s = 0;        // will store last time LED was updated
long previousMillis_100ms = 0; 
long interval = 1000;           // interval at which to blink (milliseconds)
byte Ethernet::buffer[800]; // tcp/ip send and receive buffer
byte res;

int brightness = 0;    // how bright the LED is
int fadeAmount = 5;    // how many points to fade the LED by
 
//char f1[6],f2[6],c1[6],t1[6],t2[6],t3[6],t4[6],t[6],b[6];
float t1,t2,t3,t4,t,b;
char *f1, *f2, *c;
const char page[] PROGMEM = "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "\r\n"
          "<script src=\"active.js\"></script>"
           "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">"
          "<body onload='PI(0,0)'>"
  "<b>Temp1</b><i id='t1'>2*</i>&deg;C<br><b>Temp2</b><i id='t2'>2*</i>&deg;C<br><b>Temp3</b><i id='t3'>2*</i>&deg;C<br><b>Temp4</b><i id='t4'>2*</i>&deg;C<br>"
  "<b>Fan1</b><i id='f1'>OFF</i><br><b>Fan2</b><i id='f2'>OFF</i><br><b>Compressor</b><i id='c'>OFF</i><br><b>TempTop</b>"
  "<i><input type='number' min ='0' max = '9' name='top' id='t' onchange='PI(this.name,this.value)'></i><br><b>TempBottom</b><i><input type='number' min ='0' max = '9' name='btm' id='b' onchange='PI(this.name,this.value)'></i>"
  "</body>"
 ;
const char style[] PROGMEM = "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "\r\n"
          "b,a,i{padding:5px;display:inline-block;width:100px;}.on{background:green;}.off{background:red;}i{text-align:center;}i input{width:50px;text-align:right;}";          
const char js[] PROGMEM = "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "\r\n"         
  "var f=['t1','t2','t3','t4','f1','f2','c','t','b'];function PI(a,b){var request=new XMLHttpRequest(); "
  "request.onreadystatechange=function(){if(this.readyState==4&&this.status==200&&this.responseXML!=null){for(var i=0;i<f.length;++i){var e=document.getElementById(f[i]);"
  " var x=this.responseXML.getElementsByTagName(f[i])[0].childNodes[0].nodeValue; if ((f[i]=='t'||f[i]=='b')&&e.defaultvalue=='')e.defaultValue=x;else e.innerHTML=x;"
  " if(isNaN(e.innerHTML))if(e.innerHTML=='ON')e.className='on';else e.className='off';};};}; if (a==0)request.open('GET','data.xml?r='+Date.now(),true);else"
  " request.open('GET','setter.php?'+a+'='+b,true); request.send(null); if (a==0)setTimeout('PI(0,0)',15000);}";
const char xml[] PROGMEM = "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/xml\r\n"
          "\r\n"
          "<?xml version = '1.0' ?>"
          "<inputs>"
            "<t1>$T</t1>"
            "<t2>$T</t2>"
            "<t3>$T</t3>"
            "<t4>$T</t4>"
            "<f1>$S</f1>"
            "<f2>$S</f2>"
            "<c>$S</c>"
            "<t>$T</t>"
            "<b>$T</b>"
         " </inputs>"
          ;
const char ok[] PROGMEM = "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "\r\n";
/*
  %D = unsigned integer
  %T = Floating point
  %L = long
  %H = hexadecimal
  %S = string
  %F = PROGMEM string
  %E = EEPROM string
 */
void setup() { 
  pinMode(ledPin, OUTPUT);
  pinMode(FAN1, OUTPUT);
  pinMode(FAN2, OUTPUT);
  pinMode(COMP, OUTPUT);
  pinMode(RES0, OUTPUT);
  //EVERYTHING IF OFF WHEN WE START
  digitalWrite(FAN1, OFF);
  digitalWrite(FAN2, OFF);
  digitalWrite(COMP, OFF);
  digitalWrite(RES0, OFF);
  
  Serial.begin(9600);
   if (!ether.begin(sizeof Ethernet::buffer, mymac, ETH_CS_PIN)) {
    Serial.println( "Failed to access Ethernet controller. Dying...");
    while(1);
   }
   if (STATIC)   ether.staticSetup(myip, gwip);
   else
      if (!ether.dhcpSetup()) {
        Serial.println("DHCP failed, using static.");
        ether.staticSetup(myip, gwip);
      }
    ether.printIp("IP:  ", ether.myip);
    ether.printIp("GW:  ", ether.gwip);  
    ether.printIp("DNS: ", ether.dnsip);
    Serial.print("MAC: ");
    for (byte i = 0; i < 6; ++i) {
      Serial.print(mymac[i], HEX);
      if (i < 5)
        Serial.print(':');
    }
    Serial.println();
    Serial.println("Initialising Temperature Sensors...");
    sensors.begin();
    Serial.print("Number of Devices found on bus = ");  
    Serial.println(sensors.getDeviceCount());  
    // set the resolution to 10 bit (Can be 9 to 12 bits .. lower is faster)
    sensors.setResolution(Probe01, 12);
    sensors.setResolution(Probe02, 12);
    sensors.setResolution(Probe03, 12);
    sensors.setResolution(Probe04, 12);
    //sensors.setResolution(Probe05, 12);
    t=4.00; //toptemp = 4
    b=7.00; //bottomtemp = 7 temps we want to achieve
    if (EEPROM.read(0x00)==0xFF&&EEPROM.read(0x01)==0xFF) { //eeprom not initialised, writing default values
      EEPROM.writeFloat(0x00,t);
      EEPROM.writeFloat(0x10,b);
    } else {
      t=EEPROM.readFloat(0x00);
      b=EEPROM.readFloat(0x10);
    }
    delay(500); // 500 ms delay to wait for init
    
}

void loop() {
  getRelays();
  unsigned long currentMillis = millis();
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  if(currentMillis - previousMillis_100ms > 100) { 
    //stuff that happens every 0.1 sec
    previousMillis_100ms = currentMillis;
//    analogWrite(ledPin, brightness);
//    brightness = brightness + fadeAmount;
//    if (brightness == 0 || brightness == 255) {
//      fadeAmount = -fadeAmount ;
//    }
  }
  if(currentMillis - previousMillis_1s > 1000) {
    // stuff that happens every sec
    previousMillis_1s = currentMillis;
    updateTemps ();
    setRelays();
    digitalWrite(ledPin, !digitalRead(ledPin)); //toggle LED
  }
  if(pos) {

    char* request = (char *)Ethernet::buffer + pos;
    
    #ifdef DEBUG_MODE
    Serial.println(F("---------------------------------------- NEW PACKET ----------------------------------------"));
    Serial.println(request);
    Serial.println(F("--------------------------------------------------------------------------------------------"));
    Serial.println();
    #endif
    if(strncmp("GET /setter.php?", request, 16) == 0) {
      Serial.print(F("Data for temp: "));
      // the request is set.php?<tempId>=<tempValue>
      char tempId = request[16];
      char tempValue = (float)(request[20]-48);
      if (tempId == 't') t=tempValue; else b=tempValue;
      Serial.print(tempId);
      Serial.print(F(", new temp: "));
      Serial.println(tempValue);
      Serial.println(F("Done!"));
      memcpy_P(ether.tcpOffset(), ok, sizeof ok);
      ether.httpServerReply(sizeof ok - 1);
    }
    
    else if(strncmp("GET /data.xml", request, 6) == 0) {
      Serial.print(F("Requested xml page -- size "));
      Serial.println (sizeof xml);
      BufferFiller bfill = ether.tcpOffset();
      bfill.emit_p(xml,t1,t2,t3,t4,f1,f2,c,t,b);
       ether.httpServerReply(bfill.position());      
    }
    // if the request is for the "default" page
    else if(strncmp("GET / ", request, 6) == 0) {
      Serial.print(F("Requested default page -- size "));
      Serial.println (sizeof page);
      memcpy_P(ether.tcpOffset(), page, sizeof page);
      ether.httpServerReply(sizeof page - 1);
    }
    else if(strncmp("GET /style.css", request, 6) == 0) {
      Serial.print(F("Requested css page -- size "));
      Serial.println (sizeof style);
      memcpy_P(ether.tcpOffset(), style, sizeof style);
      ether.httpServerReply(sizeof style - 1);
    }
    else if(strncmp("GET /active.js", request, 6) == 0) {
      Serial.print(F("Requested js page -- size "));
      Serial.println (sizeof js);
      memcpy_P(ether.tcpOffset(), js, sizeof js);
      ether.httpServerReply(sizeof js - 1);
    }
    // if the request is for the switch page, get the parameters and change the relay status
    
    // else throw 400
    else  {
      Serial.println(F("Unknown request"));
      
      BufferFiller bfill = ether.tcpOffset();
      bfill.emit_p(PSTR("HTTP/1.0 400 Bad Request\r\n\r\nBad Request"));
      ether.httpServerReply(bfill.position());      
    }      
  }

}

void updateTemps () {
  sensors.requestTemperatures(); // Send the command to get temperatures
  t1 = sensors.getTempC(Probe01);
  t2 = sensors.getTempC(Probe02);
  t3 = sensors.getTempC(Probe03);
  t4 = sensors.getTempC(Probe04);
}
void getRelays() {
  if(digitalRead(FAN1)==ON) f1="ON"; else f1="OFF";
  if(digitalRead(FAN2)==ON) f2="ON"; else f2="OFF";
  if(digitalRead(COMP)==ON) c="ON"; else c="OFF";
  //if(digitalRead(RES0)==ON) f1="ON"; else f1="OFF";
}

void setRelays() {
  //todo --- setting of relays

}

