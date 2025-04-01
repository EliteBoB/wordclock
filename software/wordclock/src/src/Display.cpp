#include "logging.h"
#include "Display.h"
#include "ClockFace.h"
#include "Palette.h"
#include "Iot.h"


// Forward declaration of the helper function
String rgbColorToString(const RgbColor& color);

Display::Display(ClockFace* clockFace, uint8_t pin)
    : _clockFace(clockFace),
      _pixels(ClockFace::pixelCount(), pin),
      _animations(ClockFace::pixelCount(), NEO_CENTISECONDS),
      _bootAnimations(2) {}

void Display::setup()
{
    _mode = CLOCK;
    _pixels.Begin();
    _brightnessController.setup();

    // Seed the random number generator
    srand(millis());
}

void Display::loop()
{
  if (_mode == TICKER) {
    return;
  }
  if (_bootAnimations.IsAnimating()) {
    _bootAnimations.UpdateAnimations();
  } else if (_mode == MATRIX) {
      if (_matrix_buf.size() >= NEOPIXEL_COLUMNS * NEOPIXEL_ROWS) {
        _pixels.SetPixelColor(_clockFace->mapMinute(ClockFace::TopLeft), black);
        _pixels.SetPixelColor(_clockFace->mapMinute(ClockFace::TopRight), black); 
        _pixels.SetPixelColor(_clockFace->mapMinute(ClockFace::BottomLeft), black); 
        _pixels.SetPixelColor(_clockFace->mapMinute(ClockFace::BottomRight), black);  
        DLOGLN("Updating matrix from arbitray color vector");
        uint16_t indexPixel = 0;
        for (int j = 0; j < NEOPIXEL_ROWS; j++) {
          {
          for (int i = 0; i < NEOPIXEL_COLUMNS; i++)
            {
              if (_matrix_buf.size() >= indexPixel) {
                _pixels.SetPixelColor(_clockFace->map(i, j), _matrix_buf[indexPixel]);
              }
              indexPixel++;
            }
          }
        }
        _matrix_buf.clear();
      }
  } else {
    _animations.UpdateAnimations();
    if (!_off && _brightnessController.hasChanged())
    {
      DLOGLN("Brightness has changed, updating");
      _update(30); // Update in 300 ms
    }
    _brightnessController.loop();
  }
  _pixels.Show();
}

void Display::setClockFace(ClockFace* clockface)
{
  DLOGLN("Updating clockface");
  _clockFace = clockface;
  _update();
}

void Display::setColor(const RgbColor &color)
{
  DLOGLN("Updating color");
  _brightnessController.setOriginalColor(color);
}

void Display::setColorRandValue(bool value) {
    DLOG("Setting color_rand_value_ to: ");
    DLOGLN(value);
    color_rand_value_ = value;
}

void Display::setHourlyAnimationValue(bool value) {
    DLOG("Setting hourly_animation_value_ to: ");
    DLOGLN(value);
    hourly_animation_value_ = value;
}

void Display::setColorWaveValue(bool value) {
    DLOG("Setting color_wave_value_ to: ");
    DLOGLN(value);
    color_wave_value_ = value;
}

void Display::setOff()
{
  _off = true;
  _update();
}

void Display::setOn()
{
  _off = false;
  _update();
}

