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
		* Make HTML a little more neat
		* Create reset function
		* Actually make this thing to a lamp

*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <string.h>
#include <EEPROM.h>

// WiFi definitions
const char*				APSSID = "tinlight";	// SSID when acting as AP
const char*				APpsk = "tinlight";		// PSK when acting as AP

// Pin definitions
const int				pixelPin = 2;
const int				buttonPin = 0;

// EEPROM
const int				eepromSize = 512;		// 512B for ESP8266
const int				isClientAddr = 1;
const int				SSIDStrlenAddr = 2;
const int				pskStrlenAddr = 3;
const int				SSIDAddr = 10;
const int				pskAddr = 50;
const int				isClientData = 9;

/* int 					isClient; */

// Enable HTTPD
WiFiServer server(80);

void clearEEPROM()
{
	for(int a = 0; a < eepromSize; a++) { EEPROM.write(a, 0);}
	EEPROM.commit();
}

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

String htmlSettings() {

	String htmlData = "HTTP/1.1 200 OK\r\n";
	htmlData += "Content-Type: text/html\r\n\r\n";
	htmlData += "<!DOCTYPE HTML>\r\n";
	htmlData += "<HTML><HEAD><TITLE>Set Wifi Data</TITLE> </HEAD>\r\n";
	htmlData += "<BODY BGCOLOR=111111 TEXT=FFFFFF>\r\n";
	htmlData += "<FORM ACTION=setwifi METHOD=GET>\r\n";
	htmlData += "<BR>\r\n";
	htmlData += "SSID (Network name):<BR>\r\n";
	htmlData += "<INPUT TYPE=TEXT NAME=ssid VALUE=""><BR>\r\n";
	htmlData += "<BR>\r\n";
	htmlData += "Password:<BR>\r\n";
	htmlData += "<INPUT TYPE=TEXT NAME=psk VALUE=""><BR><BR>\r\n";
	htmlData += "<INPUT TYPE=SUBMIT VALUE=Submit>\r\n";
	htmlData += "</FORM></BODY></HTML>\r\n";

	return htmlData;
}

String htmlIndex(char *input) {

	String htmlData = "HTTP/1.1 200 OK\r\n";
	htmlData += "Content-Type: text/html\r\n\r\n";
	htmlData += "<!DOCTYPE HTML>\r\n";
	htmlData += "<HTML><HEAD><TITLE>tinLight configuration</TITLE> </HEAD>\r\n";
	htmlData += "<BODY BGCOLOR=111111 TEXT=FFFFFF>\r\n";
	htmlData += "Hello. This is the main screen.\r\n";
	htmlData += "<BR><BR><BR>\r\n";
	htmlData += "Debug data:<BR>\r\n";
	htmlData += input;
	/* htmlData += "<BR>\r\n"; */
	/* htmlData += isClient; */
	htmlData += "<BR><BR><BR>\r\n";
	htmlData += "<A HREF=wificonfig>WiFi configuration</A>\r\n";
	htmlData += "</BODY></HTML>\r\n";

	return htmlData;
}

String htmlSetWifi(char *ssid, char *psk) {

	int SSIDStrlen = strlen(ssid);
	int pskStrlen = strlen(psk);

	// Write Wifi data to all registers
	EEPROM.write(SSIDStrlenAddr, SSIDStrlen);
	EEPROM.write(pskStrlenAddr, pskStrlen);
	EEPROM.write(isClientAddr, isClientData);
	for(int a = 0; a < SSIDStrlen; a++) { EEPROM.write(SSIDAddr + a, ssid[a]); }
	for(int a = 0; a < pskStrlen; a++) { EEPROM.write(pskAddr + a, psk[a]); }
	EEPROM.commit();

	// Return a response
	String htmlData = "HTTP/1.1 200 OK\r\n";
	htmlData += "Content-Type: text/html\r\n\r\n";
	htmlData += "<!DOCTYPE HTML>\r\n";
	htmlData += "<HTML><HEAD><TITLE>tinLight configuration</TITLE></HEAD>\r\n";
	htmlData += "<BODY BGCOLOR=111111 TEXT=FFFFFF>\r\n";
	htmlData += "Your configuration has been updated.<BR>\r\n";
	htmlData += "Restart tinLight for changes to have effect.<BR><BR>\r\n";
	htmlData += "New SSID:<BR>\r\n";
	htmlData += ssid;
	htmlData += "<BR>New Password:<BR>\r\n";
	htmlData += psk;
	htmlData += "</BODY></HTML>\r\n";

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
	} else { 
		htmlData = htmlIndex(page);
	}

	return htmlData;
}

void setup() 
{
	pinMode(pixelPin, OUTPUT);
	pinMode(buttonPin, INPUT);

	EEPROM.begin(eepromSize);

	if(EEPROM.read(isClientAddr) == isClientData) { setupClient(); }
	else { setupAP(); }
	server.begin();
}

void loop() 
{
	WiFiClient client = server.available();
	if (!client) { return; }

	String request = client.readStringUntil('\r');
	int reqStrlen = request.length() + 1;
	char reqChar[reqStrlen];

	request.toCharArray(reqChar, reqStrlen); 
	client.flush();

	String htmlData = getHtml(reqChar);
	client.print(htmlData);
	delay(1);
}
