#include "_build_libs/CD74HC4067/src/CD74HC4067.cpp"
// Boot Core0: 12-bit ADC, encoders begin, mux GPIO directions.
void init_controls() {
  analogReadResolution(12);

  analogReadResolution(12);

  // Initialize the instances actually living in the arrays!
  for(int i = 0; i < NUM_ENCODERS; i++) {
    encoders[i].MD_REncoder_Name.begin();
  }
  
  pinMode(digitalMUX1_PIN_SIG0, INPUT);
  pinMode(digitalMUX1_PIN_SIG0, INPUT);
  pinMode(digitalMUX2_PIN_SIG0, INPUT);
  pinMode(digitalMUX3_PIN_SIG0, INPUT);

  pinMode(muxAnalog_PIN_CH0, OUTPUT);
  pinMode(muxAnalog_PIN_CH1, OUTPUT);
  pinMode(muxAnalog_PIN_CH2, OUTPUT);
  pinMode(muxAnalog_PIN_CH3, OUTPUT);

  pinMode(muxAnalog_PIN_SIG, INPUT);
}

void init_tables() {
  for (int i = 0; i < LIN_TO_EXP_TABLE_SIZE; i++) {
    linToExpLookup[i] = linearToExponential(i, 50, maxADSRControlValue);
  }
}
// Core0 hot path: scan mux (analog on 1 ms) and encoders/buttons (~99 µs).
void SRAM_HOT(readControls)() {

  if (timer1msFlag) {
    read_digitalMux(1);
    read_AnalogMux();
  } else {
    read_digitalMux(0);
  }

  if (timer99microsFlag) {
    // if (!presetSaved) {           /// OLD METHOD FOR SAVING PRESETS
    //   read_encoders_preset_save();
    //   read_encoder_buttons_preset_save();
    // } else {
    read_encoders();
    read_encoder_buttons();
  }
}


/**
 * @brief Maps raw ADC reading with explicit low and high deadzones.
 */
 static inline int16_t SRAM_HOT(mapWithDeadzone)(int32_t val, int32_t dz_low, int32_t dz_high, int32_t out_min, int32_t out_max) {
  int32_t clamped = constrain(val, dz_low, dz_high);
  return (int16_t)map(clamped, dz_low, dz_high, out_min, out_max);
}


// Core1 @1 ms: map filtered fader/pot ADC into locals when manual flags are set.
void SRAM_HOT(setControlValues)() {

    // Deadzones for 12-bit / 4095 controls (Faders, Cutoff, Resonance, PW)
    static constexpr uint16_t dead_zone_low       = 25;
    static constexpr uint16_t dead_zone_high      = 4085;
  
    // Dedicated Deadzones for 512-max pots (ADSR2->VCF, LFO2->VCF, ADSR1->VCA)
    static constexpr uint16_t dead_zone_512_low   = 40;   // (512 - 507) * 8 = 40
    static constexpr uint16_t dead_zone_512_high  = 4056; // 507 * 8 = 4056
  
    // 1. Fader Row 1: ADSR 1 (0..4095)
    if (faderRow1ControlManual) {
      ADSR1_attack  = mapWithDeadzone(muxAnalogData[fader1ArrayPos], dead_zone_low, dead_zone_high, 0, 4095);
      ADSR1_decay   = mapWithDeadzone(muxAnalogData[fader2ArrayPos], dead_zone_low, dead_zone_high, 0, 4095);
      ADSR1_sustain = mapWithDeadzone(muxAnalogData[fader3ArrayPos], dead_zone_low, dead_zone_high, 0, 4095);
      ADSR1_release = mapWithDeadzone(muxAnalogData[fader4ArrayPos], dead_zone_low, dead_zone_high, 0, 4095);
    }
  
    // 2. Fader Row 2: ADSR 3 (if active) or ADSR 2 (0..4095)
    if (faderRow2ControlManual) {
      if (ADSR3Enabled) {
        ADSR3_attack  = mapWithDeadzone(muxAnalogData[fader5ArrayPos], dead_zone_low, dead_zone_high, 0, 4095);
        ADSR3_decay   = mapWithDeadzone(muxAnalogData[fader6ArrayPos], dead_zone_low, dead_zone_high, 0, 4095);
        ADSR3_sustain = mapWithDeadzone(muxAnalogData[fader7ArrayPos], dead_zone_low, dead_zone_high, 0, 4095);
        ADSR3_release = mapWithDeadzone(muxAnalogData[fader8ArrayPos], dead_zone_low, dead_zone_high, 0, 4095);
      } else {
        ADSR2_attack  = mapWithDeadzone(muxAnalogData[fader5ArrayPos], dead_zone_low, dead_zone_high, 0, 4095);
        ADSR2_decay   = mapWithDeadzone(muxAnalogData[fader6ArrayPos], dead_zone_low, dead_zone_high, 0, 4095);
        ADSR2_sustain = mapWithDeadzone(muxAnalogData[fader7ArrayPos], dead_zone_low, dead_zone_high, 0, 4095);
        ADSR2_release = mapWithDeadzone(muxAnalogData[fader8ArrayPos], dead_zone_low, dead_zone_high, 0, 4095);
      }
    }
  
    // 3. VCF Pots
    if (VCFPotsControlManual) {
      CUTOFF     = mapWithDeadzone(muxAnalogData[pot2ArrayPos], dead_zone_low, dead_zone_high, 4095, 0);
      RESONANCE  = mapWithDeadzone(muxAnalogData[pot3ArrayPos], dead_zone_low, dead_zone_high, 4095, 0);
      
      // Uses 512 deadzone profile (40 .. 4056 -> 512 .. 0)
      ADSR2toVCF = mapWithDeadzone(muxAnalogData[pot4ArrayPos], dead_zone_512_low, dead_zone_512_high, 512, 0);
      LFO2toVCF  = mapWithDeadzone(muxAnalogData[pot1ArrayPos], dead_zone_512_low, dead_zone_512_high, 512, 0);
    }
  
    // 4. VCA Pots
    if (VCAPotsControlManual) {
      // Uses 512 deadzone profile (40 .. 4056 -> 512 .. 0)
      ADSR1toVCA = mapWithDeadzone(muxAnalogData[pot5ArrayPos], dead_zone_512_low, dead_zone_512_high, 512, 0);
    }
  
    // 5. PWM Pots
    if (PWMPotsControlManual) {
      PW = mapWithDeadzone(muxAnalogData[pot6ArrayPos], dead_zone_low, dead_zone_high, 0, 4095);
    }
}

