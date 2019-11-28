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
  byte* valueBuffer;
  int length;
} IotWebConfSerializationData;


/**
 * IotWebConfParameters is a configuration item of the config portal.
 * The parameter will have its input field on the configuration page,
 * and the provided value will be saved to the EEPROM.
 */
class IotWebConfParameter
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
   * @visible - Should be offered to be modified by the user.
   *   NOTE: Might be removed in the future!
   * @defaultValue - Defalt value set on startup, when no configuration ever saved
   *   with the current config-version.
   */
  IotWebConfParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    boolean visible = true, const char* defaultValue = NULL);

  /**
   * This method will create the html form item for the config portal.
   * 
   * @dataArrived - True if there was a form post, where (some) form
   *   data arrived from the client.
   * @hasValueFromPost - Parameter is present in the form post.
   * @valueFromPost - Value of the form parameter being posted,
   *   String.emptyString if no value was present.
   * @returns - The rendered HTML form-segment.
   */
  virtual String renderHtml(boolean dataArrived, boolean hasValueFromPost, String valueFromPost);

  /**
   * New value arrived from the form post. Value should be stored in the
   *   valueBuffer to be saved to EEPROM.
   */
  virtual void update(String newValue);

  /**
   * Serialization is done before saving data to EEPROM.
   * If the valueBuffer needs some processing before save, it can be done
   *   here. E.g. you might want to store a numeric value in binary format.
   * 
   * @returns - Returns with a serialized data stored in the corresponding
   *   structure.
   */
  virtual IotWebConfSerializationData serialize();

  /**
   * By default parameter is stored in the valueBuffer when read from the
   *   EEPROM. In this method one can specify a different storing
   *   point read from the EEPROM.
   * 
   * @returns - Prepared store for reading EEPROM value into.
   */
  virtual IotWebConfSerializationData prepareDeserialization();

  /**
   * You might want to post-process the value read from the EEPROM to be
   *   human readable. E.g. if a numeric value is stored in binary format
   *   it is time to convert it to text.
   * 
   * @serializationData - Data store prepared by the prepareDeserialization.
   */
  virtual void deserialize(IotWebConfSerializationData serializationData);

  /**
   * This method should display information to Serial containing the parameter
   *   ID and the current value of the parameter (if it is confidential).
   *   Will only be called if debug is enabled.
   */
  virtual void debugTo(Stream* out);

  const char* label;
  char* valueBuffer;
  boolean visible;
  const char* defaultValue;
  const char* errorMessage;

  const char* getId() { return this->_id; }
  int getLength() { return this->_length; }

protected:
  IotWebConfParameter() {};

private:
  const char* _id = 0;
  int _length;
  IotWebConfParameter* _nextParameter = NULL;
  friend class IotWebConf; // Allow IotWebConf to access private members.
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
    boolean visible = true, const char* defaultValue = NULL,
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

  // Overrides
  virtual String renderHtml(
    boolean dataArrived, boolean hasValueFromPost, String valueFromPost) override;
  virtual void update(String newValue) override;
  virtual IotWebConfSerializationData serialize() override;
  virtual IotWebConfSerializationData prepareDeserialization() override;
  virtual void deserialize(IotWebConfSerializationData serializationData) override;
  virtual void debugTo(Stream* out) override;

protected:
  /**
   * Renders a standard HTML form INPUT.
   * @type - The type attribute of the html input field.
   */
  virtual String renderHtml(const char* type, boolean hasValueFromPost, String valueFromPost);
  IotWebConfTextParameter() {};

private:
  friend class IotWebConf;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * The Password parameter has a special handling, as the value will be
 * overwritten in the EEPROM only if value was provided on the config portal.
 * Because of this logic, "password" type field with length more then
 * IOTWEBCONF_WORD_LEN characters are not supported.
 */
class IotWebConfPasswordParameter : public IotWebConfTextParameter
{
public:
  IotWebConfPasswordParameter(
    const char* label, const char* id, char* valueBuffer, int length,
    boolean visible = true, const char* defaultValue = NULL,
    const char* placeholder = NULL,
    const char* customHtml = "ondblclick=\"pw(this.id)\"");

  // Overrides
  virtual String renderHtml(boolean dataArrived, boolean hasValueFromPost, String valueFromPost) override;
  virtual void update(String newValue) override;
  virtual void debugTo(Stream* out) override;

private:
  IotWebConfPasswordParameter() {};
  friend class IotWebConf;
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
    boolean visible = true, const char* defaultValue = NULL,
    const char* placeholder = NULL,
    const char* customHtml = NULL);

  // Overrides
  virtual String renderHtml(boolean dataArrived, boolean hasValueFromPost, String valueFromPost) override;

private:
  IotWebConfNumberParameter() {};
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
    boolean visible = true, boolean defaultValue = false);

  // Overrides
  virtual String renderHtml(boolean dataArrived, boolean hasValueFromPost, String valueFromPost) override;
  virtual void update(String newValue) override;

  boolean isChecked() { return strncmp(this->valueBuffer, "selected", this->getLength()) == 0; }

private:
  IotWebConfCheckboxParameter() {};
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
    boolean visible = true, const char* defaultValue = NULL);

protected:
  IotWebConfOptionsParameter() {};
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
    boolean visible = true, const char* defaultValue = NULL);

  // Overrides
  virtual String renderHtml(
    boolean dataArrived, boolean hasValueFromPost, String valueFromPost) override;

private:
  IotWebConfSelectParameter() {};
  friend class IotWebConf;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * A separator for separating field sets.
 * NOTE: Separator will be replaced by a grouping ability of the parameters.
 */
class IotWebConfSeparator : public IotWebConfParameter
{
public:
  IotWebConfSeparator();

  /**
   * Create a seperator with a label (legend tag)
   */
  IotWebConfSeparator(const char* label);

  virtual String renderHtml(
    boolean dataArrived, boolean hasValueFromPost, String valueFromPost) override;
  virtual void update(String newValue) override;
  virtual IotWebConfSerializationData serialize() override;
  virtual IotWebConfSerializationData prepareDeserialization() override;
  virtual void deserialize(IotWebConfSerializationData serializationData) override;
  virtual void debugTo(Stream* out) override;
};

#endif
