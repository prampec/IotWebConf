/**
 * IotWebConfTParameter.h -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf
 *
 * Copyright (C) 2021 Balazs Kelemen <prampec+arduino@gmail.com>
 *                    rovo89
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifndef IotWebConfTParameter_h
#define IotWebConfTParameter_h

#include <IotWebConfParameter.h>
#include <Arduino.h>
#include <IPAddress.h>
#include <errno.h>

// At least in PlatformIO, strtoimax/strtoumax are defined, but not implemented.
#if 1
#define strtoimax strtoll
#define strtoumax strtoull
#endif

namespace iotwebconf
{

class ConfigItemBridge : public ConfigItem
{
public:
  virtual void update(WebRequestWrapper* webRequestWrapper) override
  {
      if (webRequestWrapper->hasArg(this->getId()))
      {
        String newValue = webRequestWrapper->arg(this->getId());
        this->update(newValue);
      }
  }
  void debugTo(Stream* out) override
  {
    out->print("'");
    out->print(this->getId());
    out->print("' with value: '");
    out->print(this->toString());
    out->println("'");
  }

protected:
  ConfigItemBridge(const char* id) : ConfigItem(id) { }
  virtual int getInputLength() { return 0; };
  virtual bool update(String newValue, bool validateOnly = false) = 0;
  virtual String toString() = 0;
};

///////////////////////////////////////////////////////////////////////////

template <typename ValueType, typename _DefaultValueType = ValueType>
class DataType : virtual public ConfigItemBridge
{
public:
  using DefaultValueType = _DefaultValueType;

  DataType(const char* id, DefaultValueType defaultValue) :
    ConfigItemBridge(id),
    _defaultValue(defaultValue)
  {
  }

  ValueType& value() { return this->_value; }
  ValueType& operator*() { return this->_value; }

protected:
  int getStorageSize() override
  {
    return sizeof(ValueType);
  }

  virtual bool update(String newValue, bool validateOnly = false) = 0;
  bool validate(String newValue) { return update(newValue, true); }
  virtual String toString() override { return String(this->_value); }

  ValueType _value;
  const DefaultValueType _defaultValue;
};

///////////////////////////////////////////////////////////////////////////

class StringDataType : public DataType<String>
{
public:
  using DataType<String>::DataType;

protected:
  virtual bool update(String newValue, bool validateOnly) override {
    if (!validateOnly)
    {
      this->_value = newValue;
    }
    return true;
  }
  virtual String toString() override { return this->_value; }
};

///////////////////////////////////////////////////////////////////////////

template <size_t len>
class CharArrayDataType : public DataType<char[len], const char*>
{
public:
using DataType<char[len], const char*>::DataType;
  CharArrayDataType(const char* id, const char* defaultValue) :
    ConfigItemBridge::ConfigItemBridge(id),
    DataType<char[len], const char*>::DataType(id, defaultValue) { };

protected:
  virtual void applyDefaultValue() override
  {
    strncpy(this->_value, this->_defaultValue, len);
  }
  virtual bool update(String newValue, bool validateOnly) override
  {
    if (newValue.length() + 1 > len)
    {
      return false;
    }
    if (!validateOnly)
    {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
      Serial.print(this->getId());
      Serial.print(": ");
      Serial.println(newValue);
#endif
      strncpy(this->_value, newValue.c_str(), len);
    }
    return true;
  }
  void storeValue(std::function<void(
    SerializationData* serializationData)> doStore) override
  {
    SerializationData serializationData;
    serializationData.length = len;
    serializationData.data = (byte*)this->_value;
    doStore(&serializationData);
  }
  void loadValue(std::function<void(
    SerializationData* serializationData)> doLoad) override
  {
    SerializationData serializationData;
    serializationData.length = len;
    serializationData.data = (byte*)this->_value;
    doLoad(&serializationData);
  }
  virtual int getInputLength() override { return len; };
};

///////////////////////////////////////////////////////////////////////////

template <typename ValueType>
class PrimitiveDataType : public DataType<ValueType>
{
public:
using DataType<ValueType>::DataType;
  PrimitiveDataType(const char* id, ValueType defaultValue) :
    ConfigItemBridge::ConfigItemBridge(id),
    DataType<ValueType>::DataType(id, defaultValue) { };

  void setMax(ValueType val) { this->_max = val; this->_maxDefined = true; }
  void setMin(ValueType val) { this->_min = val; this->_minDefined = true; }

protected:
  virtual void applyDefaultValue() override
  {
    this->_value = this->_defaultValue;
  }

  virtual bool update(String newValue, bool validateOnly) override
  {
    errno = 0;
    ValueType val = getValue(newValue);
    if ((errno == ERANGE)
      || (this->_minDefined && (val < this->_min))
      || (this->_maxDefined && (val > this->_max)))
    {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
      Serial.print(this->getId());
      Serial.print(" value not accepted: ");
      Serial.println(val);
#endif
      return false;
    }
    if (!validateOnly)
    {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
      Serial.print(this->getId());
      Serial.print(": ");
      Serial.println((ValueType)val);
#endif
      this->_value = (ValueType) val;
    }
    return true;
  }
  void storeValue(std::function<void(
    SerializationData* serializationData)> doStore) override
  {
    SerializationData serializationData;
    serializationData.length = this->getStorageSize();
    serializationData.data =
      reinterpret_cast<byte*>(&this->_value);
    doStore(&serializationData);
  }
  void loadValue(std::function<void(
    SerializationData* serializationData)> doLoad) override
  {
    byte buf[this->getStorageSize()];
    SerializationData serializationData;
    serializationData.length = this->getStorageSize();
    serializationData.data = buf;
    doLoad(&serializationData);
    ValueType* valuePointer = reinterpret_cast<ValueType*>(buf);
    this->_value = *valuePointer;
  }
  virtual ValueType getValue(String stringValue) = 0;

  ValueType getMax() { return this->_max; }
  ValueType getMin() { return this->_min; }
  ValueType isMaxDefined() { return this->_maxDefined; }
  ValueType isMinDefined() { return this->_minDefined; }

private:
  ValueType _min;
  ValueType _max;
  bool _minDefined = false;
  bool _maxDefined = false;
};

///////////////////////////////////////////////////////////////////////////

template <typename ValueType, int base = 10>
class SignedIntDataType : public PrimitiveDataType<ValueType>
{
public:
using DataType<ValueType>::DataType;
  SignedIntDataType(const char* id, ValueType defaultValue) :
    ConfigItemBridge::ConfigItemBridge(id),
    PrimitiveDataType<ValueType>::PrimitiveDataType(id, defaultValue) { };

protected:
  virtual ValueType getValue(String stringValue)
  {
    return (ValueType)strtoimax(stringValue.c_str(), NULL, base);
  }
};

template <typename ValueType, int base = 10>
class UnsignedIntDataType : public PrimitiveDataType<ValueType>
{
public:
using DataType<ValueType>::DataType;
  UnsignedIntDataType(const char* id, ValueType defaultValue) :
    ConfigItemBridge::ConfigItemBridge(id),
    PrimitiveDataType<ValueType>::PrimitiveDataType(id, defaultValue) { };

protected:
  virtual ValueType getValue(String stringValue)
  {
    return (ValueType)strtoumax(stringValue.c_str(), NULL, base);
  }
};

class FloatDataType : public PrimitiveDataType<float>
{
public:
using DataType<float>::DataType;
  FloatDataType(const char* id, float defaultValue) :
    ConfigItemBridge::ConfigItemBridge(id),
    PrimitiveDataType<float>::PrimitiveDataType(id, defaultValue) { };

protected:
  virtual float getValue(String stringValue)
  {
    return atof(stringValue.c_str());
  }
};

class DoubleDataType : public PrimitiveDataType<double>
{
public:
using DataType<double>::DataType;
  DoubleDataType(const char* id, double defaultValue) :
    ConfigItemBridge::ConfigItemBridge(id),
    PrimitiveDataType<double>::PrimitiveDataType(id, defaultValue) { };

protected:
  virtual double getValue(String stringValue)
  {
    return strtod(stringValue.c_str(), NULL);
  }
};

/////////////////////////////////////////////////////////////////////////

class IpDataType : public DataType<IPAddress>
{
using DataType<IPAddress>::DataType;

protected:
  virtual bool update(String newValue, bool validateOnly) override
  {
    if (validateOnly)
    {
      IPAddress ip;
      return ip.fromString(newValue);
    }
    else
    {
      return this->_value.fromString(newValue);
    }
  }

  virtual String toString() override { return this->_value.toString(); }
};

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

class InputParameter : virtual public ConfigItemBridge
{
public:
  InputParameter(const char* id, const char* label) :
    ConfigItemBridge::ConfigItemBridge(id),
    label(label) { }

  virtual void renderHtml(
    bool dataArrived, WebRequestWrapper* webRequestWrapper) override
  {
    String content = this->renderHtml(
      dataArrived,
      webRequestWrapper->hasArg(this->getId()),
      webRequestWrapper->arg(this->getId()));
    webRequestWrapper->sendContent(content);
  }

  const char* label;

  /**
   * This variable is meant to store a value that is displayed in an empty
   *   (not filled) field.
   */
  const char* placeholder = NULL;
  virtual void setPlaceholder(const char* placeholder) { this->placeholder = placeholder; }

  /**
   * Usually this variable is used when rendering the form input field
   *   so one can customize the rendered outcome of this particular item.
   */
  const char* customHtml = NULL;

  /**
   * Used when rendering the input field. Is is overriden by different
   *   implementations.
   */
  virtual String getCustomHtml() { return String(customHtml); }

  const char* errorMessage = NULL;

