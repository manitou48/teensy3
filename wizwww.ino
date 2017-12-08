/*
  Web Server

  SPI1 NSS,MOSI,MISO,SCK  10-13,  reset pin 9
*/

#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
static byte mac[6] = {0x0A, 0x1B, 0x3C, 0x4D, 0x5E, 0x6F};
IPAddress ip(192, 168, 1, 15);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

// web page
String str1 = "HTTP/1.0 200 OK\nContent-type:text/html\n"
              "Connection: close\n\n<!DOCTYPE HTML>\n<html>\n"
              "<H1>wizwww wiznet web server</H1>\n";
String str2 = "<br>Turn pin 14  <a href=\"/LEDON\">on</a>"
              "<br>Turn pin 14  <a href=\"/LEDOFF\">off</a>\n";
String form = "<p><form><input  type=\"text\" name=\"data1\">\n"
              "<input type=\"submit\" value=\"Submit\"></form>\n";

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) { }

  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);    // begin reset the WIZ820io
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);  // de-select WIZ820io
  delay(1);
  digitalWrite(9, HIGH);   // end reset pulse
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}


void loop() {
  // listen for incoming clients
  String str, line;
  int lines = 0;
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    line = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //       Serial.write(c);
        line += String(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println(str1);
          // output the value of each analog input pin
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
          str = "<p> uptime " + String(millis(), DEC) + " ms";
          client.println(str);
          str = "<br> pin 14 is ";
          if (digitalRead(14)) str += "<b>on</b>";
          else str += "off";
          client.println(str);
          client.println(str2);
          client.println(form);
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          lines++;
          Serial.printf("line %d  %d: ", lines, line.length());
          Serial.print(line);
          // parse query
          if (line.indexOf("GET /LEDON") != -1) digitalWrite(14, HIGH);
          if (line.indexOf("GET /LEDOFF") != -1) digitalWrite(14, LOW);
          // could parse form   ?data1=xxxxxxxxx

          line = "";
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

