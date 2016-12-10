/**
 * File: http.ino
 * Description:
 * IotWebConf is an ESP8266 stand alone WiFi/AP web configuration template for Arduino.
 *
 * Author: Balazs Kelemen
 * Contact: prampec+arduino@gmail.com
 * Copyright: 2012 Balazs Kelemen
 * Copying permission statement:
    This file is part of IotWebConf.
    PciManager is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * This file contains helper functions to handle http data, especialy variables posted by a html form.
 */
void flushTillBody(Stream* s)
{
    while(true) {
      String line = s->readStringUntil('\r');
//      Serial.print("Head: ");
//      Serial.print(line.length());
//      Serial.print(" ");
//      Serial.println(line);
//      dumpHex(&line);
      if ((line.length() <= 0) || ((line.length() == 1) && (line.charAt(0) == '\n')))
        break;
    }
}

void dumpHex(String* s)
{
  Serial.print("[");
  for(byte i = 0; i<s->length(); i++)
  {
    Serial.print(" ");
    Serial.print(s->charAt(i), HEX);
  }
  Serial.println("]");
}

String data;
/**
 * Reads the URL encoded data line from the stream.
 */
void readData(Stream* s)
{
  data = s->readStringUntil('\r');
  data.replace("\n", "&");
  data = data + "&";
//  Serial.print("Data: ");
//  Serial.println(data);
}

/**
 * Parses a value from the URL encoded data line.
 */
char* readValue(char* variable, char* place, unsigned int len)
{
  String varPattern = "&";
  varPattern += variable;
  varPattern += "=";
  int start = data.indexOf(varPattern);
//  Serial.print("variable found at: ");
//  Serial.println(start);
  if (start == -1)
  {
    return NULL;
  }
  start += varPattern.length();
//  Serial.print("value starts at: ");
//  Serial.println(start);

  int ends = data.indexOf("&", start);
//  Serial.print("value ends before: ");
//  Serial.println(ends);

  String value = data.substring(start, ends);
  value.replace("+", " ");
  for(int i=1;i<256;i++)
  {
    replaceEscaped(&value, i);
  }

//  Serial.print(variable);
//  Serial.print("= [");
//  Serial.print(value);
//  Serial.println("]");

  value.toCharArray(place, len);

  return place;
}

char escTemp[4];
void replaceEscaped(String* s, int v)
{
  sprintf(escTemp, "%%%02X", v);
//Serial.print("(");
//Serial.print(escTemp);
//Serial.print(")");
//  String r = "%";
//  r += String(v, HEX);
  s->replace(escTemp, String((char)v));
}

