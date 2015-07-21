//#include <enc28j60.h>
#include <EtherCard.h>
#include <net.h>
#include <OneWire.h> 

int DS18S20_Pin = 4; //DS18S20 Signal pin on digital 4
#define ETH_CS_PIN  10
#define DEBUG_MODE

//Temperature chip i/o
OneWire ds(DS18S20_Pin); // on digital pin 4

#define STATIC 0  // set to 1 to disable DHCP (adjust myip/gwip values below)


// ethernet interface ip address
static byte myip[] = { 10,10,10,10 };
// gateway ip address
static byte gwip[] = { 10,0,0,1 };
// ethernet mac address -
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

// LED to control output
int ledPin = 19;
int ledState = LOW;             // ledState used to set the LED
long previousMillis = 0;        // will store last time LED was updated
long interval = 1000;           // interval at which to blink (milliseconds)
byte Ethernet::buffer[800]; // tcp/ip send and receive buffer
byte res;
signed int t1,t2,t3,t4,t,b;
char f1[3],f2[3],c1[3];

const char page[] PROGMEM = "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "\r\n"
          "<script src=\"active.js\"></script>"
           "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">"
          "<body onload='PI(0,0)'>"
  "<b>Temp1</b><i id='t1'>2*</i>&deg;C<br><b>Temp2</b><i id='t2'>2*</i>&deg;C<br><b>Temp3</b><i id='t3'>2*</i>&deg;C<br><b>Temp4</b><i id='t4'>2*</i>&deg;C<br>"
  "<b>Fan1</b><i id='f1'>OFF</i><br><b>Fan2</b><i id='f2'>OFF</i><br><b>Compressor</b><i id='c'>OFF</i><br><b>TempTop</b>"
  "<i><input type='number' name='top' id='t' onchange='PI(this.name,this.value)'></i><br><b>TempBottom</b><i><input type='number' name='btm' id='b' onchange='PI(this.name,this.value)'></i>"
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
  " var x=this.responseXML.getElementsByTagName(f[i])[0].childNodes[0].nodeValue; if ((f[i]=='t'||f[i]=='b')&&e.value=='')e.value=x;else e.innerHTML=x;"
  " if(isNaN(e.innerHTML))if(e.innerHTML=='ON')e.className='on';else e.className='off';};};}; if (a==0)request.open('GET','data.xml?r='+Date.now(),true);else"
  " request.open('GET','i.ht?'+a+'='+b,true); request.send(null); if (a==0)setTimeout('PI(0,0)',5000);}";
const char xml[] PROGMEM = "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/xml\r\n"
          "\r\n"
          "<?xml version = '1.0' ?>"
          "<inputs>"
              "<t1>$D</t1>"
              "<t2>$D</t2>"
              "<t3>$D</t3>"
              "<t4>$D</t4>"
              "<f1>$F</f1>"
              "<f2>$F</f2>"
              "<c>$F</c>"
              "<t>$D</t>"
              "<b>$D</b>"
         " </inputs>"
          ;

void setup() { 
  pinMode(ledPin, OUTPUT);
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
      
}

void loop() {
  unsigned long currentMillis = millis();
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
 
  if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;
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
    if(strncmp("GET /data.xml", request, 6) == 0) {
      Serial.print(F("Requested xml page -- size "));
      Serial.println (sizeof xml);
      BufferFiller bfill = ether.tcpOffset();
      memcpy_P(ether.tcpOffset(), xml, sizeof xml);
      ether.httpServerReply(sizeof xml - 1);  
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
    else if(strncmp("GET /i.ht?", request, 16) == 0) {
      Serial.print(F("Data for temp: "));
      // the request is i.ht?<tempId>=<tempValue>
      int tempId = request[16] - '0';
      int tempValue = request[18] - '0';
      Serial.print(tempId);
      Serial.print(F(", new temp: "));
      Serial.println(tempValue);
      Serial.println(F("Done!"));
    }
    // else throw 400
    else  {
      Serial.println(F("Unknown request"));
      
      BufferFiller bfill = ether.tcpOffset();
      bfill.emit_p(PSTR("HTTP/1.0 400 Bad Request\r\n\r\nBad Request"));
      ether.httpServerReply(bfill.position());      
    }      
  }
}


