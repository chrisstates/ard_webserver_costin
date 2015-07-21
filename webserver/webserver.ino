#include <EEPROMex.h>
#include <EtherCard.h>
#include <net.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
//Temperature chip i/o
int DS18B20_Bus_Top = 2; //DS18S20-1 Signal pin on digital 2
int DS18B20_Bus_Btm = 3; //DS18S20-2 Signal pin on digital 3
int DS18B20_Bus_Int = 4; //DS18S20-3 Signal pin on digital 4
OneWire ds_1(DS18B20_Bus_Top);
OneWire ds_2(DS18B20_Bus_Btm);  
OneWire ds_3(DS18B20_Bus_Int);  
DallasTemperature Top_Bus(&ds_1);
DallasTemperature Btm_Bus(&ds_2);
DallasTemperature Int_Bus(&ds_3);
//relay definition
#define FAN1 6
#define FAN2 5
#define COMP 8
#define RES0 9 //mains relay
#define OFF LOW
#define ON HIGH
// LED to control output
int ledPin = 7;
int ledState = LOW;  

#define ETH_CS_PIN  10

//#define DEBUG_MODE  //uncomment for debug info

#define STATIC 0  // set to 1 to disable DHCP (adjust myip/gwip values below)
// ethernet interface ip address
static byte myip[] = { 10,10,10,10 };
// gateway ip address
static byte gwip[] = { 10,0,0,1 };
// ethernet mac address 
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

long previousMillis_1s = 0;        
long previousMillis_100ms = 0; 
long interval = 1000;           
byte Ethernet::buffer[900]; // tcp/ip send and receive buffer
byte temp_roll;
float t1,t2,t3,t4,t,b,ti,deltaTop,deltaBtm,delta2Btm,deltaFault,oldTop,oldBtm,avgTop,avgBtm;
char *f1, *f2, *c;
const char page[] PROGMEM = "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "\r\n"
          "<script src=\"active.js\"></script>"
           "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">"
          "<body onload='PI(0,0)'>"
  "<b>Temp1</b><i id='t1'>2*</i>&deg;C<br><b>Temp2</b><i id='t2'>2*</i>&deg;C<br><b>Temp3</b><i id='t3'>2*</i>&deg;C<br><b>Temp4</b><i id='t4'>2*</i>&deg;C<br><b>TempInt</b><i id='ti'>2*</i>&deg;C<br>"
  "<b>Fan1</b><i id='f1'>OFF</i><br><b>Fan2</b><i id='f2'>OFF</i><br><b>Compressor</b><i id='c'>OFF</i><br><b>TempTop</b>"
  "<i><input type='number' min='0' max='9' name='top' id='to' onchange='PI(this.name,this.value)'></i><i id='t'></i><br><b>TempBottom</b><input type='number' min='0' max='9' name='btm' id='bo' onchange='PI(this.name,this.value)'></i><i id='b'></i>"
  "</body>"
 ;
const char style[] PROGMEM = "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "\r\n"
          "b,a,i{padding:5px;display:inline-block;width:100px;}.on{background:green;}.off{background:red;}i{text-align:center;}i input{width:50px;text-align:right;}";          
const char js[] PROGMEM = "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "\r\n"         
  "var f=['t1','t2','t3','t4','ti','f1','f2','c','t','b'];function PI(a,b){var request=new XMLHttpRequest(); "
  "request.onreadystatechange=function(){if(this.readyState==4&&this.status==200&&this.responseXML!=null){for(var i=0;i<f.length;++i){var e=document.getElementById(f[i]);"
  " var x=this.responseXML.getElementsByTagName(f[i])[0].childNodes[0].nodeValue; e.innerHTML=x;"
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
            "<ti>$T</ti>"
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
  digitalWrite(RES0, ON);
  //deltas
  deltaTop = 5.00;
  deltaBtm = 1.00;  // delta on set temperature
  delta2Btm = 0.50; // delta between t3 and t4
  deltaFault = 25.00; // delta between 2 readings that triggers fault
  
  Serial.begin(9600);
   if (!ether.begin(sizeof Ethernet::buffer, mymac, ETH_CS_PIN)) {
    Serial.println(F( "Failed to access Ethernet controller. Doing without..."));
   }
   if (STATIC)   ether.staticSetup(myip, gwip);
   else
      if (!ether.dhcpSetup()) {
        Serial.println(F("DHCP failed, using static."));
        ether.staticSetup(myip, gwip);
      }
    ether.printIp(F("IP:  "), ether.myip);
    ether.printIp(F("GW:  "), ether.gwip);  
    ether.printIp(F("DNS: "), ether.dnsip);
    Serial.print(F("MAC: "));
    for (byte i = 0; i < 6; ++i) {
      Serial.print(mymac[i], HEX);
      if (i < 5)
        Serial.print(':');
    }
    Serial.println();
    Serial.println(F("Initialising Temperature Sensors..."));
    Top_Bus.begin();
    Serial.print(F("Number of Devices found on Top bus = "));  
    Serial.println(Top_Bus.getDeviceCount());  
    Btm_Bus.begin();
    Serial.print(F("Number of Devices found on Btm bus = "));  
    Serial.println(Btm_Bus.getDeviceCount());  
    Int_Bus.begin();
    Serial.print(F("Number of Devices found on Int bus = "));  
    Serial.println(Int_Bus.getDeviceCount());

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
    temp_roll = 0;
    
}

