#ifndef _ESC_DRIVER_ESP32_H_
#define _ESC_DRIVER_ESP32_H_

#if defined(ESP32) && !defined(ESP32C3)

#include "EscDriver.h"
#include <driver/rmt.h>

class EscDriverEsp32: public EscDriverBase
{
  public:
    class Slot {
      public:
        void inline setTerminate(int item, bool val)
        {
          items[item].val = val ? (1 << 15) : 0ul;
        }

        void inline setDshotBit(int item, bool val, bool inverted)
        {
          const uint32_t th = (val ? dshot_t1h : dshot_t0h) & DURATION_MAX;
          const uint32_t tl = (val ? dshot_t1l : dshot_t0l) & DURATION_MAX;
          if(!inverted)
          {
            items[item].val = (th | 1 << 15) | (tl << 16 | 0 << 31);
          }
          else
          {
            items[item].val = (th | 0 << 15) | (tl << 16 | 1 << 31);
          }
          //items[item].duration0 = th;
          //items[item].level0 = inverted ? 0 : 1;
          //items[item].duration1 = tl;
          //items[item].level1 = inverted ? 1 : 0;
        }

        void inline setDuration(int item, int duration, bool val)
        {
          if(duration >= 2)
          {
            int half = duration / 2;
            items[item].level0 = val;
            items[item].duration0 = half & DURATION_MAX;
            items[item].level1 = val;
            items[item].duration1 = (duration - half) & DURATION_MAX;
          }
          else
          {
            items[item].level0 = 0;
            items[item].duration0 = 1;
            items[item].level1 = 0;
            items[item].duration1 = 1;
          }
        }

        bool inline attached()
        {
          return (int)pin != -1;
        }

        static const size_t ITEM_COUNT = DSHOT_BIT_COUNT + 1;
        static const int32_t DURATION_MAX = 0x7fff; // max in 15 bits

        rmt_item32_t items[ITEM_COUNT];
        int pin;
        EscProtocol protocol;
        int32_t pulse_min;
        int32_t pulse_max;
        int32_t pulse_space;
        int32_t pulse;
        uint16_t divider;
        uint16_t dshot_t0h;
        uint16_t dshot_t0l;
        uint16_t dshot_t1h;
        uint16_t dshot_t1l;
        int telemetryValue;
    };

    EscDriverEsp32();
    int begin(EscProtocol protocol, bool async, int32_t rate, int timer = 0);
    void end();
    int attach(size_t channel, int pin, int pulse);
    int write(size_t channel, int pulse);
    void apply();
    int pin(size_t channel) const;

  private:
    void initChannel(int channel, gpio_num_t pin, int pulse);
    void modeTx(rmt_channel_t channel) IRAM_ATTR;
    void modeRx(rmt_channel_t channel) IRAM_ATTR;
    void enableTx(rmt_channel_t channel) IRAM_ATTR;
    void enableRx(rmt_channel_t channel) IRAM_ATTR;
    void disableTx(rmt_channel_t channel) IRAM_ATTR;
    void disableRx(rmt_channel_t channel) IRAM_ATTR;
    static void txDoneCallback(rmt_channel_t channel, void *arg) IRAM_ATTR;
    void transmitOne(uint8_t i) IRAM_ATTR;
    void transmitAll();
    void readTelemetry();
    void writeAnalogCommand(uint8_t channel, int32_t pulse) IRAM_ATTR;
    void writeDshotCommand(uint8_t channel, int32_t pulse) IRAM_ATTR;
    void transmitCommand(uint8_t channel) IRAM_ATTR;
    uint32_t getClockDivider() const;
    uint32_t getPulseMin() const;
    uint32_t getPulseMax() const;
    uint32_t getPulseInterval() const;
    uint32_t getAnalogPulse(int32_t ns) const;
    uint16_t getDshotPulse(uint32_t width) const;
    bool isDigital(EscProtocol protocol) const;

    Slot _channel[ESC_CHANNEL_COUNT];
    EscProtocol _protocol;
    int32_t _async;
    int32_t _rate;
    int32_t _interval;
    bool _digital;
    bool _invert;
    uint32_t _channel_mask;

    static bool _tx_end_installed;
    static EscDriverEsp32* instances[];
};

#endif

#endif
