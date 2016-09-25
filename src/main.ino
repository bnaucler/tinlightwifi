/*
		tinLight WiFi 0.2
		By Björn Westerberg Nauclér
		mail@bnaucler.se
		
		Absolutely no rights reserved.
		This code is far from functional!

		USAGE:
		At first launch the tinLight acts as an access point
		and creates a WiFi network with SSID "tinlight".
		Connect to this SSID and connect to http://192.168.4.1/ 
		You can now enter details for your own WiFi network.
		The tinLight will reboot and gain an IP address.
		
		HARDWARE RESET:
		Keep the button pressed for three seconds.

		TODO:
		* Make HTML look neat
		* Make reset not cause http timeout
		* OTA functionality
			http://esp8266.github.io/Arduino/versions/2.1.0-rc2/doc/ota_updates/ota_updates.html
		* Actually make this thing to a lamp
		* GUI color themes?

*/

#include <Arduino.h>
#include <string.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <Adafruit_NeoPixel.h>

// WiFi definitions
const char*				APSSID = "tinlight";	// SSID when acting as AP
const char*				APpsk = "tinlight";		// PSK when acting as AP

// Pin definitions
const int				pixelPin = 2;
const int				buttonPin = 0;

// EEPROM
const int				eepromSize = 512;		// 512B for ESP01
const int				isClientAddr = 1;
const int				SSIDStrlenAddr = 2;
const int				pskStrlenAddr = 3;
const int				SSIDAddr = 10;
const int				pskAddr = 50;
const int				isClientData = 9;

// Pixel constants and variables
const int				numPixels = 13;

int						color1, color2, color3;

// Timer
unsigned long			timer;

// Enable Neopixel strip
Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, pixelPin, NEO_GRB + NEO_KHZ800);

// Enable HTTPD
WiFiServer server(80);

void setupClient() {

	byte SSIDStrlen = EEPROM.read(SSIDStrlenAddr);
	byte pskStrlen = EEPROM.read(pskStrlenAddr);

	char ssid[SSIDStrlen];
	char psk[pskStrlen];

	for(int a = 0; a <= SSIDStrlen; a++) { ssid[a] = EEPROM.read(SSIDAddr + a); }
	for(int a = 0; a <= pskStrlen; a++) { psk[a] = EEPROM.read(pskAddr + a); }

	WiFi.begin(ssid, psk);
}

void setupAP() {
	WiFi.mode(WIFI_AP);
	WiFi.softAP(APSSID, APpsk);
}

String makeHTML(char *title, String data) {

	String htmlData = "HTTP/1.1 200 OK\r\n";
	htmlData += "Content-Type: text/html\r\n\r\n";
	htmlData += "<!DOCTYPE HTML>\r\n";
	htmlData += "<HTML><HEAD>";
	htmlData += "<STYLE TYPE=\"text/css\">\r\n";
	htmlData += "a {text-decoration: none;}\r\n";
	htmlData += "a:link {color: red;}\r\n";
	htmlData += "a:visited {color: red;}\r\n";
	htmlData += "a:hover {color: yellow;}\r\n";
	htmlData += "a:active {color: yellow;}\r\n";
	htmlData += "</STYLE>\r\n";
	htmlData += "<TITLE>\r\n";
	htmlData += title;
	htmlData += "</TITLE></HEAD>\r\n";
	htmlData += "<BODY BGCOLOR=500000 TEXT=FFFFFF>\r\n";
	htmlData += "<FONT FACE=sans-serif>\r\n";
	htmlData += "<CENTER>\r\n";
	htmlData += "<BR><BR>\r\n";
	htmlData += "<DIV STYLE=\"width:60%; background: #111111\">\r\n";
	htmlData += "<BR>\r\n";
	htmlData += data;
	htmlData += "<BR><BR></CENTER>\r\n";
	htmlData += "</DIV></FONT></BODY></HTML>\r\n";

	return htmlData;
}

String htmlReset() {

	char title[] = "Restored to factory default";

	for(int a = 0; a < eepromSize; a++) { EEPROM.write(a, 0);}
	EEPROM.commit();

	String data = "Your tinLight has been reset to factory defaults.<BR><BR>\r\n";
	data += "Please restart and connect to the wireless network '";
	data += APSSID;
	data += "' with password '";
	data += APpsk;
	data += "'<BR>When connected, point your browser to "; 
	data += "<A HREF=\"http://192.168.4.1\">http://192.168.4.1</A>\r\n";

	String htmlData = makeHTML(title, data);
	return htmlData;
}

