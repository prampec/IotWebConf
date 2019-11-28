/**
 * IotWebConfParameter.cpp -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf
 *
 * Copyright (C) 2019 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <IotWebConfParameter.h>

IotWebConfParameter::IotWebConfParameter(
  const char* label, const char* id, char* valueBuffer, int length,
  boolean visible, const char* defaultValue)
{
  this->label = label;
  this->_id = id;
  this->valueBuffer = valueBuffer;
  this->_length = length;
  this->visible = visible;
  this->defaultValue = defaultValue;

  this->errorMessage = NULL;
}

///////////////////////////////////////////////////////////////////////////////

IotWebConfTextParameter::IotWebConfTextParameter(
  const char* label, const char* id, char* valueBuffer, int length,
  boolean visible, const char* defaultValue,
  const char* placeholder,
  const char* customHtml)
  : IotWebConfParameter(label, id, valueBuffer, length, visible, defaultValue)
{
  this->placeholder = placeholder;
  this->customHtml = customHtml;
}

String IotWebConfTextParameter::renderHtml(boolean dataArrived, boolean hasValueFromPost, String valueFromPost)
{
  return this->renderHtml("text", hasValueFromPost, valueFromPost);
}
String IotWebConfTextParameter::renderHtml(const char* type, boolean hasValueFromPost, String valueFromPost)
{
  IotWebConfTextParameter* current = this;
  char parLength[5];

  String pitem = FPSTR(IOTWEBCONF_HTML_FORM_PARAM);

  pitem.replace("{b}", current->label);
  pitem.replace("{t}", type);
  pitem.replace("{i}", current->getId());
  pitem.replace("{p}", current->placeholder == NULL ? "" : current->placeholder);
  snprintf(parLength, 5, "%d", current->getLength());
  pitem.replace("{l}", parLength);
  if (hasValueFromPost)
  {
    // -- Value from previous submit
    pitem.replace("{v}", valueFromPost);
  }
  else
  {
    // -- Value from config
    pitem.replace("{v}", current->valueBuffer);
  }
  pitem.replace(
      "{c}", current->customHtml == NULL ? "" : current->customHtml);
  pitem.replace(
      "{s}",
      current->errorMessage == NULL ? "" : "de"); // Div style class.
  pitem.replace(
      "{e}",
      current->errorMessage == NULL ? "" : current->errorMessage);

  return pitem;
}

void IotWebConfTextParameter::update(String newValue)
{
  IotWebConfParameter* current = this;
  newValue.toCharArray(current->valueBuffer, current->getLength());
}

IotWebConfSerializationData IotWebConfTextParameter::serialize()
{
  IotWebConfSerializationData serializationData;
  serializationData.valueBuffer = (byte*)this->valueBuffer;
  serializationData.length = this->getLength();
  return serializationData;
}
IotWebConfSerializationData IotWebConfTextParameter::prepareDeserialization()
{
  IotWebConfSerializationData serializationData;
  serializationData.valueBuffer = (byte*)this->valueBuffer;
  serializationData.length = this->getLength();
  return serializationData;
}
void IotWebConfTextParameter::deserialize(IotWebConfSerializationData serializationData)
{
}

void IotWebConfTextParameter::debugTo(Stream* out)
{
  IotWebConfParameter* current = this;
  out->print("'");
  out->print(current->getId());
  out->print("' with value: '");
  out->print(current->valueBuffer);
  out->print("'");
}

///////////////////////////////////////////////////////////////////////////////

IotWebConfNumberParameter::IotWebConfNumberParameter(
  const char* label, const char* id, char* valueBuffer, int length,
  boolean visible, const char* defaultValue,
  const char* placeholder,
  const char* customHtml)
  : IotWebConfTextParameter(label, id, valueBuffer, length, visible, defaultValue,
  placeholder, customHtml)
{
}

String IotWebConfNumberParameter::renderHtml(
  boolean dataArrived,
  boolean hasValueFromPost, String valueFromPost)
{
  return IotWebConfTextParameter::renderHtml("number", hasValueFromPost, valueFromPost);
}

///////////////////////////////////////////////////////////////////////////////

IotWebConfPasswordParameter::IotWebConfPasswordParameter(
  const char* label, const char* id, char* valueBuffer, int length,
  boolean visible, const char* defaultValue,
  const char* placeholder,
  const char* customHtml)
  : IotWebConfTextParameter(label, id, valueBuffer, length, visible, defaultValue,
  placeholder, customHtml)
{
}

String IotWebConfPasswordParameter::renderHtml(
  boolean dataArrived,
  boolean hasValueFromPost, String valueFromPost)
{
  return IotWebConfTextParameter::renderHtml("password", true, String(""));
}

void IotWebConfPasswordParameter::debugTo(Stream* out)
{
  IotWebConfParameter* current = this;
  out->print("'");
  out->print(current->getId());
  out->print("' with value: ");
#ifdef IOTWEBCONF_DEBUG_PWD_TO_SERIAL
  out->print("'");
  out->print(current->valueBuffer);
  out->print("'");
#else
  out->print(F("<hidden>"));
#endif
}

void IotWebConfPasswordParameter::update(String newValue)
{
  IotWebConfParameter* current = this;
//  char temp[IOTWEBCONF_WORD_LEN];
  char temp[current->getLength()];
  newValue.toCharArray(temp, current->getLength());
  if (temp[0] != '\0')
  {
    // -- Value was set.
    strncpy(current->valueBuffer, temp, current->getLength());
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.print("Updated ");
#endif
  }
  else
  {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.println("Was not changed ");
#endif
  }
}

///////////////////////////////////////////////////////////////////////////////

IotWebConfCheckboxParameter::IotWebConfCheckboxParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    boolean visible, boolean defaultValue)
  : IotWebConfTextParameter(label, id, valueBuffer, length, visible, defaultValue ? "selected" : NULL,
  NULL, NULL)
{
}

String IotWebConfCheckboxParameter::renderHtml(
  boolean dataArrived,
  boolean hasValueFromPost, String valueFromPost)
{
  boolean checkSelected = false;
  if (dataArrived)
  {
    if (hasValueFromPost && valueFromPost.equals("selected"))
    {
      checkSelected = true;
    }
  }
  else
  {
    if (this->isChecked())
    {
      checkSelected = true;
    }
  }

  if (checkSelected)
  {
    this->customHtml = IotWebConfCheckboxParameter::_checkedStr;
  }
  else
  {
    this->customHtml = NULL;
  }
  
  
  return IotWebConfTextParameter::renderHtml("checkbox", true, "selected");
}

void IotWebConfCheckboxParameter::update(String newValue)
{
  return IotWebConfTextParameter::update(newValue);
}

///////////////////////////////////////////////////////////////////////////////

IotWebConfOptionsParameter::IotWebConfOptionsParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* optionValues, const char* optionNames, size_t optionCount, size_t nameLength,
    boolean visible,
    const char* defaultValue)
  : IotWebConfTextParameter(label, id, valueBuffer, length, visible, defaultValue,
  NULL, NULL)
{
  this->_optionValues = optionValues;
  this->_optionNames = optionNames;
  this->_optionCount = optionCount;
  this->_nameLength = nameLength;
}

///////////////////////////////////////////////////////////////////////////////

IotWebConfSelectParameter::IotWebConfSelectParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* optionValues, const char* optionNames, size_t optionCount, size_t nameLength,
    boolean visible,
    const char* defaultValue)
  : IotWebConfOptionsParameter(label, id, valueBuffer, length, optionValues, optionNames,
  optionCount, nameLength,
  visible, defaultValue)
{
}

String IotWebConfSelectParameter::renderHtml(
  boolean dataArrived,
  boolean hasValueFromPost, String valueFromPost)
{
  IotWebConfTextParameter* current = this;

  String options = "";

  for (size_t i=0; i<this->_optionCount; i++)
  {
    const char *optionValue = (this->_optionValues + (i*this->getLength()) );
    const char *optionName = (this->_optionNames + (i*this->_nameLength) );
    String oitem = FPSTR(IOTWEBCONF_HTML_FORM_OPTION);
    oitem.replace("{v}", optionValue);
//    if (sizeof(this->_optionNames) > i)
    {
      oitem.replace("{n}", optionName);
    }
//    else
//    {
//      oitem.replace("{n}", "?");
//    }
    if ((hasValueFromPost && (valueFromPost == optionValue)) ||
      (strncmp(current->valueBuffer, optionValue, this->getLength()) == 0))
    {
      // -- Value from previous submit
      oitem.replace("{s}", " selected");
    }
    else
    {
      // -- Value from config
      oitem.replace("{s}", "");
    }

    options += oitem;
  }

  String pitem = FPSTR(IOTWEBCONF_HTML_FORM_SELECT_PARAM);

  pitem.replace("{b}", current->label);
  pitem.replace("{i}", current->getId());
  pitem.replace(
      "{c}", current->customHtml == NULL ? "" : current->customHtml);
  pitem.replace(
      "{s}",
      current->errorMessage == NULL ? "" : "de"); // Div style class.
  pitem.replace(
      "{e}",
      current->errorMessage == NULL ? "" : current->errorMessage);
  pitem.replace("{o}", options);

  return pitem;
}

///////////////////////////////////////////////////////////////////////////////

IotWebConfSeparator::IotWebConfSeparator()
  : IotWebConfParameter(NULL, NULL, NULL, 0)
{
}

IotWebConfSeparator::IotWebConfSeparator(const char* label)
  : IotWebConfParameter(label, NULL, NULL, 0)
{
}
String IotWebConfSeparator::renderHtml(
  boolean dataArrived,
  boolean hasValueFromPost, String valueFromPost)
{
  IotWebConfParameter* current = this;
  String pitem = "</fieldset><fieldset>";
  if (current->label != NULL)
  {
    pitem += "<legend>";
    pitem += current->label;
    pitem += "</legend>";
  }
  return pitem;
}
void IotWebConfSeparator::update(String newValue)
{
  // Does nothing.
}
IotWebConfSerializationData IotWebConfSeparator::serialize()
{
  // Does nothing.
  IotWebConfSerializationData serializationData;
  serializationData.valueBuffer = NULL;
  serializationData.length = 0;
  return serializationData;
}
IotWebConfSerializationData IotWebConfSeparator::prepareDeserialization()
{
  // Does nothing.
  IotWebConfSerializationData serializationData;
  serializationData.valueBuffer = NULL;
  serializationData.length = 0;
  return serializationData;
}
void IotWebConfSeparator::deserialize(IotWebConfSerializationData serializationData)
{
  // Does nothing.
}
void IotWebConfSeparator::debugTo(Stream* out)
{
  IotWebConfParameter* current = this;
  out->print("--");
  if (current->label != NULL)
  {
    out->print(current->label);
  }
  out->print("--");
}
