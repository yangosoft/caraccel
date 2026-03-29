#pragma once

#include <stdint.h>

enum class MeassureState : uint8_t { kInit, kV80, kV100, kV120, kSave };

struct Timmings {
  uint32_t v80_to_v100_ms;
  uint32_t v100_to_v120_ms;
};

class SMMeasure {
public:
  SMMeasure() : state(MeassureState::kInit) {}

  MeassureState get_state() const;

  MeassureState run(uint32_t speed_kph);

private:
  MeassureState state;
};
