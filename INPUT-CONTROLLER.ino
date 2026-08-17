
#include "Arduino.h"
#include "sram_hot.h"
//#include <Adafruit_TinyUSB.h>

// Derives voice count, UART wiring and panel layout from the instrument this
// checkout belongs to, which comes from the superproject's project_config.h.
// Nothing in this sketch differs between DCO3-MONOSYNTH and DCO4-REBORN.
#include "board_model.h"

#if INPUT_IS_DCO4
static bool preset_dir_retry_pending = false;
static uint32_t preset_dir_retry_at_ms = 0;
#endif

int8_t OSC1Interval = 24;
int8_t OSC2Interval = 36;  // 36 ⇒ unison with OSC1 (wire bias; display = value - 36)
int8_t OSC3Interval = 36;
int16_t OSC2Detune = 0;
int16_t OSC3Detune = 0;
float DETUNE1;
float DETUNE2;
uint16_t PW;

int16_t SubLevel;
int16_t OSC1Level;
int16_t OSC2Level;
int16_t OSC3Level;

uint16_t RESONANCE;
uint16_t CUTOFF = 1024;
int16_t VCALevel = 0;

#include <stdint.h>
#include "_build_libs/DCO-PROTOCOL/params_def.h"
#include "params.h"

#include "auxiliary.h"
#include "Timers_millis.h"

#include "Serial.h"
#include "Controls.h"
#include "buttons.h"
#include "encoders.h"

#include "Flow.h"
#include "PresetSaveFlow.h"

#include "formulas.h"

#include "LED_control.h"



uint32_t tiempodeejecucion;
unsigned long loopStartTime;

void setup() {
  // Core0 boot: panel mux/encoders and lin→exp table.
  init_controls();
  init_tables();

  // USBDevice.setManufacturerDescriptor("FELA         ");
  // USBDevice.setProductDescriptor("DCO4 Input Controller        ");
}

void setup1() {
  // Core1 boot: UARTs, LEDs, preset directory fetch, LED PWM.
#ifdef ENABLE_SERIAL
  Serial.begin(2000000);
#endif
#ifdef ENABLE_DCO_LINK
  // DCO3: Serial1 TX+RX to the DCO. DCO4: Serial2 TX GP4 to the Mainboard;
  // inbound is Serial1 GP1 (DCO_RX_PORT), begun with SCREEN_PORT below.
  DCO_PORT.setTX(INPUT_DCO_TX_PIN);
#if INPUT_IS_DCO3
  DCO_PORT.setRX(INPUT_DCO_RX_PIN);
#endif
  DCO_PORT.setPollingMode(false);
  DCO_PORT.setFIFOSize(512);
  DCO_PORT.begin(2500000);
  init_param_router();
  init_dco_link_parser();
#endif

#ifdef ENABLE_SCREEN_LINK
  // DCO3: TX only. DCO4: TX to Screen on GP0, RX from Mainboard on GP1.
  SCREEN_PORT.setRX(INPUT_SCREEN_RX_PIN);
  SCREEN_PORT.setTX(INPUT_SCREEN_TX_PIN);
  SCREEN_PORT.setPollingMode(false);
  SCREEN_PORT.setFIFOSize(512);
  SCREEN_PORT.begin(2500000);
#endif
  serial_dma_init();
#ifdef ENABLE_SCREEN_LINK
  // Let the Screen learn which synth it's attached to (3 vs 8 oscillators)
  // without a per-project build flag; screen_target.h derives its whole
  // calibration UI from this one value. Silent: 157 is outside the 150..155
  // range that raises the Screen's redraw flag.
  serialSendParamByteToScreen(ParamId::PARAM_UI_VOICE_TOPOLOGY, (uint8_t)NUM_OSCILLATORS);
#endif

  init_LED_control();

  // Preset storage lives on the DCO now (preset_store.h); Input just fetches
  // the 256-slot name directory into RAM. Boot-time preset *recall* is the
  // DCO's own job (preset_store_boot_recall()), not Input's.
  request_preset_directory();

#if INPUT_IS_DCO4
  preset_dir_retry_pending = true;
  preset_dir_retry_at_ms = millis() + 2000u;
#endif

#if INPUT_HAS_LED_PWM
  // GP5 ends up a PWM output here, which takes it back from the setRX above.
  pinMode(PIN_LED_PWM, OUTPUT);
  analogWriteFreq(200000);
  analogWrite(PIN_LED_PWM, 245);
#endif
}

void __not_in_flash_func(loop1)() {
  // Core1: map manual controls, TX blocks, LED refresh, Mainboard/DCO RX relay.
  // Drain inbound frames before any panel TX so a blocked DCO_PORT write cannot
  // let the Mainboard mirror backlog overflow the RX FIFO.
  serial_read_from_dco();
  serial_dma_poll();

  unsigned long loopStartMicros = micros();

  millisTimer2();

  if (timer1msFlag2) {
    setControlValues();  //LO HACE EL INPUT BOARD
    serial_send_manual_controls(false);
  }

  if (ledRefreshPending) {
    ledRefreshPending = false;
    set_LED_Status(LED_REFRESH_ALL, 0);
  }

  if (timer31msFlag2) {
    LED_Control_Mux.update();
  }

  if (timer200msFlag2) {
    //serial_send_param_change(22, ADSR1Level[0]);
    //drawTM(RESONANCE);
    //drawTM(CUTOFF);
    //serial_send_param_change(15, ADSR3toDETUNE1_formula * 100000);
    //Serial.println(tiempodeejecuciontotal);
  }
#if INPUT_IS_DCO4
  if (preset_dir_retry_pending && preset_dir_retry_at_ms != 0 &&
      (int32_t)(millis() - preset_dir_retry_at_ms) >= 0) {
    preset_dir_retry_pending = false;
    preset_dir_retry_at_ms = 0;
    request_preset_directory();
  }
#endif
  serial_read_from_dco();
}

