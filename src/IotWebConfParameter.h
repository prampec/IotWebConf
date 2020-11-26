/**
 * IotWebConfParameter.h -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf
 *
 * Copyright (C) 2019 Balazs Kelemen <prampec+arduino@gmail.com>
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef IotWebConfParameter_h
#define IotWebConfParameter_h

#include <Arduino.h>
#include <IotWebConfSettings.h>

// TODO: use webserver wrapper
#include <IotWebConfCompatibility.h>


const char IOTWEBCONF_HTML_FORM_PARAM[] PROGMEM =
  "<div class='{s}'><label for='{i}'>{b}</label><input type='{t}' id='{i}' "
  "name='{i}' maxlength={l} placeholder='{p}' value='{v}' {c}/>"
  "<div class='em'>{e}</div></div>\n";

const char IOTWEBCONF_HTML_FORM_SELECT_PARAM[] PROGMEM =
  "<div class='{s}'><label for='{i}'>{b}</label><select id='{i}' "
  "name='{i}' {c}/>\n{o}"
  "</select><div class='em'>{e}</div></div>\n";
const char IOTWEBCONF_HTML_FORM_OPTION[] PROGMEM =
  "<option value='{v}'{s}>{n}</option>\n";

typedef struct IotWebConfSerializationData
{
  byte* data;
  int length;
} IotWebConfSerializationData;

class IotWebConfConfigItem
{
public:
  boolean visible = true;
  const char* getId() { return this->_id; }
protected:
  IotWebConfConfigItem(const char* id) { this->_id = id; };
  /**
   * Calculate the size of bytes should be stored in the EEPROM.
   */
  virtual int getStorageSize();

  /**
   * On initial startup (when no data was saved), it may be required to apply a default value
   *   to the parameter.
   */
  virtual void applyDefaultValue();

  /**
   * Save data.
   * @doStore - A method is passed as a parameter, that will performs the actual EEPROM access.
   *   The argument 'serializationData' of this referenced method should be pre-filled with
   *   the size and the serialized data before calling the method.
   */
  virtual void storeValue(std::function<void(IotWebConfSerializationData* serializationData)> doStore);

  /**
   * Load data.
   * @doLoad - A method is passed as a parameter, that will performs the actual EEPROM access.
   *   The argument 'serializationData' of this referenced method should be pre-filled with
   *   the size of the expected data, and the data buffer should be allocated with this size.
   *   The doLoad will fill the data from the EEPROM.
   */
  virtual void loadValue(std::function<void(IotWebConfSerializationData* serializationData)> doLoad);

  /**
   * This method will create the HTML form item for the config portal.
   * 
   * @dataArrived - True if there was a form post, where (some) form
   *   data arrived from the client.
   * @server - The web server, that will send the rendered content to the client.
   *   The server->sendContent() method should be used in the implementations.
   */
  virtual void renderHtml(boolean dataArrived, WebServer* server);

  /**
   * New value arrived from the form post. The value should be stored in the
   *   in this config item.
   * 
   * @server - The web server, that will send the rendered content to the client.
   *   The server->hasArg() and server->arg() methods should be used in the
   *   implementations.
   */
  virtual void update(WebServer* server);

  /**
   * Before validating the form post, it is required to clear previos error messages.
   */
  virtual void clearErrorMessage();

  /**
   * This method should display information to Serial containing the parameter
   *   ID and the current value of the parameter (if it is confidential).
   *   Will only be called if debug is enabled.
   */
  virtual void debugTo(Stream* out);

  friend class IotWebConf; // Allow IotWebConf to access protected members.

private:
  const char* _id = 0;
  IotWebConfConfigItem* _parentItem = NULL;
  IotWebConfConfigItem* _nextItem = NULL;
  friend class IotWebConfParameterGroup; // Allow IotWebConfParameterGroup to access _nextItem.
};

class IotWebConfParameterGroup : public IotWebConfConfigItem
{
public:
  IotWebConfParameterGroup(const char* id, const char* label = NULL);
  void addItem(IotWebConfConfigItem* configItem);
  const char *label;

protected:
  int getStorageSize() override;
  void applyDefaultValue() override;
  void storeValue(std::function<void(IotWebConfSerializationData* serializationData)> doStore) override;
  void loadValue(std::function<void(IotWebConfSerializationData* serializationData)> doLoad) override;
  void renderHtml(boolean dataArrived, WebServer* server) override;
  void update(WebServer* server) override;
  void clearErrorMessage() override;
  void debugTo(Stream* out) override;
  friend class IotWebConf; // Allow IotWebConf to access protected members.

private:
  IotWebConfConfigItem* _firstItem = NULL;
};

/**
 * IotWebConfParameters is a configuration item of the config portal.
 * The parameter will have its input field on the configuration page,
 * and the provided value will be saved to the EEPROM.
 */
class IotWebConfParameter : public IotWebConfConfigItem
{
public:
  /**
   * Create a parameter for the config portal.
   *
   * @label - Displayable label at the config portal.
   * @id - Identifier used for HTTP queries and as configuration key. Must not
   *   contain spaces nor other special characters.
   * @valueBuffer - Configuration value will be loaded to this buffer from the
   *   EEPROM.
   * @length - The buffer should have a length provided here.
   * @defaultValue - Defalt value set on startup, when no configuration ever saved
   *   with the current config-version.
   */
  IotWebConfParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* defaultValue = NULL);

  const char* label;
  char* valueBuffer;
  const char* defaultValue;
  const char* errorMessage;

  int getLength() { return this->_length; }

protected:
  // Overrides
  int getStorageSize() override;
  void applyDefaultValue() override;
  void storeValue(std::function<void(IotWebConfSerializationData* serializationData)> doStore) override;
  void loadValue(std::function<void(IotWebConfSerializationData* serializationData)> doLoad) override;
  virtual void update(WebServer* server) override;
  virtual void update(String newValue);
  void clearErrorMessage() override;

