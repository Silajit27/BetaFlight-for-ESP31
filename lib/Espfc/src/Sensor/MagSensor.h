#ifndef _ESPFC_SENSOR_MAG_SENSOR_H_
#define _ESPFC_SENSOR_MAG_SENSOR_H_

#include "BaseSensor.h"
#include "Device/MagDevice.h"

#include <math.h>

namespace Espfc {

namespace Sensor {

class MagSensor: public BaseSensor
{
  public:
    MagSensor(Model& model): _model(model) {}

    int begin()
    {
      if(!_model.magActive()) return 0;

      _mag = Hardware::getMagDevice(_model);
      if(!_mag) return 0;

      if(_model.state.magTimer.rate < 5) return 0;

      // by default use eeprom calibration settings
      _model.state.magCalibrationState = MAG_CALIBRATION_IDLE;
      _model.state.magCalibrationValid = true;

      _model.logger.info().log(F("MAG INIT")).log(FPSTR(Device::MagDevice::getName(_mag->getType()))).logln(_model.state.magTimer.rate);
      
      return 1;
    }

    int update()
    {
      if(!_model.magActive() || !_model.state.magTimer.check()) return 0;
      if(!_mag) return 0;

      {
        Stats::Measure measure(_model.state.stats, COUNTER_MAG_READ);
        _mag->readMag(_model.state.magRaw);
      }

      {      
        Stats::Measure measure(_model.state.stats, COUNTER_MAG_FILTER);

        align(_model.state.magRaw, _model.config.magAlign);
        _model.state.mag = _mag->convert(_model.state.magRaw);
        if(_model.state.magCalibrationValid && _model.state.magCalibrationState == MAG_CALIBRATION_IDLE)
        {
          _model.state.mag -= _model.state.magCalibrationOffset;
          _model.state.mag *= _model.state.magCalibrationScale;
        }
        for(size_t i = 0; i < 3; i++)
        {
          _model.state.mag.set(i, _model.state.magFilter[i].update(_model.state.mag[i]));
        }
      }
      magCalibration();

      return 1;
    }

    void magCalibration()
    {
      switch(_model.state.magCalibrationState)
      {
        case MAG_CALIBRATION_RESET:
          resetMagCalibration();
          _model.state.magCalibrationSamples = 30 * _model.state.magTimer.rate;
          _model.state.magCalibrationState = MAG_CALIBRATION_UPDATE;
          break;
        case MAG_CALIBRATION_UPDATE:
          updateMagCalibration();
          _model.state.magCalibrationSamples--;
          if(_model.state.magCalibrationSamples <= 0) _model.state.magCalibrationState = MAG_CALIBRATION_APPLY;
          break;
        case MAG_CALIBRATION_APPLY:
          applyMagCalibration();
          _model.state.magCalibrationState = MAG_CALIBRATION_SAVE;
          break;
        case MAG_CALIBRATION_SAVE: // @see Model::finishCallibration()
        case MAG_CALIBRATION_IDLE:
        default:
          return;
      }
    }

    void resetMagCalibration()
    {
      _model.state.magCalibrationMin = VectorFloat();
      _model.state.magCalibrationMax = VectorFloat();
      _model.state.magCalibrationValid = false;
    }

    void updateMagCalibration()
    {
      for(int i = 0; i < 3; i++)
      {
        if(_model.state.mag[i] < _model.state.magCalibrationMin[i]) _model.state.magCalibrationMin.set(i, _model.state.mag[i]);
        if(_model.state.mag[i] > _model.state.magCalibrationMax[i]) _model.state.magCalibrationMax.set(i, _model.state.mag[i]);
      }
    }

    void applyMagCalibration()
    {
      const float EPSILON = 0.001f;

      // verify calibration data and find biggest range
      float maxRange = -1;
      for(int i = 0; i < 3; i++)
      {
        if(_model.state.magCalibrationMin[i] > -EPSILON) return;
        if(_model.state.magCalibrationMax[i] <  EPSILON) return;
        if((_model.state.magCalibrationMax[i] - _model.state.magCalibrationMin[i]) > maxRange)
        {
          maxRange = _model.state.magCalibrationMax[i] - _model.state.magCalibrationMin[i];
        }
      }

      // probably incomplete data, must be positive
      if(maxRange <= 0) return;

      VectorFloat scale(1.f, 1.f, 1.f);
      VectorFloat offset(0.f, 0.f, 0.f);

      for (int i = 0; i < 3; i++)
      {
        const float range = (_model.state.magCalibrationMax[i] - _model.state.magCalibrationMin[i]);
        const float bias  = (_model.state.magCalibrationMax[i] + _model.state.magCalibrationMin[i]) * 0.5f;

        if(abs(range) < EPSILON) return;    // incomplete data
        
        scale.set(i, maxRange / range);     // makes everything the same range
        offset.set(i, bias);                // level bias
      }

      _model.state.magCalibrationScale = scale;
      _model.state.magCalibrationOffset = offset;
      _model.state.magCalibrationValid = true;
    }

  private:
    Model& _model;
    Device::MagDevice * _mag;
};

}

}
#endif