void Display::_update(int animationSpeed)
{
    DLOGLN("Updating display");

    _animations.StopAll();



if (color_wave_value_)
   {
    RgbColor randomColorWave = _brightnessController.getCorrectedColor();

      int paletteSize = Palette::size(); // Assuming Palette has a size() function.
      if (paletteSize > 0)
      {
        do
        {
          int randomIndex = rand() % paletteSize; // Generate a random index
          randomColorWave = Palette::getColor(randomIndex); // Get a random color
        } while (randomColorWave == RgbColor(0, 0, 0)); // Exclude black color
      }

      const std::vector<bool> &state = _clockFace->getState();

      for (int index = 0; index < state.size(); index++)
      {
          RgbColor originalColor = _pixels.GetPixelColor(index);
          RgbColor targetColor = _off ? black : (state[index] ? randomColorWave : black);
  
          AnimUpdateCallback animUpdate = [=](const AnimationParam &param) {
              float progress = NeoEase::QuadraticIn(param.progress);
              RgbColor updatedColor = RgbColor::LinearBlend(
                  originalColor, targetColor, progress);
              _pixels.SetPixelColor(index, updatedColor);
          };
          _animations.StartAnimation(index, animationSpeed, animUpdate);
      }
    }
   
else
   { 
    // Use the cached color
    RgbColor randomColor = _cachedColor;
    
    // For all the LEDs, animate a change from the current visible state to the new one
    const std::vector<bool> &state = _clockFace->getState();

    for (int index = 0; index < state.size(); index++)
    {
        RgbColor originalColor = _pixels.GetPixelColor(index);
        RgbColor targetColor = _off ? black : (state[index] ? randomColor : black);

        AnimUpdateCallback animUpdate = [=](const AnimationParam &param) {
            float progress = NeoEase::QuadraticIn(param.progress);
            RgbColor updatedColor = RgbColor::LinearBlend(
                originalColor, targetColor, progress);
            _pixels.SetPixelColor(index, updatedColor);
        };
        _animations.StartAnimation(index, animationSpeed, animUpdate);
    }
   }
}

void Display::updateForTime(int hour, int minute, int second, int animationSpeed)
{
    static int lastHour = -1;

    if (_mode != CLOCK || !_clockFace->stateForTime(hour, minute, second, _show_ampm))
    {
        return; // Nothing to update.
    }

    DLOG("Time: ");
    DLOG(hour);
    DLOG(":");
    DLOGLN(minute);

    // Check if the hour has changed
    if (hour != lastHour && hourly_animation_value_) {
        playHourlyAnimation(); // Play the hourly animation
    }

    // Update the cached color
    if (color_rand_value_)
    {
        int paletteSize = Palette::size();
        if (paletteSize > 0)
        {
            do
            {
                int randomIndex = rand() % paletteSize;
                _cachedColor = Palette::getColor(randomIndex);
            } while (_cachedColor == RgbColor(0, 0, 0)); // Exclude black
        }
    }
    else
    {
        _cachedColor = _brightnessController.getCorrectedColor();
    }

    lastHour = hour; // Update the last hour
    _update(animationSpeed);
}

void Display::_circle(uint16_t x, uint16_t y, int radius, RgbColor color)
{
  for (int i = 0; i < 11; i++)
  {
    for (int j = 0; j < 10; j++) {
      double distance = _distance(i, j, x, y);
      if (distance < radius && distance > radius - 2)
      {
        _pixels.SetPixelColor(_clockFace->map(i, j), color);
      }
    }
  }
}

void Display::_colorCornerPixels(RgbColor color)
{
  for (int corner = _clockFace->TopLeft; corner <= _clockFace->TopRight; corner++)
  {
    _pixels.SetPixelColor(_clockFace->mapMinute(static_cast<ClockFace::Corners>(corner)), color);
  }
}

void Display::_circleAnimUpdate(const AnimationParam& param)
{
  if (param.state == AnimationState_Completed)
    {      
      switch(_circleCount) {
        case 0:
          DLOGLN("Testing green");
          _circleCenterX = 0;
          _circleCenterY = 5;
          _circleColor = HtmlColor(0x007f00);
          _bootAnimations.RestartAnimation(param.index);
          break;
        case 1:
          DLOGLN("Testing blue");
          _circleCenterX = 10;
          _circleCenterY = 5;
          _circleColor = HtmlColor(0x00007f);
          _bootAnimations.RestartAnimation(param.index);
          break;
        default:
          return;
      }
      _circleCount++;
  } else {
    float progress = NeoEase::QuarticOut(param.progress);
    int radius = progress * 15;
    _circle(_circleCenterX, _circleCenterY, radius, _circleColor);
    if (progress >= .95f && progress <= .98f) {
      _colorCornerPixels(_circleColor);
    }
  }
}

