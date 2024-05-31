#pragma once

#include <array>
#include <functional>
#include <stdint.h>

namespace synthux {

static constexpr float kBPMMin = 40;
static constexpr float kBPMRange = 200;

inline static bool fcomp(const float lhs, const float rhs, const int precision = 2) {
    auto digits = precision * 10;
    auto lhs_int = static_cast<int32_t>(roundf(lhs * digits));
    auto rhs_int = static_cast<int32_t>(roundf(rhs * digits));
    return lhs_int == rhs_int;
}

template<size_t ppqn>
class Clock {
public:
    Clock():
      _on_tick             { nullptr },
      _is_running          { false },
      _is_about_to_run     { false },
      _tr_time             { 0 },
      _ticks_per_clock     { ppqn / 4 },
      _ticks               { 0 },
      _fticks              { 0 },
      _ticks_at_last_clock { 0 },
      _tempo_ticks         { 0 },
      _hold                { false },
      _resync              { false },
      _manual_tempo        { 120 },
      _raw_manual_tempo    { 120 },
      _tempo_mks           { 500000 },
      _last_state          { 1 } 
      {}
    
    ~Clock() = default;

    void Init(float sample_rate, float buffer_size) {
        auto interval = 1e6 * buffer_size / sample_rate;
        _tr_time = ppqn * interval;
    }

    // Called by internal interrupt timer (audio callback in this implementation).
    void Tick() {
        if (!_is_running) return;
        emit_ticks();
    }

    void SetOnTick(std::function<void()> on_tick) {
      _on_tick = on_tick;
    }

    /*
    Read external clock pin
    */
    void Process(bool state) {
        if (state && !_last_state) {
            external_clock_tick();
        }
        _last_state = state;
    }
    
    float Tempo() { return 60000000.f / _tempo_mks; }
    /*
    Setting tempo from internal control. 
    Has no effect in case of syncing to extrnal clock.
    */
    void SetTempo(float norm_value) {
        if (fcomp(norm_value, _raw_manual_tempo)) return;
        _raw_manual_tempo = norm_value;
        const auto clock_off_offset = 10;
        _manual_tempo = (kBPMRange - clock_off_offset) * norm_value + kBPMMin - clock_off_offset;
        _tempo_mks = tempo_mks(_manual_tempo);
        if (external_clock()) {
            if (_is_running) {
                _is_running = false;
                _is_about_to_run = true;
            }
            else if (_is_about_to_run) {
                _is_running = true;
                _is_about_to_run = false;
            }
            reset();
        }
    }

    /*
    In case of external clock sync this
    method only schedules playback. Actual playback 
    starts on the first tick of the external clock.
    see clock_in_tick() below.
    */
    void Run() {
      if (external_clock()) _is_about_to_run = true;
      else _is_running = true;
    }

    void Stop() {
      _is_running = false;
      reset();
    }
    bool IsRunning() { return _is_running; };

private:
    bool external_clock() { return _manual_tempo < kBPMMin; }
    /*
    External clock received
    This method starts playback on first received clock
    after playback was scheduled. After that, calls sync 
    for every tick received.
    */
    void external_clock_tick() {
        if (!external_clock()) return;
        if (!_is_running && !_is_about_to_run) return;

        if (_is_about_to_run) {
            _is_about_to_run = false;
            _is_running = true;
        }
        else {
            _resync = true;
            _hold = false;
            emit_ticks();
        }
    }

    /*
    Derived from Maximum MIDI Programmer's ToolKit Copyright ©1993-1998 by Paul Messick.
    This method generates internal ticks and also synchronises to the external clock.
    So it's called both from internal interrupt timer (audio callback in this implementation) and upon external clock tick reception.
    nticks - integer count of internal ticks
    _fticks - fractional count of internal ticks
    _tempo_ticks - integer ticks count since last external clock
    _tempo_mks - tempo in microseconds / beat (quarter note)
    _resync - flag to resync to external clock. Is set to true once external clock tick is received.
    _hold - flag to stop advancing internal timeline if the number of internal ticks exceeded expected count of internal ticks per extrnal tick
    _tr_time - internal resolution (ppqn) multiplied by interrupt interval.
    */
    void emit_ticks() {
        uint32_t nticks = 0;

        //If we generated more internal ticks per extrnal tick as expected,
        //we don't advance internal "timeline", but only accumulate _tempo_ticks
        //in order to calculate and correct the tempo.
        //This flag is set to false upon reception of the external tick.
        if (_hold) {
            nticks = (_fticks + _tr_time) / _tempo_mks;
            _fticks += _tr_time - (nticks * _tempo_mks);
            _tempo_ticks += nticks;
            return;
        }

        //Once a tick of the extrnal clock is received,
        //we do resync, i.e. align inernal timeline with the external one
        //and adjust tempo.
        if (_resync) {
            _fticks = 0;
            nticks = _ticks_per_clock - (_ticks - _ticks_at_last_clock);
            _ticks_at_last_clock = _ticks + nticks;
            _tempo_mks -= ((int32_t)_ticks_per_clock - (int32_t)_tempo_ticks) * (int32_t)_tempo_mks / (int32_t)ppqn;
            _tempo_ticks = 0;
            _resync = false;
        }
        //Regular mode. We generate internal ticks.
        else {
            nticks = (_fticks + _tr_time) / _tempo_mks;
            _fticks += _tr_time - nticks * _tempo_mks;
            if (external_clock()) {
                _tempo_ticks += nticks;
                //If there are more internal ticks per the external tick than 
                //expected, we set _hold to true effectively stopping advancing timeline
                //until next external tick
                if (_ticks - _ticks_at_last_clock + nticks >= _ticks_per_clock) {
                    nticks = _ticks_per_clock - 1 - (_ticks - _ticks_at_last_clock);
                    _hold = true;
                }
            }
        }

        //Accumulate ticks
        _ticks += nticks;

        //Advance timeline
        if (_on_tick != nullptr) {
          for (uint32_t i = 0; i < nticks; i++) _on_tick();
        }
    }
    
    void reset() {
         _fticks = 0;
        _ticks = 0;
        _ticks_at_last_clock = 0;
        _tempo_ticks = 0;
        _hold = false;
        _resync = false;
    }
    
    uint32_t tempo_mks(const float tempo) {
        return static_cast<uint32_t>(60.f * 1e6 / tempo);
    }

    std::function<void()> _on_tick;

    bool _is_running;
    bool _is_about_to_run;

    uint32_t _tr_time;
    uint32_t _ticks_per_clock;
    uint32_t _ticks;
    uint32_t _fticks;
    uint32_t _ticks_at_last_clock;
    uint32_t _tempo_ticks;
    bool _hold;
    bool _resync;

    float _manual_tempo;
    float _raw_manual_tempo;
    uint32_t _tempo_mks;

    int _last_state;
};

};
