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

#ifndef IotWebConfTParameterBuilder_h
#define IotWebConfTParameterBuilder_h

#include <IotWebConfTParameter.h>

namespace iotwebconf
{

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

template <typename ParamType>
class Builder : public AbstractBuilder<ParamType>
{
public:
  Builder(const char* id) : AbstractBuilder<ParamType>(id) { };
};

///////////////////////////////////////////////////////////////////////////

template <typename ValueType, int base>
class Builder<IntTParameter<ValueType, base>> :
  public AbstractBuilder<IntTParameter<ValueType, base>>
{
public:
  Builder<IntTParameter<ValueType, base>>(const char* id) :
    AbstractBuilder<IntTParameter<ValueType, base>>(id) { };
  virtual IntTParameter<ValueType, base> build() const override
  {
    IntTParameter<ValueType, base> instance =
      std::move(IntTParameter<ValueType, base>(
        this->_label, this->_id, this->_defaultValue));
    this->apply(&instance);
    return instance;
  }
  Builder& min(ValueType min) { this->_minDefined = true; this->_min = min; return *this; }
  Builder& max(ValueType max) { this->_maxDefined = true; this->_max = max; return *this; }
  Builder& step(ValueType step) { this->_step = step; return *this; }
  Builder& placeholder(const char* placeholder) { this->_placeholder = placeholder; return *this; }

protected:
  virtual IntTParameter<ValueType, base>* apply(
    IntTParameter<ValueType, base>* instance) const
  {
    if (this->_minDefined)
    {
      instance->setMin(this->_min);
    }
    if (this->_maxDefined)
    {
      instance->setMax(this->_max);
    }
    instance->setStep(this->_step);
    instance->setPlaceholder(this->_placeholder);
    return instance;
  }

  bool _minDefined = false;
  bool _maxDefined = false;
  ValueType _min;
  ValueType _max;
  ValueType _step = 0;
  const char* _placeholder = NULL;
};

} // End namespace

#endif
