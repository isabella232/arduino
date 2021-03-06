/* Talk to an ME007-ULS V1 using PWM.
 * The Arduino's usual pulseIn, which works fine with other
 * rangefinders, for some reason doesn't work with this device.
 * Thanks to timemage on #arduino for the solution!
 */

// Trigger pin can be any GPIO pin.
static constexpr int TRIGGER_PIN = 4;
// Echo pin MUST BE an intx capable pin: 2 or 3 for an UNO
static constexpr int ECHO_PIN = 3;

using t_edge_counter = uint16_t;
static volatile t_edge_counter g_edge_counter;

static volatile uint32_t start_micros;
static volatile uint32_t end_micros;

// For debugging: save times of both highs and lows
uint32_t lows[5];
int low_i = 0;
uint32_t highs[5];
int high_i = 0;

static void clear_edge_counter() {
  noInterrupts();
  g_edge_counter = 0;

  start_micros = end_micros = 0;
  low_i = high_i = 0;
  interrupts();
}

static t_edge_counter get_edge_counter() {
  t_edge_counter r;
  noInterrupts();
  r = g_edge_counter;
  interrupts();

  return r;
}

void edge_counter_isr() {
  if (start_micros == 0) {
    start_micros = micros();
  } else {
    end_micros = micros();
  }
  ++g_edge_counter;
  if (digitalRead(ECHO_PIN))
      highs[high_i++] = micros();
  else
      lows[low_i++] = micros();
}

static uint32_t get_micros_difference() {
  noInterrupts();
  uint32_t s = start_micros;
  uint32_t e = end_micros;
  Serial.print("Lows: ");
  int i;
  for (i=0; i<low_i; ++i) {
      Serial.print(lows[i]);
      Serial.print(", ");
  }
  Serial.println();
  Serial.print("Highs: ");
  for (i=0; i<high_i; ++i) {
      Serial.print(highs[i]);
      Serial.print(", ");
  }
  Serial.println();
  interrupts();

  return e - s;
}

void setup() {
  pinMode(ECHO_PIN, INPUT);

  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, HIGH);

  delay(2000);
  Serial.begin(9600);

  attachInterrupt(
    digitalPinToInterrupt(ECHO_PIN),
    edge_counter_isr,
    CHANGE
  );
  // Might be better to specify FALLING or RISING,
  // except it looks like the time we need is the time
  // between the two edges.
}

void loop() {
  clear_edge_counter();
  Serial.println(F("\n\n\nTrigger going LOW:"));
  digitalWrite(TRIGGER_PIN, LOW);

  // If we could track only rising or only falling edges,
  // then maybe we could start the timer here. But in practice
  // that gives times that are considerably too long.
  //start_micros = micros();

  delay(5000);

  Serial.print(F("5 seconds passed, counted "));
  Serial.print(get_edge_counter());
  Serial.println(F(" edges."));
  digitalWrite(TRIGGER_PIN, HIGH);

  uint32_t micdiff = get_micros_difference();
  Serial.print(F("Pulse length: "));
  Serial.println(micdiff);
  float inches = micdiff / 148.;
  Serial.print(F("Inches: "));
  Serial.println(inches);

  Serial.println(F("Sleeping"));
  delay(5000);
}