private:
  int _length;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * IotWebConfTexParameters is to store text based parameters.
 */
class IotWebConfTextParameter : public IotWebConfParameter
{
public:
  /**
   * Create a parameter for the config portal.
   *
   * @placeholder (optional) - Text appear in an empty input box.
   * @customHtml (optional) - The text of this parameter will be added into
   *   the HTML INPUT field.
   * (see IotWebConfParameter for more arguments)
   */
  IotWebConfTextParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* defaultValue = NULL,
    const char* placeholder = NULL,
    const char* customHtml = NULL);

  /**
   * This variable is meant to store a value that is displayed in an empty
   *   (not filled) field.
   */
  const char* placeholder;

  /**
   * Usually this variable is used when rendering the form input field
   *   so one can customize the rendered outcome of this particular item.
   */
  const char* customHtml;

protected:
  virtual String renderHtml(
    boolean dataArrived, boolean hasValueFromPost, String valueFromPost);
  // Overrides
  virtual void renderHtml(boolean dataArrived, WebServer* server) override;
  virtual void update(String newValue) override;
  virtual void debugTo(Stream* out) override;

  /**
   * Renders a standard HTML form INPUT.
   * @type - The type attribute of the html input field.
   */
  virtual String renderHtml(const char* type, boolean hasValueFromPost, String valueFromPost);

private:
  friend class IotWebConf;
  friend class IotWebConfWifiParameterGroup;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * The Password parameter has a special handling, as the value will be
 * overwritten in the EEPROM only if value was provided on the config portal.
 * Because of this logic, "password" type field with length more then
 * IOTWEBCONF_PASSWORD_LEN characters are not supported.
 */
class IotWebConfPasswordParameter : public IotWebConfTextParameter
{
public:
  IotWebConfPasswordParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* defaultValue = NULL,
    const char* placeholder = NULL,
    const char* customHtml = "ondblclick=\"pw(this.id)\"");

protected:
  // Overrides
  virtual String renderHtml(
    boolean dataArrived, boolean hasValueFromPost, String valueFromPost) override;
  virtual void update(String newValue) override;
  virtual void debugTo(Stream* out) override;

private:
  friend class IotWebConf;
  friend class IotWebConfWifiParameterGroup;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * This is just a text parameter, that is rendered with type 'number'.
 */
class IotWebConfNumberParameter : public IotWebConfTextParameter
{
public:
  IotWebConfNumberParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* defaultValue = NULL,
    const char* placeholder = NULL,
    const char* customHtml = NULL);

protected:
  // Overrides
  virtual String renderHtml(
    boolean dataArrived, boolean hasValueFromPost, String valueFromPost) override;

private:
  friend class IotWebConf;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * Checkbox parameter is represended as a text parameter but has a special
 * handling. As the value is either empty or has the word "selected".
 * Note, that form post will not send value if checkbox was not selected.
 */
class IotWebConfCheckboxParameter : public IotWebConfTextParameter
{
public:
  IotWebConfCheckboxParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    boolean defaultValue = false);
  boolean isChecked() { return strncmp(this->valueBuffer, "selected", this->getLength()) == 0; }

protected:
  // Overrides
  virtual String renderHtml(
    boolean dataArrived, boolean hasValueFromPost, String valueFromPost) override;
  virtual void update(String newValue) override;

private:
  friend class IotWebConf;
  boolean _checked;
  const char* _checkedStr = "checked='checked'";
};

///////////////////////////////////////////////////////////////////////////////

/**
 * Options parameter is a structure, that handles multiply values when redering
 * the HTML representation.
 */
class IotWebConfOptionsParameter : public IotWebConfTextParameter
{
public:
  /**
   * @optionValues - List of values to choose from with, where each value
   *   can have a maximal size of 'length'. Contains 'optionCount' items.
   * @optionNames - List of names to render for the values, where each
   *   name can have a maximal size of 'nameLength'. Contains 'optionCount'
   *   items.
   * @optionCount - Size of both 'optionValues' and 'optionNames' lists.
   * @nameLength - Size of any item in optionNames list.
   */
  IotWebConfOptionsParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* optionValues, const char* optionNames, size_t optionCount, size_t nameLength,
    const char* defaultValue = NULL);

protected:
  const char* _optionValues;
  const char* _optionNames;
  size_t _optionCount;
  size_t _nameLength;

private:
  friend class IotWebConf;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * Select parameter is an option parameter, that rendered as HTML SELECT.
 */
class IotWebConfSelectParameter : public IotWebConfOptionsParameter
{
public:
  IotWebConfSelectParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    const char* optionValues, const char* optionNames, size_t optionCount, size_t namesLenth,
    const char* defaultValue = NULL);

protected:
  // Overrides
  virtual String renderHtml(
    boolean dataArrived, boolean hasValueFromPost, String valueFromPost) override;

private:
  friend class IotWebConf;
};

/**
 * This class is here just to make some nice indents on debug output
 *   for group tree.
 */
class PrefixStreamWrapper : public Stream
{
public:
  PrefixStreamWrapper(
    Stream* originalStream,
    std::function<size_t(Stream* stream)> prefixWriter);
  size_t write(uint8_t) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  int available() override { return _originalStream->available(); };
  int read() override { return _originalStream->read(); };
  int peek() override { return _originalStream->peek(); };
  void flush() override { return _originalStream->flush(); };

private:
  Stream* _originalStream;
  std::function<size_t(Stream* stream)> _prefixWriter;
  boolean _newLine = true;

  size_t checkNewLine();
};

#endif
