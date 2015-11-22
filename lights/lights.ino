/*
fht_adc_serial.pde
guest openmusiclabs.com 7.7.14
example sketch for testing the fht library.
it takes in data on ADC0 (Analog0) and processes them
with the fht. the data is sent out over the serial
port at 115.2kb.
*/

#define LOG_OUT 1 // use the log output function
#define FHT_N 128 // set to 256 point fht
#define SCALE 4

#include <FHT.h> // include the library
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif


void setup_fht() {
  Serial.begin(57600); // use the serial port
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe7; // set the adc to free running mode
  ADMUX = 0x40; // use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0
}

#define SMOOTH .3f
#define RUNNING_AVG_SMOOTH .95f
#define INIT_READS 20

void get_fht() {
    cli();  // UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < FHT_N ; i++) {
      while(!(ADCSRA & 0x10)); // wait for adc to be ready
      ADCSRA = 0xf7; // restart adc
      byte m = ADCL; // fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m; // form into an int
      k -= 0x0200; // form into a signed int
      k <<= 6; // form into a 16b signed int
      fht_input[i] = k; // put real data into bins
    }
    fht_window(); // window the data for better frequency response
    fht_reorder(); // reorder the data before doing the fht
    fht_run(); // process the data in the fht
    fht_mag_log(); // take the output of the fht
    sei();
}

float running_avg[FHT_N/2];
float smoothed_read[FHT_N/2];

class RepeatTracker {
 private:
  int init_reads = 0;
  int last_max_index = 0;
  int last_max_repeat = 0;
  
 public:
  RepeatTracker() {
    for (int i = 0; i < FHT_N/2; i++) {
      running_avg[i] = 0;
      smoothed_read[i] = 0;
    }
  }

  bool read() {
    if (init_reads < INIT_READS) {
      for (byte i = 0; i < FHT_N/2; i++) {
    	running_avg[i] += fht_log_out[i];
      }
      init_reads += 1;
    } else if (init_reads == INIT_READS) {
      init_reads += 1;
      for (byte i = 0; i < FHT_N/2; i++) {
    	smoothed_read[i] = fht_log_out[i];
    	running_avg[i] = running_avg[i] / INIT_READS;
      }
      init_reads += 1;
    } else {
      float max_value = 0;
      int max_index = -10;
      for (byte i = 0; i < FHT_N/2; i++) {
	byte val = fht_log_out[i];
    	smoothed_read[i] = ((SMOOTH)*smoothed_read[i] + (1 - SMOOTH)*val);
    	running_avg[i] = (RUNNING_AVG_SMOOTH*running_avg[i] + (1 - RUNNING_AVG_SMOOTH)*val);

    	float delta_val = smoothed_read[i] - running_avg[i];
    	if (delta_val > max_value) {
    	  max_value = delta_val;
    	  max_index = i;
	}
      }
      Serial.print("HORT: ");
      Serial.print(max_index);
      Serial.print(" ");
      Serial.println(max_value);
      if (max_value < 18) {
	last_max_index = -4;
	last_max_repeat = 0;
      } else {
	if (abs(last_max_index - max_index) < 3) {
	  last_max_repeat += 1;
	  if (last_max_repeat > 2) {
	    return true;
	  }
	} else {
	  last_max_repeat = 0;
	}
	last_max_index = max_index;
      }
    }
    return false;
  }
};


/* // Which pin on the Arduino is connected to the NeoPixels? */
/* // On a Trinket or Gemma we suggest changing this to 1 */
#define PIN            6

/* // How many NeoPixels are attached to the Arduino? */
#define NUMPIXELS      16
#define DELAY_MILLIS   50
#define MAX_BRIGHT    150
#define R 0
#define G 1
#define B 2

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup_lights() {
  pixels.begin();
}

byte target_color[NUMPIXELS][3];
byte current_color[NUMPIXELS][3];

class PixelManager {
 public:
  PixelManager() {
  }

  void setup() {
    Serial.println("Almost new targ");
    new_random_targets();
    Serial.println("Almost update");
    update_pixels();
    Serial.println("Done");
  }

  void new_random_targets() {
    target_color[0][R] = random(MAX_BRIGHT);
    target_color[0][G] = random(MAX_BRIGHT);
    target_color[0][B] = random(MAX_BRIGHT);

    Serial.println("HOOTY");
    Serial.print("R: ");
    Serial.print(target_color[0][R]);
    Serial.print(" G: ");
    Serial.print(target_color[0][G]);
    Serial.print(" B: ");
    Serial.println(target_color[0][B]);

    // Everything moves towards the main color.
    for (int i = 1; i < NUMPIXELS; i++) {
      target_color[i][R] = target_color[0][R];
      target_color[i][G] = target_color[0][G];
      target_color[i][B] = target_color[0][B];
    }
    perturb_targets(16);
  }

  int perturb(int delta, int current) {
    int next_color = current + random(delta) - delta / 2;
    if (next_color > MAX_BRIGHT) {
      next_color = MAX_BRIGHT;
    } else if (next_color < 0) {
      next_color = 0;
    }
    return next_color;
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
	current_color[i][j] += (target_color[i][j] - current_color[i][j])/8;
      }
    }
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(current_color[i][R],
					   current_color[i][G],
					   current_color[i][B]));
    }
    pixels.show();
  }
};


void setup() {
  setup_fht();
  setup_lights();
}

void loop() {
  int init_reads = 0;
  int last_max_index = 0;
  int last_max_repeat = 0;
  RepeatTracker tracker;
  unsigned long last_output = millis();
  bool should_switch = false;

  PixelManager manager;
  manager.setup();

  while(1) {
    get_fht();
    should_switch |= tracker.read();
    if (millis() - last_output > DELAY_MILLIS) {
      last_output = millis();
      if (should_switch) {
    	should_switch = false;
    	manager.new_random_targets();
      }
      if (random(100) > 80) {
    	manager.perturb_targets(random(8));
      }
      manager.update_pixels();
    }
  }
}
