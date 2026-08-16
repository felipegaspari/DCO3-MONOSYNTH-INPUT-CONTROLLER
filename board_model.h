#ifndef BOARD_MODEL_H
#define BOARD_MODEL_H

#include <stdint.h>
#include "params_def.h"
#include "_build_libs/DCO-PROTOCOL/params_def.h"

// -----------------------------------------------------------------------------
// Which instrument this Input controller build targets.
//
// The same panel firmware runs the front panel of both synths. Everything that
// differs between them is derived here, so every other file in this sketch is
// byte-identical in the DCO3-MONOSYNTH and DCO4-REBORN trees — and so is this
// one. The instrument itself is not chosen here: it comes from the superproject
// this checkout sits in, through project_config.h.
//
//   DCO3-MONOSYNTH  1 voice, 3 oscillators + sub. Input is wired straight to the
//                   DCO, so preset/calibration frames make a single hop.
//   DCO4-REBORN     4 voices of 2 oscillators. Input is wired to the STM32
//                   Mainboard, which relays those frames on to the DCO.
//
// Both models share one preset store: the DCO's 256 slots (DCO/preset_store.h).
// Input keeps only a RAM name cache, so nothing here selects a storage backend.
// -----------------------------------------------------------------------------

#define INPUT_BOARD_DCO3 3
#define INPUT_BOARD_DCO4 4

// project_config.h is a symlink to the superproject root, so it is the same
// committed file in both trees and resolves to a different instrument in each.
#if __has_include("project_config.h")
#include "project_config.h"
#endif

// A -D on the build line still wins, to compile-check the other instrument from
// this tree. Missing config is an error rather than a default: a silent fallback
// here is what flashes monosynth firmware onto a 4-voice panel.
#ifndef INPUT_BOARD_MODEL
#  ifdef PROJECT_INSTRUMENT
#    define INPUT_BOARD_MODEL PROJECT_INSTRUMENT
#  else
#    error "no project_config.h - this sketch must sit in a DCO superproject root (see README), or pass -DINPUT_BOARD_MODEL"
#  endif
#endif

#if INPUT_BOARD_MODEL != INPUT_BOARD_DCO3 && INPUT_BOARD_MODEL != INPUT_BOARD_DCO4
#error "INPUT_BOARD_MODEL must be INPUT_BOARD_DCO3 or INPUT_BOARD_DCO4"
#endif

#define INPUT_IS_DCO3 (INPUT_BOARD_MODEL == INPUT_BOARD_DCO3)
#define INPUT_IS_DCO4 (INPUT_BOARD_MODEL == INPUT_BOARD_DCO4)

// --- voice topology ------------------------------------------------------------
//
// NUM_OSCILLATORS counts the physical oscillators the DCO calibrates, which is
// what sizes the per-oscillator calibration offset array.

#if INPUT_IS_DCO3
#define NUM_VOICES 1
#define NUM_OSCILLATORS 3
#else
#define NUM_VOICES 4
#define NUM_OSCILLATORS (NUM_VOICES * 2)
#endif

// --- serial links --------------------------------------------------------------
//
// The two PCBs swap which UART faces which peer, so never infer the peer from
// the port number. DCO_PORT is the panel TX path in both cases; on DCO4 it
// reaches the DCO through the Mainboard. DCO4 inbound is a different UART:
// Serial1 GP1 (same UART as Screen TX; the Screen never transmits).
//
//   DCO3: Serial1 TX GP0 -> DCO GP21       | RX GP1 <- DCO GP20
//         Serial2 TX GP4 -> Screen GP13    | RX GP5 unwired (Screen never TX)
//   DCO4: Serial2 TX GP4 -> Mainboard PE0  | RX unused (do not listen on GP5)
//         Serial1 TX GP0 -> Screen GP13    | RX GP1 <- Mainboard PE1

#if INPUT_IS_DCO3
#define INPUT_DCO_PORT_OBJ    Serial1
#define INPUT_DCO_RX_PORT_OBJ Serial1
#define INPUT_DCO_RX_PIN      1
#define INPUT_DCO_TX_PIN      0
#define INPUT_SCREEN_PORT_OBJ Serial2
#define INPUT_SCREEN_RX_PIN   5
#define INPUT_SCREEN_TX_PIN   4
// GP5 is free on this PCB (no conductor, and the Screen never transmits), so it
// is reclaimed from the Screen RX above to dim the panel LEDs.
#define INPUT_HAS_LED_PWM 1
#else
#define INPUT_DCO_PORT_OBJ    Serial2
#define INPUT_DCO_RX_PORT_OBJ Serial1
#define INPUT_DCO_RX_PIN      1
#define INPUT_DCO_TX_PIN      4
#define INPUT_SCREEN_PORT_OBJ Serial1
#define INPUT_SCREEN_RX_PIN   1
#define INPUT_SCREEN_TX_PIN   0
// GP5 is not the Mainboard RX pad here (that is GP1). Leave PWM off.
#define INPUT_HAS_LED_PWM 0
#endif

// --- panel layout ---------------------------------------------------------------