protected:
  void clearErrorMessage() override
  {
    this->errorMessage = NULL;
  }

  virtual String renderHtml(
    bool dataArrived, bool hasValueFromPost, String valueFromPost)
  {
    String pitem = String(this->getHtmlTemplate());

    pitem.replace("{b}", this->label);
    pitem.replace("{t}", this->getInputType());
    pitem.replace("{i}", this->getId());
    pitem.replace(
      "{p}", this->placeholder == NULL ? "" : this->placeholder);
    int length = this->getInputLength();
    if (length > 0)
    {
      char parLength[5];
      snprintf(parLength, 5, "%d", length);
      pitem.replace("{l}", parLength);
    }
    if (hasValueFromPost)
    {
      // -- Value from previous submit
      pitem.replace("{v}", valueFromPost);
    }
    else
    {
      // -- Value from config
      pitem.replace("{v}", this->toString());
    }
    pitem.replace(
        "{c}", this->customHtml == NULL ? "" : this->customHtml);
    pitem.replace(
        "{s}",
        this->errorMessage == NULL ? "" : "de"); // Div style class.
    pitem.replace(
        "{e}",
        this->errorMessage == NULL ? "" : this->errorMessage);

    return pitem;
  }

  /**
   * One can override this method in case a specific HTML template is required
   * for a parameter.
   */
  virtual String getHtmlTemplate() { return FPSTR(IOTWEBCONF_HTML_FORM_PARAM); };
  virtual const char* getInputType() = 0;
};

