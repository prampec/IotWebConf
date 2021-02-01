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
#define intmax_t long long
#define uintmax_t unsigned long long
#define strtoimax strtoll
#define strtoumax strtoull
#endif

namespace iotwebconf
{

class ConfigItemBridge : public ConfigItem
{
protected:
  ConfigItemBridge(const char* id) : ConfigItem(id) { }
  virtual String toString();
  virtual int getInputLength() { return 0; };
  virtual const char* getLabel();
};

template <typename ValueType, typename _DefaultValueType = ValueType>
class TParameter : virtual public ConfigItemBridge
{
public:
  using DefaultValueType = _DefaultValueType;

  TParameter(const char* id, const char* label, DefaultValueType defaultValue) :
    ConfigItemBridge(id),
    _label(label),
    _defaultValue(defaultValue)
  {
  }

  ValueType& value() { return this->_value; }
  ValueType& operator*() { return this->_value; }

protected:
//  void storeValue(std::function<void(SerializationData* serializationData)> doStore) override;
//  void loadValue(std::function<void(SerializationData* serializationData)> doLoad) override;

  int getStorageSize() override
  {
    return sizeof(ValueType);
  }
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

  virtual bool update(String newValue, bool validateOnly = false) = 0;
  bool validate(String newValue) { return update(newValue, true); }
  virtual String toString() override { return String(this->_value); }
  virtual const char* getLabel() override { return this->_label; }

  const char* _label;
  ValueType _value;
  const DefaultValueType _defaultValue;
};

///////////////////////////////////////////////////////////////////////////

class InputParameter : virtual public ConfigItemBridge
{
public:
  InputParameter(const char* id) : ConfigItemBridge::ConfigItemBridge(id) { }
  virtual void renderHtml(
    bool dataArrived, WebRequestWrapper* webRequestWrapper) override
  {
    String content = this->renderHtml(
      dataArrived,
      webRequestWrapper->hasArg(this->getId()),
      webRequestWrapper->arg(this->getId()));
    webRequestWrapper->sendContent(content);
  }

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

  const char* errorMessage;

protected:
  void clearErrorMessage() override
  {
    this->errorMessage = NULL;
  }

  virtual String renderHtml(
    bool dataArrived, bool hasValueFromPost, String valueFromPost)
  {
    String pitem = String(this->getHtmlTemplate());

    pitem.replace("{b}", this->getLabel());
    pitem.replace("{t}", this->getInputType());
    pitem.replace("{i}", this->getId());
    pitem.replace(
      "{p}", this->placeholder == NULL ? "" : this->placeholder);
    int length = this->getInputLength();
    if (length >= 0)
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
  virtual const char* getInputType();
};

///////////////////////////////////////////////////////////////////////////

class StringTParameter : public TParameter<String>
{
public:
  using TParameter<String>::TParameter;

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
class CharArrayTParameter : public TParameter<char[len], const char*>
{
public:
using TParameter<char[len], const char*>::TParameter;
  CharArrayTParameter(const char* id, const char* label, const char* defaultValue) :
    TParameter<char[len], const char*>::TParameter(id, label, defaultValue),
    ConfigItemBridge::ConfigItemBridge(id) { };

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
      strcpy(this->_value, newValue.c_str());
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

template <typename ValueType, int base = 10>
class SignedIntTParameter : public TParameter<ValueType>
{
using TParameter<ValueType>::TParameter;

protected:
  intmax_t min = std::numeric_limits<ValueType>::min();
  intmax_t max = std::numeric_limits<ValueType>::max();

  virtual bool update(String newValue, bool validateOnly) override
  {
    char *end;
    errno = 0;
    intmax_t val = strtoimax(newValue.c_str(), &end, base);
    if (errno == ERANGE || end == newValue.end() || *end != '\0'
      || val < this->min  || val > this->max)
    {
      return false;
    }
    if (!validateOnly)
    {
      this->_value = (ValueType) val;
    }
    return true;
  }
};

///////////////////////////////////////////////////////////////////////////

template <typename ValueType, int base = 10>
class UnsignedIntTParameter : public TParameter<ValueType>
{
using TParameter<ValueType>::TParameter;

protected:
  uintmax_t min = std::numeric_limits<ValueType>::min();
  uintmax_t max = std::numeric_limits<ValueType>::max();

  virtual bool update(String newValue, bool validateOnly) override
  {
    char *end;
    errno = 0;
    intmax_t val = strtoumax(newValue.c_str(), &end, base);
    if (errno == ERANGE || end == newValue.end() || *end != '\0'
      || val < this->min  || val > this->max)
    {
      return false;
    }
    if (!validateOnly)
    {
      this->_value = (ValueType) val;
    }
    return true;
  }
};

/////////////////////////////////////////////////////////////////////////

class IpTParameter : public TParameter<IPAddress>
{
using TParameter<IPAddress>::TParameter;

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

template <typename ParamType> class Builder;

template <typename ParamType>
class AbstractBuilder
{
public:
  AbstractBuilder(const char* id) : _id(id) { };
  virtual ParamType build() const
  {
    return std::move(
      ParamType(this->_id, this->_label, this->_defaultValue));
  }

  Builder<ParamType>& label(const char* label)
    { this->_label = label; return static_cast<Builder<ParamType>&>(*this); }
  Builder<ParamType>& defaultValue(typename ParamType::DefaultValueType defaultValue)
    { this->_defaultValue = defaultValue; return static_cast<Builder<ParamType>&>(*this); }

protected:
  const char* _label;
  const char* _id;
  typename ParamType::DefaultValueType _defaultValue;
};

///////////////////////////////////////////////////////////////////////////

template <typename ParamType>
class Builder : public AbstractBuilder<ParamType>
{
public:
  Builder(const char* id) : AbstractBuilder<ParamType>(id) { };
};

template <typename ValueType, int base>
class Builder<SignedIntTParameter<ValueType, base>>
  : public AbstractBuilder<SignedIntTParameter<ValueType, base>>
{
public:
  Builder<SignedIntTParameter<ValueType, base>>(const char* id) :
    AbstractBuilder<SignedIntTParameter<ValueType, base>>(id) { };
  virtual SignedIntTParameter<ValueType, base> build() const override
  {
    return
      std::move(SignedIntTParameter<ValueType, base>(
        this->_label, this->_id, this->_defaultValue));
  }

  Builder& min(uintmax_t min) { this->_min = min; return *this; }
  Builder& max(uintmax_t max) { this->_max = max; return *this; }

protected:
  uintmax_t _min;
  uintmax_t _max;
};

///////////////////////////////////////////////////////////////////////////

template <size_t len>
class TextTParameter : public CharArrayTParameter<len>, public InputParameter
{
public:
using CharArrayTParameter<len>::CharArrayTParameter;
  TextTParameter(const char* id, const char* label, const char* defaultValue) :
    CharArrayTParameter<len>::CharArrayTParameter(id, label, defaultValue),
    InputParameter::InputParameter(id), ConfigItemBridge(id) { }

protected:
  virtual const char* getInputType() override { return "text"; }
};

} // end namespace

#endif
