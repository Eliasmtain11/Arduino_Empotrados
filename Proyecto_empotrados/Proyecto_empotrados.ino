#include <LiquidCrystal.h>
#include "DHT.h"
#include <Thread.h>
#include <avr/wdt.h>

#define DHTTYPE DHT11 
#define DHT11PIN A5
#define ECHO_PIN A4
#define TRIG_PIN A3
#define Y_JOYSTICK A2
#define X_JOYSTICK A1
#define LED_RED_PIN 11
#define LED_GREEN_PIN 10
#define BUTTON_JOYSTICK 3
#define BUTTON 2
#define SECONDS_TO_MILI 1000UL
#define LED_INTERVAL 500            
#define SERVICE_DURATION 5000       
#define DHT_INTERVAL 1000          
#define TAKE_DRINK_TIME 3000
#define MIN_VALID_TIME_PRESSED 500UL
#define SHOW_DISTANCE_TIME 300

LiquidCrystal lcd(9,8,7,6,5,4);
DHT dht(DHT11PIN, DHTTYPE);

// Thread que actualiza sensor DHT sin bloquear loop()
Thread dhtThread;

unsigned long last_led1_time = 0;
unsigned long timer = 0;
unsigned long inicio = 0;
unsigned long fin = 0;
unsigned long last_change_time = 0;    

// Máquina de estados principal
enum STATES {
  INIT = 0,
  WAIT_FOR_CLIENT,
  MENU_COFFEE,
  COFFEE_SELECTED,
  COFFEE_FINISHED,
  ADMIN,
  DISTANCE,
  TEMP_HUM,
  TIME,
  PRICES,
  MODIFY
};

byte state;

byte euro[8] = { ... };
byte arrow_up[8] = { ... };
byte arrow_down[8] = { ... };

// Arrays de cafés, precios y opciones admin
char* coffees[] = {"Cafe Solo", "Cafe Cortado", "Cafe Doble", "Cafe Premium", "Chocolate"};
float prices[] = {1.00, 1.10, 1.25, 1.50, 2.00};
char* options_admin_first_row[] = {"Ver Temperatura", "Ver distancia ", "Ver contador", "Modificar "};
char* options_admin_second_row[] = {"","sensor","","precios"};

float new_price = 0;

volatile bool change = false;     // Control pulsación botón grande
volatile bool pressed = false;    // Control botón joystick
bool modify_selected = false;
bool modifing_price = false;
bool person_detected = false;
bool led_1_state = false;
bool block_left = false;

int last_state = 0;
int lecture = 0;
int i = 0;
int j = 0;

byte brightness;
byte time = 0;
byte iteracion = 0;

enum AXIS { X_AXIS = 1, Y_AXIS = 2 };

// ISR botón principal
void buttonISR() {
  change = true;   
}

// ISR click del joystick
void joystickISR() {
  pressed = true;   
}

void setup() {
  wdt_disable();
  wdt_enable(WDTO_2S);  // Watchdog evita bloqueos

  lcd.begin(16,2);
  lcd.createChar(0, euro);  
  lcd.createChar(1, arrow_up); 
  lcd.createChar(2, arrow_down); 
  Serial.begin(9600);
  dht.begin();

  pinMode(BUTTON_JOYSTICK, INPUT_PULLUP);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  dhtThread.onRun(dhtCallback); 
  dhtThread.setInterval(DHT_INTERVAL);  
  dhtThread.enabled = false;

  state = INIT;   // Estado inicial
}

void loop() {
  button_timing();   // Manejo del botón sin delay()

  if (dhtThread.shouldRun()) {   // Thread del DHT
    dhtThread.run();
  }

  // Reset de pulsación fuera de menús
  if (state != MENU_COFFEE || state != ADMIN || state != PRICES) {
    pressed = false;
  }

  // Máquina de estados no bloqueante
  if (state == INIT) start();
  else if(state == WAIT_FOR_CLIENT) wait_and_temp_hum();
  else if (state == MENU_COFFEE) coffee_menu();
  else if (state == COFFEE_SELECTED) preprare_coffee();
  else if (state == COFFEE_FINISHED) extract_coffee();
  else if(state == ADMIN) menu_admin();
  else if (state == DISTANCE) show_distance();
  else if (state == TEMP_HUM) show_temp_hum();
  else if (state == TIME) clock();
  else if (state == PRICES) menu_prices();

  wdt_reset();  // Mantiene el watchdog feliz
}

