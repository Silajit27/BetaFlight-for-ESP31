#ifndef _ESPFC_ESPFC_H_
#define _ESPFC_ESPFC_H_

#include "Model.h"
#include "Controller.h"
#include "Input.h"
#include "Actuator.h"
#include "SensorManager.h"
#include "SerialManager.h"
#include "Fusion.h"
#include "Mixer.h"
#include "Blackbox.h"
#include "Cli.h"
#include "Hardware.h"
#include "Buzzer.h"

namespace Espfc {

class Espfc
{
  public:
    Espfc():
      _hardware(_model), _controller(_model), _input(_model), _actuator(_model), _sensor(_model),
      _mixer(_model), _blackbox(_model), _telemetry(_model), _buzzer(_model), _serial(_model)
      {}

    int begin()
    {
      //Serial.setDebugOutput(true);
      _model.begin();
      _serial.begin();
      _hardware.begin();
      _model.update();
      _buzzer.begin();
      _sensor.begin();
      _input.begin();
      _actuator.begin();
      _controller.begin();
      _mixer.begin();
      _blackbox.begin();
      _model.state.buzzer.push(BEEPER_SYSTEM_INIT);

      return 1;
    }

    int update()
    {
      if(_sensor.update())
      {
        if(_model.state.loopTimer.syncTo(_model.state.gyroTimer))
        {
          _input.update();
          _actuator.update();
          _controller.update();
          if(_model.state.mixerTimer.syncTo(_model.state.loopTimer))
          {
            _mixer.update();
          }
          _blackbox.update();
        }
        _sensor.updateDelayed();
      }
      _hardware.updateStepper();
      return 1;
    }

    // other task
    int updateOther()
    {
      _model.state.stats.update();
      _buzzer.update();
      _serial.update();
      return 1;
    }

  private:
    Model _model;
    Hardware _hardware;
    Controller _controller;
    Input _input;
    Actuator _actuator;
    SensorManager _sensor;
    Mixer _mixer;
    Blackbox _blackbox;
    Telemetry _telemetry;
    Buzzer _buzzer;
    SerialManager _serial;
};

}

#endif
