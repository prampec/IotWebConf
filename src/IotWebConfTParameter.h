/**
 * IotWebConfTParameter.h -- IotWebConf is an ESP8266/ESP32
 *   non blocking WiFi/AP web configuration library for Arduino.
 *   https://github.com/prampec/IotWebConf
 *
 * Copyright (C) 2020 Balazs Kelemen <prampec+arduino@gmail.com>
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

template <typename ValueType, typename _DefaultValueType = ValueType>
class TParameter : public ConfigItem
{
public:
  using DefaultValueType = _DefaultValueType;

  TParameter(const char* id, const char* label, DefaultValueType defaultValue) :
    ConfigItem(id),
    _label(label),
    _defaultValue(defaultValue)
  {
  }

  ValueType& value() { return this->_value; }
  ValueType& operator*() { return this->_value; }

protected:
  const char* _label;
  ValueType _value;
  const DefaultValueType _defaultValue;

  virtual bool update(String newValue, bool validateOnly = false) = 0;
  bool validate(String newValue) { return update(newValue, true); }
  virtual String toString() { return String(this->_value); }
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
using TParameter<char[len], const char*>::TParameter;

protected:
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
      ParamType(this->_label, this->_id, this->_defaultValue));
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


} // end namespace

#endif