void loop() {
  getRelays();
  unsigned long currentMillis = millis();
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  if(currentMillis - previousMillis_100ms > 100) {
    previousMillis_100ms = currentMillis;
    //stuff that happens every 0.1 sec goes here
   // checkFault();
  }
  if(currentMillis - previousMillis_1s > 1000) {
    previousMillis_1s = currentMillis;
    // stuff that happens every sec goes here
    updateTemps();
    setRelays();
    
    digitalWrite(ledPin, !digitalRead(ledPin)); //toggle heartbeat LED
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
      if (tempId == 't') {
        t=tempValue;
        EEPROM.updateFloat(0x00,t);
      }
      else {
        b=tempValue;
        EEPROM.updateFloat(0x10,b);
      }
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
      bfill.emit_p(xml,t1,t2,t3,t4,ti,f1,f2,c,t,b);
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
  ++temp_roll;
  if (temp_roll ==1) Top_Bus.requestTemperatures(); // Send the command to get temperatures
  else if (temp_roll == 2) Btm_Bus.requestTemperatures(); // Send the command to get temperatures
  else Int_Bus.requestTemperatures(); // Send the command to get temperatures
  if (temp_roll >2) temp_roll=0;
  t1 = Top_Bus.getTempCByIndex(0);
  t2 = Top_Bus.getTempCByIndex(1);
  t3 = Btm_Bus.getTempCByIndex(0);
  t4 = Btm_Bus.getTempCByIndex(1);
  ti = Int_Bus.getTempCByIndex(0);
  #ifdef DEBUG_MODE
  Serial.print(F("t1 = "));
  Serial.println (t1);
  Serial.print(F("t2 = "));
  Serial.println (t2);
  Serial.print(F("t3 = "));
  Serial.println (t3);
  Serial.print(F("t4 = "));
  Serial.println (t4);
  Serial.print(F("ti = "));
  Serial.println (ti);
  #endif
}
void Fault () {
  digitalWrite(FAN1, OFF);
  digitalWrite(FAN2, OFF);
  digitalWrite(COMP, OFF);
  digitalWrite(RES0, OFF);
  digitalWrite(ledPin, ON);   //green LED stays on if fault detected
  Serial.print(F("Fault has been triggered. Dying..."));
  while (1);                  //and we stop here
}
void getRelays() {
  if(digitalRead(FAN1)==ON) f1="ON"; else f1="OFF";
  if(digitalRead(FAN2)==ON) f2="ON"; else f2="OFF";
  if(digitalRead(COMP)==ON) c="ON"; else c="OFF";
  //if(digitalRead(RES0)==ON) f1="ON"; else f1="OFF";
}

void setRelays() {
  //todo --- setting of relays
  oldTop = avgTop;
  oldBtm = avgBtm;
  avgTop = (t1+t2)/2.00;
  avgBtm = (t3+t4)/2.00;
  if (avgTop > (t+deltaTop/2)) { // if average temp on top is bigger than set temp + delta/2, start everything
    digitalWrite(FAN1, ON);
    digitalWrite(FAN2, ON);
    digitalWrite(COMP, ON);
  }
  if (avgTop < (t-deltaTop/2)) { // if average temp on top is lower than set temp - delta/2, stop everything
    digitalWrite(FAN1, OFF);
    digitalWrite(FAN2, OFF);
    digitalWrite(COMP, OFF);
  }
  if (avgBtm > (b+deltaBtm/2)) { 
    digitalWrite(FAN1, ON);
  }
  if (avgBtm < (b-deltaBtm/2)) {
    digitalWrite(FAN1, OFF);
  }
  if (abs(abs(t3)-abs(t4))>deltaBtm) {
    digitalWrite(FAN2, ON);
  }
  else {
    digitalWrite(FAN2, OFF);
  }
  
  
}
void checkFault() {
  if (t1<-45.00 || t1 > 75.00) { // if the sensor is missing or fails
    Serial.print(F("t1 faulty"));
    Fault();
  }
  if (t2<-45.00 || t2 > 75.00) {
    Serial.print(F("t2 faulty"));
    Fault();
  }
  if (t3<-45.00 || t3 > 75.00) {
    Serial.print(F("t3 faulty"));
    Fault();
  }
  if (t4<-45.00 || t4 > 75.00) {
    Serial.print(F("t4 faulty"));
    Fault();
  }
  if (ti<-45.00 || ti > 75.00) {
    Serial.print(F("ti faulty"));
    Fault();
  }
  if (abs(abs(avgTop)-abs(oldTop))>deltaFault || abs(abs(avgBtm)-abs(oldBtm))>deltaFault) Fault(); // if the delta between 2 readings os above a treshold...
  Serial.print(F("Temperature difference too high. "));
}

