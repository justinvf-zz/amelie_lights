// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            6

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      16
#define R               0
#define G               0
#define B               0
#define DELAY_MILLIS   10
#define JUMP            5
#define MAX_BRIGHT    150

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int target_color[NUMPIXELS][3];
int current_color[NUMPIXELS][3];

void setup() {
  pixels.begin(); // This initializes the NeoPixel library.
  update_pixels();
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 3; j++) {
      current_color[i][j] = random(MAX_BRIGHT);
    }
  }
}

byte clamp(int c) {
  if (c > MAX_BRIGHT) {
    return MAX_BRIGHT;
  } else if (c < 0) {
    return 0;
  }
  return (byte)c;
}

void set_to_first() {
  for (int i = 1; i < NUMPIXELS; i++) {
    for (int j = 0; j < 3; j++) {
      target_color[i][j] = target_color[i][0];
    }
  }
}

void new_random_targets() {
  for (int i = 1; i < NUMPIXELS; i++) {
    for (int j = 0; j < 3; j++) {
      target_color[i][j] = random(MAX_BRIGHT);
    }
  }
}

void update_pixels() {
  for (int i = 0; i < NUMPIXELS; i++) {
    for (int j = 0; j < 3; j++) {
      if (current_color[i][j] < target_color[i][j]) {
	current_color[i][j] += random(JUMP);
      } else if (current_color[i][j] < target_color[i][j]) {
	current_color[i][j] -= random(JUMP);
	if (current_color[i][j	 0) {
	  current_color[i][j] = 0;
      }
      }
      current_color[i][j] += (target_color[i][j] - current_color[i][j])/8;
    }
  }
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(current_color[i][R],
					 current_color[i][G],
					 current_color[i][B]));
  }
  pixels.setPixelColor(0, pixels.Color(120, 0, 120));
  pixels.setPixelColor(15, pixels.Color(120, 0, 120));
  pixels.show();
}

void loop() {
  if (random(20) == 1) {
    new_random_targets();
  }
  if (random(100) == 1) {
    set_to_first();
  }
  update_pixels();
  delay(DELAY_MILLIS);
}