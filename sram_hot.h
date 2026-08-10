#ifndef __SRAM_HOT_H__
#define __SRAM_HOT_H__

// RP2040: Pico SDK / Arduino-Pico already define __not_in_flash_func.
// Fallback keeps non-Pico hosts (linter, AVR) compiling.
#ifndef __not_in_flash_func
#define __not_in_flash_func(fn) fn
#endif

#ifndef INPUT_ALWAYS_INLINE
#define INPUT_ALWAYS_INLINE __attribute__((always_inline))
#endif

#ifndef ROXMUX_FELA_SRAM_HOT
#define ROXMUX_FELA_SRAM_HOT 1
#endif
#ifndef MD_RENCODER_FELA_SRAM_HOT
#define MD_RENCODER_FELA_SRAM_HOT 1
#endif
#ifndef CD74HC4067_FELA_SRAM_HOT
#define CD74HC4067_FELA_SRAM_HOT 1
#endif

#endif
