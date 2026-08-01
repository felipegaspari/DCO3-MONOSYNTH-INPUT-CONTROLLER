#ifndef __FS_H__
#define __FS_H__

  #include <stdint.h>
  #include "LittleFS.h"

  static constexpr uint16_t NUM_PRESETS             = 256;
  static constexpr uint16_t flashPresetSize         = 180;
  static constexpr uint16_t LEGACY_FLASH_PRESET_SIZE = 140;  // pre–format-v1 slots
  static constexpr uint8_t  PRESET_FORMAT_VERSION   = 1;
  static constexpr uint32_t flashBankSize =
      (uint32_t)NUM_PRESETS * (uint32_t)flashPresetSize;
  static constexpr uint32_t LEGACY_FLASH_BANK_SIZE =
      (uint32_t)NUM_PRESETS * (uint32_t)LEGACY_FLASH_PRESET_SIZE;

byte flashData[flashPresetSize];
byte presetBank1Buffer[flashBankSize];

File fileBank1;

#endif