// Apply dual Kalman filters to muxAnalogRaw[] → muxAnalogData[].
void SRAM_HOT(read_AnalogMux)() {

  for (uint8_t i = 0; i < 16; i++) {

muxAnalogData[i] = simpleKalmanFilter[i+16].updateEstimate(simpleKalmanFilter[i].updateEstimate(muxAnalogRaw[i]));

  }
}

// Scan 16 mux channels into valorMUX1[48]; optionally sample analog on each channel.
void SRAM_HOT(read_digitalMux)(bool readPots) {

  for (activeDigitalMuxChannel = 0; activeDigitalMuxChannel < 16; activeDigitalMuxChannel++) {

    muxDigital.channel(activeDigitalMuxChannel);
    delayMicroseconds(1);
    valorMUX1[activeDigitalMuxChannel] = digitalRead(digitalMUX1_PIN_SIG0);
    valorMUX1[activeDigitalMuxChannel + 16] = digitalRead(digitalMUX2_PIN_SIG0);
    valorMUX1[activeDigitalMuxChannel + 32] = digitalRead(digitalMUX3_PIN_SIG0);

    if (readPots) {
      muxAnalogRaw[activeDigitalMuxChannel] = analogRead(muxAnalog_PIN_SIG);
    }

    //activeDigitalMuxChannel++;
    //if (activeDigitalMuxChannel > 15) activeDigitalMuxChannel = 0;
  }
}

// Cubic-ish fader curve helper (also used from Controls).
uint16_t SRAM_HOT(faderExpConverter)(uint16_t readingValue) {
  uint16_t pow3Calc = readingValue / 4;
  uint16_t expValOut = pow3Calc * pow3Calc * pow3Calc / 20000;
  return expValOut;
}

float SRAM_HOT(expConverterFloat)(uint16_t readingValue, uint16_t curve) {
  uint16_t pow3Calc = readingValue;
  float expValOut = (float)pow3Calc * pow3Calc / curve;
  if (expValOut < 0.005) {
    expValOut = 0;
  }
  return expValOut;
}

uint16_t SRAM_HOT(expConverter)(uint16_t readingValue, uint16_t curve) {
  uint16_t pow3Calc = readingValue;
  uint16_t expValOut = (float)pow3Calc * pow3Calc / curve;
  if (expValOut < 0.1) {
    expValOut = 0;
  }
  return expValOut;
}

uint16_t SRAM_HOT(expConverterReverse)(uint16_t readingValue, uint16_t curve) {
  uint16_t expValOut = sqrt((float)readingValue / curve);
  return expValOut;
}

uint16_t SRAM_HOT(expConverterFloatReverse)(float readingValue, uint16_t curve) {
  uint16_t expValOut = sqrt(readingValue / curve);
  return expValOut;
}
