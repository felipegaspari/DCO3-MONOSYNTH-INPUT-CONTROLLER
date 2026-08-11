#ifndef BOARD_MODEL_H
#define BOARD_MODEL_H

#include <stdint.h>
#include "params_def.h"

// -----------------------------------------------------------------------------
// Which instrument this Input controller build targets.
//
// The same panel firmware runs the front panel of both synths. Everything that
// differs between them is derived here, so every other file in this sketch is
// byte-identical in the DCO3-MONOSYNTH and DCO4-REBORN trees and the two copies
// can be diffed (or shared outright) with no merge work.
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

// >>> The only line that differs between the DCO3-MONOSYNTH and DCO4-REBORN
// >>> copies of this sketch. Everything else in the folder is byte-identical.
// (The guard lets a build override it, e.g. --build-property
// compiler.cpp.extra_flags=-DINPUT_BOARD_MODEL=4, to compile-check both models
// from one tree.)
#ifndef INPUT_BOARD_MODEL
#define INPUT_BOARD_MODEL INPUT_BOARD_DCO3
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
// the port number. DCO_PORT is the name the panel TX path uses in both cases;
// on DCO4 it reaches the DCO through the Mainboard.
//
//   DCO3: Serial1 TX GP0 -> DCO GP21       | RX GP1 <- DCO GP20
//         Serial2 TX GP4 -> Screen GP13    | RX GP5 unwired (Screen never TX)
//   DCO4: Serial2 TX GP4 -> Mainboard PE0  | RX GP5 <- Mainboard PE1
//         Serial1 TX GP0 -> Screen GP13    | RX GP1 unwired

#if INPUT_IS_DCO3
#define INPUT_DCO_PORT_OBJ    Serial1
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
#define INPUT_DCO_RX_PIN      5
#define INPUT_DCO_TX_PIN      4
#define INPUT_SCREEN_PORT_OBJ Serial1
#define INPUT_SCREEN_RX_PIN   1
#define INPUT_SCREEN_TX_PIN   0
// GP5 is the Mainboard RX pad here — never drive it. LED brightness is fixed.
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

// Manual calibration walks the oscillators one stage at a time. The monosynth
// splits each oscillator into two stages (sawtooth, then pulse) because its
// waveforms are switched independently in the analog path.
#if INPUT_IS_DCO3
#define INPUT_CAL_STAGES_PER_OSC 2
#else
#define INPUT_CAL_STAGES_PER_OSC 1
#endif
#define INPUT_CAL_STAGE_MAX ((NUM_OSCILLATORS * INPUT_CAL_STAGES_PER_OSC) - 1)
#define INPUT_CAL_STAGE_TO_OSC(stage) ((uint8_t)((stage) / INPUT_CAL_STAGES_PER_OSC))

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

#endif  // BOARD_MODEL_H