// Pantalla inicial con parpadeo del LED
void start() {
  lcd.setCursor(0,0);
  lcd.print("CARGANDO...      ");
  parpadear_led();

  if (iteracion >= 6) {
    state = WAIT_FOR_CLIENT;
    person_detected = false;
    lcd.clear();
    attachInterrupt(digitalPinToInterrupt(BUTTON), buttonISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(BUTTON_JOYSTICK), joystickISR, FALLING);
  }
}

// Espera cliente midiendo distancia con ultrasonidos
void wait_and_temp_hum() {
  if (!person_detected) {
    long dist = read_distance();
    
    lcd.setCursor(0,0);
    lcd.print("    ESPERANDO    ");
    lcd.setCursor(0,1);
    lcd.print("     CLIENTE     ");

    if (dist > 0 && dist < 100) {     // Cliente detectado
      timer = millis();
      person_detected = true;
      dhtThread.enabled = true;       // Activa thread DHT
      lcd.clear();
    }
  }
}

// Menú de cafés controlado con joystick
void coffee_menu() {
  byte len = sizeof(coffees) / sizeof(coffees[0]);
  lecture = joystick(Y_AXIS);

  if (last_state != lecture) {
    i += lecture;           // Navegación
    last_state = lecture;
  }

  if (i < 0) i = len - 1;
  else if (i > len - 1) i = 0;

  lcd.setCursor(0, 0);
  lcd.print(coffees[i]);
  lcd.setCursor(11, 1);
  lcd.print(prices[i]);
  lcd.write(byte(0));

  if(pressed) {             // Selección de café
    pressed = false;
    state = COFFEE_SELECTED;
    time = random(4,8);     // Duración aleatoria
    brightness = 0;
    analogWrite(LED_GREEN_PIN, brightness);
    timer = millis();
    lcd.clear();
  }
}

// Simulación progreso café mediante PWM
void preprare_coffee() {
  unsigned long total_ms = time * SECONDS_TO_MILI;
  unsigned long elapsed  = millis() - timer;

  if (elapsed < total_ms) {
    brightness = (elapsed * 255) / total_ms;
    if (brightness > 255) brightness = 255;
    analogWrite(LED_GREEN_PIN, brightness);
    lcd.print("Preparando cafe...");
  } else {
    timer = millis();
    lcd.clear();
    state = COFFEE_FINISHED;
  }
}

// Pantalla tomar bebida
void extract_coffee() {
  if(!clock_timer(timer,TAKE_DRINK_TIME)) {
    lcd.print("RETIRE BEBIDA");
 } else {
    lcd.clear();
    person_detected = false;
    state = WAIT_FOR_CLIENT;
  }
}

// Menú administrador
void menu_admin() {
  byte len = sizeof(options_admin_first_row) / sizeof(options_admin_first_row[0]);
  lecture = joystick(Y_AXIS);

  if (last_state != lecture) {
    i += lecture;
    last_state = lecture;
  }

  if (i < 0) i = len - 1;
  else if (i > len - 1) i = 0;

  lcd.print(options_admin_first_row[i]);
  lcd.setCursor(0,1);
  lcd.print(options_admin_second_row[i]);

  if(pressed) {
    pressed = false;
    char action[32];
    strcpy(action, options_admin_first_row[i]);
    strcat(action, options_admin_second_row[i]);
    byte action_admin = action_from_option(action);

    if(action_admin == 0) state = TEMP_HUM;
    else if(action_admin == 1) state = DISTANCE;
    else if(action_admin == 2) state = TIME;
    else if(action_admin == 3) state = PRICES;

    lcd.clear();
  }
}

// Gestión de menú de precios
void menu_prices() {
  byte len = sizeof(coffees) / sizeof(coffees[0]);
  lecture = joystick(Y_AXIS);
  int x = joystick(X_AXIS);

  if(!modify_selected) {
    if (last_state != lecture) {
      j += lecture;
      last_state = lecture;
    }

    if (j < 0) j = len - 1;
    else if (j > len - 1) j = 0;

    lcd.setCursor(0, 0);
    lcd.print(coffees[j]);
    lcd.setCursor(11, 1);
    lcd.print(prices[j]);
    lcd.write(byte(0));

    if (block_left && x == 0) block_left = false;
    else if (x == -1) {
      state = ADMIN;
      lcd.clear();
    }
  }

  if(modify_selected){
    new_price = modify_price(coffees[j],new_price);
    if(modifing_price){
      modifing_price = false;
      prices[j] = new_price;   // Guarda precio
    }
  }
  else if(pressed){
    pressed = false;
    modify_selected = true;
    new_price = prices[j];
    lcd.clear();
  }
}