void Display::_fadeAll(uint8_t darkenBy)
{
    RgbColor color;
    for (uint16_t indexPixel = 0; indexPixel < _pixels.PixelCount(); indexPixel++)
    {
        color = _pixels.GetPixelColor(indexPixel);
        color.Darken(darkenBy);
        _pixels.SetPixelColor(indexPixel, color);
    }
}

void Display::_fadeAnimUpdate(const AnimationParam& param)
{
    if (param.state == AnimationState_Completed)
    {
      if (_bootAnimations.IsAnimating()) 
      {
        // Keep fading as long as the other animation is running.
        _fadeAll(5);
        _bootAnimations.RestartAnimation(param.index);
      }
      else 
      {
        DLOGLN("Boot animation complete");
        _mode = CLOCK;
        _update(100);
      }
    }
}

void Display::runBootAnimation()
{
    DLOGLN("Starting boot animation");
    _mode = BOOT;
    DLOGLN("Testing red");
    _circleCenterX = 5;
    _circleCenterY = 10;
    _circleColor = HtmlColor(0x7f0000);
    _bootAnimations.StartAnimation(0, 20, [this](const AnimationParam& param) { _fadeAnimUpdate(param);});
    _bootAnimations.StartAnimation(1, 3000,[this](const AnimationParam& param) { _circleAnimUpdate(param);});
}

void Display::setMatrix(std::vector<RgbColor> colorValues)
{
  _animations.StopAll();
  _mode = MATRIX;
  _clockFace->clearDisplay();
  _matrix_buf = colorValues;
}

void Display::clearMatrix()
{
  _mode = CLOCK;
  _update(30);
}

void Display::_displayCharacter(FontTable fontTable, char character, int scrollPosition, RgbColor color) {
  // Get bit array for this character
  std::vector<bool> charData = FontTable::getCharData(fontTable, character);
  // Iterate through each pixel of the character
  if (charData.size() != fontTable.characterHeight * fontTable.characterWidth) {
    return;
  }
  // Center vertically
  static int offsetY = (NEOPIXEL_ROWS - fontTable.characterHeight) / 2;
  for (int i = 0; i < fontTable.characterHeight; i++) {
    for (int j = 0; j < fontTable.characterWidth; j++) {
      int offsetX = scrollPosition + j;
      // Only account for on-screen pixels
      if (offsetX < NEOPIXEL_COLUMNS && offsetX >= 0) {
        _pixels.SetPixelColor(_clockFace->map(offsetX, offsetY + i), charData[i * fontTable.characterWidth + j] ? color : black);
      } 
    }
  }
}

void Display::scrollText(IotWebConf &iwc, String text, RgbColor textColor, int speed, bool rightToLeft)
{
  DLOGLN("Ticker activated");
  DLOGLN(text);

  const FontTable fontTable = font5x7;  
  const int letterSpacing = 1;
  int textLength = text.length();
  int scrollSpeed = std::min(10000, std::max(10, speed));
  int scrollDirection = rightToLeft ? 1 : -1;

  if (fontTable.characterHeight > NEOPIXEL_ROWS) {
    DLOGLN("Font is too tall to fit the display");
    return;
  }

  _animations.StopAll();
  _mode = TICKER;

  // Calculate total scrolling distance
  const int letterWidth = fontTable.characterWidth + letterSpacing;
  const int totalScrollDistance = letterWidth * textLength + NEOPIXEL_COLUMNS;

  // Starting position
  int scrollPosition = scrollDirection == 1 ? NEOPIXEL_COLUMNS - totalScrollDistance : NEOPIXEL_COLUMNS;

  // Iterate through each pixel of the scrolling text
  for (int i = 0; i <= totalScrollDistance; i++) {
    _pixels.ClearTo(black);

    for (int j = 0; j < textLength; j++) {
      int charPos = scrollPosition + j * letterWidth;
      if (charPos >= -fontTable.characterWidth && charPos < NEOPIXEL_COLUMNS) {
      _displayCharacter(fontTable, text.charAt(j), charPos, textColor);
      }
    }

    // Update the matrix
    _pixels.Show();

    // Update scroll position
    scrollPosition += scrollDirection;

    // Delay between frames
    iwc.delay(scrollSpeed);
  }

  _mode = CLOCK;
  DLOGLN("Ticker exited");
  _update();
}