void __not_in_flash_func(loop)() {
  // Core0: soft timers + panel scan (mux / encoders / buttons).

  // loopStartTime = micros();

  millisTimer();

  readControls();

  // uint32_t j = micros();

  // tiempodeejecucion = (micros() - j);

  // unsigned long tiempodeejecuciontotal = micros() - i;

  //Serial.println(tiempodeejecuciontotal);
  if (timer200msFlag) {
    //serial_send_param_change(22, ADSR1Level[0]);
    //drawTM(RESONANCE);
    //drawTM(CUTOFF);
    //serial_send_param_change(15, ADSR3toDETUNE1_formula * 100000);
    //Serial.println(tiempodeejecuciontotal);
  
  }


  // if (tiempodeejecuciontotal > 600) {
  //     drawTM(tiempodeejecuciontotal);
  //     drawTMScreen(true);
  //   }

  // if (tiempodeejecucion > 2) {
  // drawTMScreen(true);
  // }

#ifdef ENABLE_SERIAL
  //drawTM(tiempodeejecucion);
  if (timer200msFlag) {
  }
  if (1 == 2) {
  //if (timer99microsFlag) {58
  //if (timer200msFlag) {
    // if (tiempodeejecuciontotal > 100 ) {
    //    contadorLatencia++;
    //    float tiemposobrelatencia = (float) micros() / contadorLatencia; // baseline = 5000
    //if (noteEnd[0] == 1) {
    //    Serial.print(tiempodeejecuciontotal);
    //    tiempodeejecucionMedian.add(tiempodeejecuciontotal);
    //    tiempodeejecucionMedian2.add(tiempodeejecucionMedian.getMedian());
    //    Serial.print(tiempodeejecucionMedian.getMedian());
    //    Serial.print((String)" - t_j : " + tiempodeejecuciontotal + (String)" | ");
    //    //    Serial.print((String)" - t / l : " + tiemposobrelatencia);
    //    //    Serial.print((String)" - noteStart" + noteStart[0]);
    //    //    Serial.print((String)" - noteEnd" + noteEnd[0]);
    //    //  //Serial.print((String) " - LFO1Level" + LFO1Level);
    //    ////  Serial.print((String)"- VCF_LFO_INT: " + VCFLFOIntensityLog);
    //    Serial.print((String)"- Fader raw : " + muxFadersRaw[4]);
    //    Serial.print((String)"- Fader median : " + faderMedian[4]);
    //   Serial.print((String)"- Attack2: " + ADSR2_attack);
    //   Serial.print((String)"- Decay2: " + ADSR2_decay);
    //   Serial.print((String)"- Sustain2: " + ADSR2_sustain);
    //    //  Serial.print((String)"- enc1: " + encVal[0]);
    //    //Serial.print((String)" - enc2: " + encVal[1]);
    //    Serial.print((String)" - enc3: " + encVal[2]);
    //    Serial.print((String)" - enc4: " + encVal[3]);
    //    Serial.print((String)" - enc5: " + encVal[4]);
    // Serial.print((String)" - CUTOFF: " + CUTOFF);
    // Serial.print((String)" - RESONANCE: " + RESONANCE);
    // Serial.print((String)" - ADSR2 TO VCF: " + ADSR2toVCF);
    // Serial.print((String)" - LFO TO VCF: " + LFO1toVCF);
    //    Serial.print((String)" - LFO1 TO DCO: " + LFO1toDCO);
    //    Serial.print((String)" - LFO1 SPEED: " + LFO1Speed);
    //    Serial.print((String)" - LFO1 LEVEL: " + LFO1Level);
    //    Serial.print((String)"- DETUNE1 : ");
    //    Serial.print(DETUNE1, 4);
    //Serial.print((String)"- OSC2 DETUNE : " + OSC2Detune);
    //Serial.print((String)" - PW : " + PW);
    //Serial.print((String)" - LFO1toPWM : " + LFO1toPWM);
    //Serial.print((String)" - ADSR1toPWM : " + ADSR1toPWM);
    //Serial.print((String)"- velocity : " + velocity[0]);
    //Serial.print((String)"- freq : " + freq);
    //    Serial.print("HOLA");
    //Serial.print((String)" -enc6" + encVal[5]);

    // Serial.print((String) "ADSR3_attack:" +  (uint16_t)ADSR3_attack + (String) "   ");
    // Serial.print((String) ",ADSR3_decay:" + (uint16_t)ADSR3_decay + (String) "   ");
    // Serial.print((String) ",ADSR3_sustain:" +  (uint16_t)ADSR3_sustain + (String) "   ");
    // Serial.print((String) ",ADSR3_release:" +  (uint16_t)ADSR3_release + (String) "   ");
    // Serial.println();

     for (int i = 0; i < 16; i++) {

    // Serial.print((String) ", MuxAnalog" + (int)i + (String) " " + (uint16_t)muxAnalogData[i] + (String) "   ");
       Serial.print((String) ", MUXAnalogFiltered" + (int)i + (String) ":" + /*(uint16_t)muxAnalogRaw[i]*/ (uint16_t)muxAnalogData[i] + (String) "   ");
     }
    //  for (int i = 0; i < 8; i++) {
    //    Serial.print((String)", -MuxFader" + (int)i + (String)": " + (uint16_t)faderMedian[i]);
    //  }
    // Serial.print(note[0]);
    //Serial.print(analogRead(PC0));
  }
#endif
}