// The monosynth exposes OSC3 interval/detune/LFO2 depth on the encoders; the
// 4x2 panel has no third oscillator to steer.
#define INPUT_HAS_OSC3_PANEL INPUT_IS_DCO3

// ADSR3 (EnvDCO) destination cycle. DCO3 walks OSC1/2/3 plus an "all" position;
// DCO4 only has A / B / A+B.
#if INPUT_IS_DCO3
#define INPUT_ADSR3_TO_OSC_SELECT_MAX 4
#else
#define INPUT_ADSR3_TO_OSC_SELECT_MAX 2
#endif

#if INPUT_IS_DCO3
#define INPUT_DEFAULT_VOICE_MODE 0  // mono
#else
#define INPUT_DEFAULT_VOICE_MODE 1  // poly, matching the DCO default
#endif

// Manual calibration: DCO3 0..8 (3×3); DCO4 0..27 (packed A4+B3 per voice).
#define INPUT_CAL_STAGE_MAX (cal_stage_max_n(NUM_OSCILLATORS))
#define INPUT_CAL_STAGE_TO_OSC(stage) cal_stage_to_osc_n((uint8_t)(stage), NUM_OSCILLATORS)
#define INPUT_CAL_STAGE_IS_440(stage) cal_stage_is_440_n((uint8_t)(stage), NUM_OSCILLATORS)
#define INPUT_CAL_STAGE_IS_PW_EDIT(stage) cal_stage_is_pw_edit_n((uint8_t)(stage), NUM_OSCILLATORS)
#define INPUT_CAL_PW_CH(osc) ((NUM_VOICES == NUM_OSCILLATORS) ? (uint8_t)(osc) : (uint8_t)((osc) / 2u))

// The five physical wave keys, in LED order (LED 0..4 = TG_SAW1, TG_SQR1,
// TG_TRI, TG_SAW2, TG_SQR2). The two panels silkscreen the same keys but wire
// them to different oscillators, so the button handler and the LED refresh both
// read this one table instead of hard-coding a layout.
struct InputWaveKey {
  uint8_t osc;   // index into waveEnable[]
  uint8_t wave;  // 0 = saw, 1 = pulse, 2 = triangle
};

#define INPUT_WAVE_KEY_COUNT 5

static const InputWaveKey inputWaveKeys[INPUT_WAVE_KEY_COUNT] =
#if INPUT_IS_DCO3
  // OSC1 saw, OSC2 pulse, OSC1 tri, OSC1 pulse, OSC3 pulse (each oscillator's
  // pulse is switched independently through the DG411).
  { { 0, 0 }, { 1, 1 }, { 0, 2 }, { 0, 1 }, { 2, 1 } };
#else
  // OSC A saw/pulse/tri, then OSC B saw/pulse.
  { { 0, 0 }, { 0, 1 }, { 0, 2 }, { 1, 0 }, { 1, 1 } };
#endif

// waveEnable[osc][wave] -> ParamId. The three enables of an oscillator are
// consecutive (saw, pulse, triangle), and OSC2/OSC3 sit in one later block.
static inline uint8_t input_wave_key_param_id(uint8_t osc, uint8_t wave) {
  return (osc == 0)
           ? (uint8_t)((uint8_t)ParamId::PARAM_OSC1_SAW_ENABLE + wave)
           : (uint8_t)((uint8_t)ParamId::PARAM_OSC2_SAW_ENABLE + (osc - 1) * 3 + wave);
}

// MCU module GP23/24 (same DCO_MCU_BOARD as the DCO). Input firmware does not
// use those GPIOs for the panel — mux channel 23 on button9 is not GPIO 23.
// Pico/Pico 2: drive SMPS PS high (quieter 3V3 for the GP27 analog mux).
// WeAct: GP23 is the onboard KEY (FUNC toggle); GP24 is unused here (DCO analog
// board-fix only).
static constexpr uint8_t MCU_PIN_UNASSIGNED = 0xFF;
#if defined(DCO_MCU_BOARD) && DCO_MCU_BOARD == DCO_MCU_WEACT_RP2040
static constexpr uint8_t SMPS_PS_PIN = MCU_PIN_UNASSIGNED;
static constexpr uint8_t USER_KEY_PIN = 23;
#elif defined(DCO_MCU_BOARD) && ((DCO_MCU_BOARD == DCO_MCU_PICO) || (DCO_MCU_BOARD == DCO_MCU_PICO2))
static constexpr uint8_t SMPS_PS_PIN = 23;
static constexpr uint8_t USER_KEY_PIN = MCU_PIN_UNASSIGNED;
#elif defined(DCO_MCU_BOARD)
#error "DCO_MCU_BOARD must be DCO_MCU_WEACT_RP2040, DCO_MCU_PICO, or DCO_MCU_PICO2"
#else
static constexpr uint8_t SMPS_PS_PIN = MCU_PIN_UNASSIGNED;
static constexpr uint8_t USER_KEY_PIN = MCU_PIN_UNASSIGNED;
#endif

#endif  // BOARD_MODEL_H
