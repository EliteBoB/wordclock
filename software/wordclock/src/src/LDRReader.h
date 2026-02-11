#pragma once
#include "pins.h"
/*
 * Light sensor. Reads the ambient light and build a representative value
 * between 0.0 and 1.0. Smooth the reading by doing a moving average.
 */
class LDRReader
{
public:
  // Reaction speed must be greater than 0 and at most 1.0. This controls the
  // input smoothing, the higher the number the less smooth the data will be.

  LDRReader(int pinNumber = LDR_PIN, float reactionSpeed = .1, int sensitivity = 1);

  // Must be called from the ino setup and loop.
  void setup();
  void loop();

  // Returns a normalized, dampened value between 0. (no light) and 1. (much lights),
  // adjusted against sensitivity setting.
  float reading();

  // Returns a value between 0. (no light) and 4096. (much lights).
  uint16_t readingRaw();

  int sensitivity;

private:
  float _currentLDR;
  float _reactionSpeed;
  int _pin;
};