template <size_t len>
class TextTParameter : public CharArrayDataType<len>, public InputParameter
{
public:
using CharArrayDataType<len>::CharArrayDataType;
  TextTParameter(const char* id, const char* label, const char* defaultValue) :
    ConfigItemBridge(id),
    CharArrayDataType<len>::CharArrayDataType(id, defaultValue),
    InputParameter::InputParameter(id, label) { }

protected:
  virtual const char* getInputType() override { return "text"; }
};

template <size_t len>
class PasswordTParameter : public CharArrayDataType<len>, public InputParameter
{
public:
using CharArrayDataType<len>::CharArrayDataType;
  PasswordTParameter(const char* id, const char* label, const char* defaultValue) :
    ConfigItemBridge(id),
    CharArrayDataType<len>::CharArrayDataType(id, defaultValue),
    InputParameter::InputParameter(id, label)
  {
    this->customHtml = _customHtmlPwd;
  }

  void debugTo(Stream* out)
  {
    out->print("'");
    out->print(this->getId());
    out->print("' with value: ");
#ifdef IOTWEBCONF_DEBUG_PWD_TO_SERIAL
    out->print("'");
    out->print(this->_value);
    out->println("'");
#else
    out->println(F("<hidden>"));
#endif
  }

  virtual bool update(String newValue, bool validateOnly) override
  {
    if (newValue.length() + 1 > len)
    {
      return false;
    }
    if (validateOnly)
    {
      return true;
    }

#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
    Serial.print(this->getId());
    Serial.print(": ");
#endif
    if (newValue.length() > 0)
    {
      // -- Value was set.
      strncpy(this->_value, newValue.c_str(), len);
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
# ifdef IOTWEBCONF_DEBUG_PWD_TO_SERIAL
      Serial.println(this->_value);
# else
      Serial.println("<updated>");
# endif
#endif
    }
    else
    {
#ifdef IOTWEBCONF_DEBUG_TO_SERIAL
      Serial.println("<was not changed>");
#endif
    }
    return true;
  }

protected:
  virtual const char* getInputType() override { return "password"; }
  virtual String renderHtml(
    bool dataArrived, bool hasValueFromPost, String valueFromPost) override
  {
    return InputParameter::renderHtml(dataArrived, true, String(""));
  }
private:
  const char* _customHtmlPwd = "ondblclick=\"pw(this.id)\"";
};

// TODO: extract primitive type getters to an InputParameter extension.
template <typename ValueType, int base = 10>
class IntTParameter :
  public SignedIntDataType<ValueType, base>,
  public InputParameter
{
public:
  IntTParameter(const char* id, const char* label, ValueType defaultValue) :
    ConfigItemBridge(id),
    SignedIntDataType<ValueType, base>::SignedIntDataType(id, defaultValue),
    InputParameter::InputParameter(id, label) { }

  virtual String getCustomHtml() override
  {
    String modifiers = String(this->customHtml);

    if (this->isMinDefined())
    {
      modifiers += " min='" ;
      modifiers += this->getMin();
      modifiers += "'";
    }
    if (this->isMaxDefined())
    {
      modifiers += " max='";
      modifiers += this->getMax();
      modifiers += "'";
    }
    if (this->step != 0)
    {
      modifiers += " step='";
      modifiers += this->step;
      modifiers += "'";
    }

    return modifiers;
  }

  ValueType step = 0;
  void setStep(ValueType step) { this->step = step; }

protected:
  virtual const char* getInputType() override { return "number"; }
};

class FloatTParameter :
  public FloatDataType,
  public InputParameter
{
public:
  FloatTParameter(const char* id, const char* label, float defaultValue) :
    ConfigItemBridge(id),
    FloatDataType::FloatDataType(id, defaultValue),
    InputParameter::InputParameter(id, label) { }

protected:
  virtual const char* getInputType() override { return "number"; }
};

} // end namespace

#include <IotWebConfTParameterBuilder.h>

#endif
