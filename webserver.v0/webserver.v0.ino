//#include <enc28j60.h>
#include <EtherCard.h>
#include <net.h>
#include <OneWire.h> 

int DS18S20_Pin = 4; //DS18S20 Signal pin on digital 4

//Temperature chip i/o
OneWire ds(DS18S20_Pin); // on digital pin 4

#define STATIC 0  // set to 1 to disable DHCP (adjust myip/gwip values below)


// ethernet interface ip address
static byte myip[] = { 192,168,2,200 };
// gateway ip address
static byte gwip[] = { 192,168,2,1 };
// ethernet mac address -
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

// LED to control output
int ledPin = 10;
int ledState = LOW;             // ledState used to set the LED
long previousMillis = 0;        // will store last time LED was updated
long interval = 1000;           // interval at which to blink (milliseconds)
byte Ethernet::buffer[800]; // tcp/ip send and receive buffer
char i;

const char page[] PROGMEM = "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "\r\n"
           "<style>b,a,i{padding:5px;display:inline-block;width:100px;}.on{background:green;}.off{background:red;}i{text-align:center;}i input{width:50px;text-align:right;}</style>"
          "<body onload='PI(0,0)'>"
  "<b>Temp1</b><i id='t1'>2*</i>&deg;C<br><b>Temp2</b><i id='t2'>2*</i>&deg;C<br><b>Temp3</b><i id='t3'>2*</i>&deg;C<br><b>Temp4</b><i id='t4'>2*</i>&deg;C<br>"
  "<b>Fan1</b><i id='f1'>OFF</i><br><b>Fan2</b><i id='f2'>OFF</i><br><b>Compressor</b><i id='c'>OFF</i><br><b>TempTop</b>"
  "<i><input type='number' name='top' id='t' onchange='PI(this.name,this.value)'></i><br><b>TempBottom</b><i><input type='number' name='btm' id='b' onchange='PI(this.name,this.value)'></i>"
 ;
          
const char page1[] PROGMEM =          
  "<script>var f=['t1','t2','t3','t4','f1','f2','c','t','b'];function PI(a,b){var request=new XMLHttpRequest(); "
  "request.onreadystatechange=function(){if(this.readyState==4&&this.status==200&&this.responseXML!=null){for(var i=0;i<f.length;++i){var e=document.getElementById(f[i]);"
  " var x=this.responseXML.getElementsByTagName(f[i])[0].childNodes[0].nodeValue; if ((f[i]=='t'||f[i]=='b')&&e.value=='')e.value=x;else e.innerHTML=x;"
  " if(isNaN(e.innerHTML))if(e.innerHTML=='ON')e.className='on';else e.className='off';};};}; if (a==0)request.open('GET','s.xml?r='+Date.now(),true);else"
  " request.open('GET','i.ht?'+a+'='+b,true); request.send(null); if (a==0)setTimeout('PI(0,0)',5000);}</script></body>";

void setup() { 
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
   if (ether.begin(sizeof Ethernet::buffer, mymac,10) == 0) Serial.println( "Failed to access Ethernet controller");
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
 
  if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;
    digitalWrite(ledPin, !digitalRead(ledPin)); //toggle LED
  }
   if (ether.packetLoop(ether.packetReceive())) {
    memcpy_P(ether.tcpOffset(), page, sizeof page);
    //memcpy_P(ether.tcpOffset(), page1, sizeof page1);
    ether.httpServerReply(sizeof page - 1);
  }
  //float temperature = getTemp();
  //Serial.print ("The inside temperature in Degrees Celsius is:") ;
  //Serial.println(temperature);
  
  //delay(1000); //just here to slow down the output so it is easier to read


}
//void addressLoop (char nume,byte address) {
//  char nLength=nume.length();
//  char aLength=address.length();
//  for (i=0; i<nLength; ++i) Serial.print (nume[i]);
//  Serial.print (" : ");
//  for (i=0; i<aLength; ++i) 
//    if (aLength > 4){
//      Serial.print(address[i], HEX);
//      if (i<aLength-1) Serial.print(':');
//    }else {
//      Serial.print(address[i]);
//      if (i<aLength-1) Serial.print('.');
//    }
//}
float getTemp(){
  //returns the temperature from one DS18S20 in DEG Celsius
  
  byte data[12];
  byte addr[8];
  
  if ( !ds.search(addr)) {
    //no more sensors on chain, reset search
    ds.reset_search();
    return -1000;
  }
  
  if ( OneWire::crc8( addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return -1000;
  }
  
  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.print("Device is not recognized");
    return -1000;
  }
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end
  
  byte present = ds.reset();
  ds.select(addr);  
  ds.write(0xBE); // Read Scratchpad
  
  
  for (int i = 0; i < 9; i++) { // we need 9 bytes
   data[i] = ds.read();
  }
  
  ds.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];
  
  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;
  
  return TemperatureSum;

}