void Display::hourlyAnimationFlash()
{
  for (int i = 0; i < 3; i++) {
    RgbColor randomColor = RgbColor(rand() % 256, rand() % 256, rand() % 256); // Generate random color
    _pixels.ClearTo(randomColor); // Use the random color
    _pixels.Show();
    delay(500);

    _pixels.ClearTo(black); // Turn off
    _pixels.Show();
    delay(500);
    }
}

void Display::hourlyAnimationRainbow()
{
    for (int cycle = 0; cycle < 3; cycle++) {
        for (int hue = 0; hue < 360; hue += 10) {
            RgbColor color = HslColor(hue / 360.0f, 1.0f, 0.5f);
            _pixels.ClearTo(color);
            _pixels.Show();
            delay(50);
        }
    }
}

void Display::hourlyAnimationWave()
{
    for (int wave = 0; wave < 10; wave++) {
        for (int i = 0; i < _pixels.PixelCount(); i++) {
            RgbColor color = (i % 2 == wave % 2) ? RgbColor(rand() % 256, rand() % 256, rand() % 256) : black;
            _pixels.SetPixelColor(i, color);
        }
        _pixels.Show();
        delay(200);
    }
}

void Display::hourlyAnimationSparkle()
{
    for (int i = 0; i < 50; i++) {
        int randomPixel = rand() % _pixels.PixelCount();
        _pixels.SetPixelColor(randomPixel, RgbColor(255, 255, 255));
        _pixels.Show();
        delay(50);

        _pixels.SetPixelColor(randomPixel, black);
        _pixels.Show();
    }
}

void Display::hourlyAnimationChasingLights()
{
    for (int i = 0; i < _pixels.PixelCount() * 1; i++) { // one cycle
        for (int j = 0; j < _pixels.PixelCount(); j++) {
            RgbColor color = (j == i % _pixels.PixelCount()) ? RgbColor(rand() % 256, rand() % 256, rand() % 256) : black; 
            _pixels.SetPixelColor(j, color);
        }
        _pixels.Show();
        delay(100);
    }
}

void Display::hourlyAnimationExpandingCircle()
{
  {
      RgbColor randomColor = RgbColor(rand() % 256, rand() % 256, rand() % 256); // Generate random color
      for (int radius = 1; radius <= 10; radius++) { // Expand the circle
          _circle(5, 5, radius, randomColor); // Use random color for the circle
          _pixels.Show();
          delay(100);
      }
  }
}

void Display::playHourlyAnimation()
{
    DLOGLN("Playing hourly animation");

    // Generate a random number to pick an animation
    int animationIndex = rand() % 6; // Adjust the range based on the number of animations

    switch (animationIndex) {
        case 0:
            DLOGLN("Playing Flash Animation");
            hourlyAnimationFlash();
            break;
        case 1:
            DLOGLN("Playing Rainbow Animation");
            hourlyAnimationRainbow();
            break;
        case 2:
            DLOGLN("Playing Wave Animation");
            hourlyAnimationWave();
            break;
        case 3:
            DLOGLN("Playing Sparkle Animation");
            hourlyAnimationSparkle();
            break;
        case 4:
            DLOGLN("Playing Chasing Lights Animation");
            hourlyAnimationChasingLights();
            break;
        case 5:
            DLOGLN("Playing Expanding Circle Animation");
            hourlyAnimationExpandingCircle();
            break;            
        default:
            DLOGLN("Unknown animation index");
            break;
    }

    // Restore the display to the current time
    _update();
}

String rgbColorToString(const RgbColor& color) {
    return "(" + String(color.R) + ", " + String(color.G) + ", " + String(color.B) + ")";
}