String htmlSettings() {

	char title[] = "Change Wifi configuration";

	String data = "<FORM ACTION=setwifi METHOD=GET>\r\n";
	data += "<BR>\r\n";
	data += "SSID (Network name):<BR>\r\n";
	data += "<INPUT TYPE=TEXT NAME=ssid><BR>\r\n";
	data += "<BR>\r\n";
	data += "Password:<BR>\r\n";
	data += "<INPUT TYPE=TEXT NAME=psk><BR><BR>\r\n";
	data += "<INPUT TYPE=SUBMIT VALUE=Submit>\r\n";

	// DEBUG
	for(int a=0; a <= numPixels; a++) {
		strip.setPixelColor(a,255,0,0);
	}
	strip.show();

	String htmlData = makeHTML(title, data);
	return htmlData;
}

String htmlIndex(char *input) {

	char title[] = "tinLight";

	String data = "Hello.<BR>\r\n";
	data += "Welcome to tinLight<BR>\r\n";
	data += "<BR><BR>\r\n";
	data += "<BR><BR><BR>\r\n";
	data += "| <A HREF=wificonfig>WiFi configuration</A> | \r\n";
	data += "<A HREF=reset>Reset to factory defaults</A> |\r\n";

	String htmlData = makeHTML(title, data);
	return htmlData;
}

String htmlSetWifi(char *ssid, char *psk) {

	int SSIDStrlen = strlen(ssid);
	int pskStrlen = strlen(psk);
	char title[] = "Configuration updated";

	// Write Wifi data to all EEPROM registers
	EEPROM.write(SSIDStrlenAddr, SSIDStrlen);
	EEPROM.write(pskStrlenAddr, pskStrlen);
	EEPROM.write(isClientAddr, isClientData);
	for(int a = 0; a < SSIDStrlen; a++) { EEPROM.write(SSIDAddr + a, ssid[a]); }
	for(int a = 0; a < pskStrlen; a++) { EEPROM.write(pskAddr + a, psk[a]); }
	EEPROM.commit();

	// Return a response
	String data = "Your configuration has been updated.<BR>\r\n";
	data += "Restart tinLight for changes to have effect.<BR><BR>\r\n";
	data += "New SSID:<BR>\r\n";
	data += ssid;
	data += "<BR>New Password:<BR>\r\n";
	data += psk;

	String htmlData = makeHTML(title, data);
	return htmlData;
}

String getHtml(char *inputString) {

	String htmlData;
	char *parser, *tmpPtr, *page, *arg1, *arg2;
	char delimit[] = " ?&/=\r\n";

	parser = strtok_r(inputString, delimit, &tmpPtr);
	parser = strtok_r(NULL, delimit, &tmpPtr);
	page = parser;	

	if(strcmp(page, "setwifi") == 0) {
		parser = strtok_r(NULL, delimit, &tmpPtr);
		parser = strtok_r(NULL, delimit, &tmpPtr);
		arg1 = parser;

		parser = strtok_r(NULL, delimit, &tmpPtr);
		parser = strtok_r(NULL, delimit, &tmpPtr);
		arg2 = parser;

		htmlData = htmlSetWifi(arg1, arg2);

	} else if(strcmp(page, "wificonfig") == 0) {
		htmlData = htmlSettings(); 
	} else if(strcmp(page, "reset") == 0) {
		htmlData = htmlReset(); 
	} else { 
		htmlData = htmlIndex(page);
	}

	return htmlData;
}

void setup() {

	// Set pin directions
	pinMode(pixelPin, OUTPUT);
	pinMode(buttonPin, INPUT);

	// Enable EEPROM
	EEPROM.begin(eepromSize);

	// Enable pixel strip
	strip.begin();
	strip.show();

	// Verify if Wifi data has been set
	if(EEPROM.read(isClientAddr) == isClientData) { setupClient(); }
	else { setupAP(); }

	server.begin();
}

void loop() {

	WiFiClient client = server.available();
	if (!client) { return; }

	String request = client.readStringUntil('\r');

	// Convert string to char array to avoid conversion errors
	int reqStrlen = request.length() + 1;
	char reqChar[reqStrlen];
	request.toCharArray(reqChar, reqStrlen); 
	
	client.flush();

	String htmlData = getHtml(reqChar);
	client.print(htmlData);
	delay(1);
}