// Modificar precio usando joystick
float modify_price(char* type, float price) {
  lecture = joystick(Y_AXIS);

  if (last_state != lecture) {
    if(lecture == -1) price += 0.05;
    else if(lecture == 1) price = max(0.0f, price - 0.05);
    last_state = lecture;
  }

  lcd.setCursor(3,0);
  lcd.print(type);
  lcd.setCursor(6,1);
  lcd.print(price);

  if(pressed){
    pressed = false;
    modify_selected = false;
    modifing_price = true;
    lcd.clear();
  }
  else if(joystick(X_AXIS) == -1) {
    state = PRICES;
    modify_selected = false;
    block_left = true;
    lcd.clear();
  }

  return price;
}

// Muestra distancia con el sensor
void show_distance() {
  if(clock_timer(timer,SHOW_DISTANCE_TIME)){
    long dist = read_distance();
    lcd.print("Distance");
    lcd.setCursor(6, 1);
    if(dist == -1) lcd.print("None");
    else lcd.print(dist);
  }

  if(joystick(X_AXIS) == -1) {
    state = ADMIN;
    lcd.clear();
  }
}

// Pantalla de temperatura y humedad
void show_temp_hum() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C");
  lcd.setCursor(0,1);
  lcd.print("Humd: ");
  lcd.print(humidity);
  lcd.print("%");

  if(joystick(X_AXIS) == -1) {
    state = ADMIN;
    lcd.clear();
  }
}

// Muestra contador global
void clock() {
  unsigned long counter = millis();
  lcd.print("CONTADOR");
  lcd.setCursor(7,1);
  lcd.print(counter/SECONDS_TO_MILI);

  if(joystick(X_AXIS) == -1) {
    state = ADMIN;
    lcd.clear();
  }
}

// Traduce texto a acción admin
byte action_from_option(char* accion) {
  if (strcmp(accion, "Ver Temperatura") == 0) return 0;
  else if (strcmp(accion, "Ver distancia sensor") == 0) return 1;
  else if (strcmp(accion, "Ver contador") == 0) return 2;
  else if (strcmp(accion, "Modificar precios") == 0) return 3;
}

// Manejo del botón principal con tiempos de pulsación
void button_timing() {
  if (!change) return;             
  change = false;  
  
  int val = digitalRead(BUTTON);  
  if (val == LOW) {
    inicio = millis(); // botón presionado
    return;
  }

  fin = millis();
  unsigned long time_pressed = fin - inicio;

  if(time_pressed > MIN_VALID_TIME_PRESSED) {
    if (time_pressed >= 2000 && time_pressed < 3000 && state != ADMIN) {
      state = WAIT_FOR_CLIENT;  // Servicio
      person_detected = false;
      i = 0;
      lcd.clear();
    }

    else if (time_pressed >= 5000) {  // Admin
      if(state != ADMIN) {
        state = ADMIN;
        i = 0;
      }
      else {
        state = WAIT_FOR_CLIENT;
        person_detected = false;
        i = 0;
      }
      lcd.clear();
    }
}

}

// Thread que muestra temp/hum durante 5 segundos
void dhtCallback() {
  if (!person_detected) {
    dhtThread.enabled = false;
    return;
  }
  
  if (clock_timer(timer, SERVICE_DURATION)) {
    state = MENU_COFFEE;
    dhtThread.enabled = false;
    lcd.clear();
    return;
  }

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.setCursor(0,1);
  lcd.print("Humd: ");
  lcd.print(humidity);
}

void parpadear_led() {
  if (clock_timer(last_led1_time, LED_INTERVAL)) {
    led_1_state = !led_1_state;
    iteracion++;
  }
  digitalWrite(LED_RED_PIN, led_1_state ? HIGH : LOW);
}

// Temporizador no bloqueante
bool clock_timer(unsigned long &last, unsigned long interval) {
  unsigned long now = millis();
  if (now - last >= interval) {
    last = now;
    return true;
  }
  return false;
}

// Lectura del sensor HC-SR04
long read_distance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) return -1;
  float distance = duration * 0.0343f / 2.0f;
  return distance;  
}

// Lectura del joystick: devuelve 1, 0 o -1
int joystick(byte axis) {
  int value = (axis == 1) ? analogRead(X_JOYSTICK) : analogRead(Y_JOYSTICK);

  if(value <= 400) return 1;
  else if(value > 900) return -1;
  return 0;
}
