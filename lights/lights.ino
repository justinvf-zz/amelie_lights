#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Comment out to remove prints (and shrink program)
#define DEBUG_LIGHTS 1

// Pin that's connected to the pixels
#define PIXEL_PIN 6

// Analog Pin the's connected to the mic
#define MIC_PIN 0

// How many samples to take in clap detection
#define SAMPLES 512

/* // How many NeoPixels are attached to the Arduino? */
#define NUMPIXELS      16
#define DELAY_MILLIS   50
#define MAX_BRIGHT     40

#define R 0
#define G 1
#define B 2

int get_spread() {
    int min_val = 0;
    int max_val = 0;
    for (int i = 0 ; i < SAMPLES; i++) {
      int k = analogRead(MIC_PIN);
      if (k < min_val) {
	min_val = k;
      }
      if (k > max_val) {
	max_val = k;
      }
    }
    sei();
    return max_val - min_val;
}

#define N_RAINBOW 6
const int RAINBOW[][3] = {{MAX_BRIGHT, 0, 0},  // Red
			  {MAX_BRIGHT, MAX_BRIGHT/2, 0},  // Orange
			  {MAX_BRIGHT, MAX_BRIGHT, 0},  // Yellow
			  {0, MAX_BRIGHT, 0},  // Green
			  {0, 0, MAX_BRIGHT},  // Blue
			  {MAX_BRIGHT/2, 0, MAX_BRIGHT}};  // Violet

byte clamp(int c) {
  if (c > MAX_BRIGHT) {
    return MAX_BRIGHT;
  } else if (c < 0) {
    return 0;
  }
  return (byte)c;
}

class PixelManager {
 private:
  byte target_color[NUMPIXELS][3];
  byte current_color[NUMPIXELS][3];
  Adafruit_NeoPixel pixels;

 public:
  PixelManager() {
    pixels = Adafruit_NeoPixel(NUMPIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
    pixels.begin();
    new_random_targets();
    update_pixels();
  }

  void new_random_targets() {
    target_color[0][R] = random(MAX_BRIGHT);
    target_color[0][G] = random(MAX_BRIGHT);
    target_color[0][B] = random(MAX_BRIGHT);

#ifdef DEBUG_LIGHTS
    Serial.print("Random Targets R: ");
    Serial.print(target_color[0][R]);
    Serial.print(" G: ");
    Serial.print(target_color[0][G]);
    Serial.print(" B: ");
    Serial.println(target_color[0][B]);
#endif

    // Everything moves towards the main color.
    for (int i = 1; i < NUMPIXELS; i++) {
      target_color[i][R] = target_color[0][R];
      target_color[i][G] = target_color[0][G];
      target_color[i][B] = target_color[0][B];
    }
    perturb_targets(8);
  }

  int perturb(int delta, int current) {
    return clamp(current + random(delta) - delta / 2);
  }

  void perturb_targets(int delta) {
    for (int i = 0; i < NUMPIXELS; i++) {
      target_color[i][R] = perturb(delta, target_color[i][R]);
      target_color[i][G] = perturb(delta, target_color[i][G]);
      target_color[i][B] = perturb(delta, target_color[i][B]);
    }
  }

  void update_pixels() {
    for (int i = 0; i < NUMPIXELS; i++) {
      for (int j = 0; j < 3; j++) {
	int c = current_color[i][j];
	if (c < target_color[i][j]) {
	  c += random(4) - 1;
	} else if (c > target_color[i][j]) {
	  c -= random(4) - 1;
	}
	current_color[i][j] = clamp(c);
      }
    }
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(current_color[i][R],
					   current_color[i][G],
					   current_color[i][B]));
    }
    pixels.show();
  }

  void target_rainbow(int rainbow_index) {
    for (int i = 0; i < NUMPIXELS; i++) {
      for (int j = 0; j < 3; j++) {
	target_color[i][j] = RAINBOW[rainbow_index][j];
      }
    }
  }
};


void setup() {
#ifdef DEBUG_LIGHTS
  Serial.begin(57600); // use the serial port
#endif
}


void loop() {
  unsigned long last_update = millis();
  bool should_switch = false;
  int rainbow_index =  0;

  PixelManager manager;

  int last_spread  = get_spread();

  while(1) {
    unsigned long start = millis();
    int spread = get_spread();
    if (spread - last_spread > 5) {
#ifdef DEBUG_LIGHTS
      Serial.println("clap");
#endif
      should_switch = true;
    }
    last_spread = spread;
    if (millis() - last_update > DELAY_MILLIS) {
      last_update = millis();
      if (should_switch) {
	if (rainbow_index == (N_RAINBOW + 1)) {
	  manager.new_random_targets();
	  rainbow_index = 0;
	} else {
	  manager.target_rainbow(rainbow_index++ % N_RAINBOW);
	}
    	should_switch = false;
      } else if (random(100) > 95) {
      	manager.perturb_targets(random(5));
      }
      manager.update_pixels();
    }
  }
}
