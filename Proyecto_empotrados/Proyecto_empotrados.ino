#include <LiquidCrystal.h>
#include "DHT.h"
#include <Thread.h>

#define DHTTYPE DHT11 
#define DHT11PIN A5
#define ECHO_PIN A4
#define TRIG_PIN A3
#define Y_JOYSTICK A2
#define X_JOYSTICK A1
#define LED_RED_PIN 11
#define LED_GREEN_PIN 10
#define BUTTON_JOYSTICK 12
#define BUTTON 2
#define SECONDS_TO_MILI 1000UL
#define LED_INTERVAL 500            
#define SERVICE_DURATION 5000       
#define DHT_INTERVAL 1000          
#define TAKE_DRINK_TIME 3000
#define DEBOUNCE_MS 50UL

LiquidCrystal lcd(9,8,7,6,5,4);
DHT dht(DHT11PIN, DHTTYPE);

Thread dhtThread;

unsigned long last_led1_time = 0;
unsigned long timer = 0;
unsigned long inicio = 0;
unsigned long fin = 0;
unsigned long last_change_time = 0;    

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

};

byte state;

byte euro[8] = {
  B00110,
  B01001,
  B11100,
  B01000,
  B11100,
  B01001,
  B00110,
  B00000
};

char* coffees[] = {"Cafe Solo", "Cafe Cortado", "Cafe Doble", "Cafe Premium", "Chocolate"};
float prices[] = {1.00, 1.10, 1.25, 1.50, 2.00};
char* options_admin_first_row[] = {"Ver Temperatura", "Ver distancia ", "Ver contador", "Modificar "};
char* options_admin_second_row[] = {"","sensor","","precios"};
bool person_detected = false;
bool led_1_state = false;
volatile bool change = false;   


int last_state = 0;
int lecture = 0;
int i = 0;


byte brightness;
byte time = 0;
byte iteracion = 0;


enum AXIS {
 X_AXIS = 1,
 Y_AXIS = 2
};

void buttonISR() {
  change = true;   
}

void setup() {
  lcd.begin(16,2);
  lcd.createChar(0, euro);  
  Serial.begin(9600);
  dht.begin();

  pinMode(BUTTON_JOYSTICK, INPUT_PULLUP);
  pinMode(BUTTON, INPUT_PULLUP);

  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);

  pinMode(ECHO_PIN, INPUT);
  pinMode(Y_JOYSTICK, INPUT);
  pinMode(X_JOYSTICK, INPUT);

  dhtThread.onRun(dhtCallback); 
  dhtThread.setInterval(DHT_INTERVAL);  
  dhtThread.enabled = false;            
  randomSeed(analogRead(A0)); 
  state == INIT;
}

void loop() {
  button_timing();
  if (dhtThread.shouldRun()) {
    dhtThread.run();
  }
  if (state == INIT) {
    start();
  }
  else if(state == WAIT_FOR_CLIENT) {
    wait_and_temp_hum();
  }
  else if (state == MENU_COFFEE) {
    coffee_menu();
  }
  else if (state == COFFEE_SELECTED) {
    preprare_coffee();
  }
  else if (state == COFFEE_FINISHED) {
    extract_coffee();
  }
  else if(state == ADMIN) {
    menu_admin();
  }
  else if (state == DISTANCE) {
    show_distance();
  }
  else if (state == TEMP_HUM) {
    show_temp_hum();
  }
  else if (state == TIME) {
    clock();
  }
}

void start() {
  lcd.setCursor(0,0);
  lcd.print("CARGANDO...      ");
  parpadear_led();

  if (iteracion >= 6) {
    state = WAIT_FOR_CLIENT;
    person_detected = false;
    attachInterrupt(digitalPinToInterrupt(BUTTON), buttonISR, CHANGE);
    lcd.clear();
  }
}

void wait_and_temp_hum() {
  if (!person_detected) {
    digitalWrite(LED_GREEN_PIN,LOW);
    digitalWrite(LED_RED_PIN,LOW);
    long dist = read_distance();
    
    lcd.setCursor(0,0);
    lcd.print("    ESPERANDO    ");
    lcd.setCursor(0,1);
    lcd.print("     CLIENTE     ");

    if (dist > 0 && dist < 100) {
    timer = millis();  
    person_detected = true;
    dhtThread.enabled = true;
    i = 0;            
    lcd.clear();
    }
  }
}

void coffee_menu() {
  byte len = sizeof(coffees) / sizeof(coffees[0]);
  byte joystick_button_state = digitalRead(BUTTON_JOYSTICK);
  lecture = joystick(Y_AXIS);
  if (last_state != lecture) {
        i += lecture;
        last_state = lecture;
      }
      if (i < 0){
        i = len - 1;
      }
      else if (i > len - 1){
        i = 0;
      }
      lcd.setCursor(0, 0);
      lcd.print(coffees[i]);
      lcd.print("                ");
      lcd.setCursor(11, 1);
      lcd.print((prices[i]));
      lcd.write(byte(0));

      if(joystick_button_state == LOW){

        state = COFFEE_SELECTED;

        time = random(4,8);
        brightness = 0;
        analogWrite(LED_GREEN_PIN, brightness);

        timer = millis();
        Serial.println(time);
        lcd.clear();
      }
}

void preprare_coffee() {

  unsigned long total_ms = (unsigned long)time * SECONDS_TO_MILI; 
  unsigned long elapsed  = millis() - timer;                      

  if (elapsed < total_ms) {

    brightness = (elapsed * 255) / total_ms;
    if (brightness > 255){
      brightness = 255;
    }
    analogWrite(LED_GREEN_PIN, brightness);

    lcd.setCursor(3,0);
    lcd.print("Preparando   ");
    lcd.setCursor(3,1);
    lcd.print("cafe...         ");

  } else {
    timer = millis();
    lcd.clear();
    state = COFFEE_FINISHED;
  }
}

void extract_coffee() {
  if(!(temporizador(timer,TAKE_DRINK_TIME))) {
    lcd.setCursor(5,0);
    lcd.print("RETIRE   ");
    lcd.setCursor(5,1);
    lcd.print("BEBIDA         ");
 }
  else {
    lcd.clear();
    person_detected = false;
    state = WAIT_FOR_CLIENT;
  }
}

void menu_admin() {
  digitalWrite(LED_RED_PIN, HIGH);
  digitalWrite(LED_GREEN_PIN, HIGH);
  byte len = sizeof(options_admin_first_row) / sizeof(options_admin_first_row[0]);
  byte joystick_button_state = digitalRead(BUTTON_JOYSTICK);
  lecture = joystick(Y_AXIS);
  if (last_state != lecture) {
    i += lecture;
    last_state = lecture;
    }
    if (i < 0){
      i = len - 1;
    }
    else if (i > len - 1){
      i = 0;
    }
    lcd.setCursor(0, 0);
    lcd.print(options_admin_first_row[i]);
    lcd.print("                ");
    lcd.setCursor(0,1);
    lcd.print(options_admin_second_row[i]);    
    lcd.print("                ");

    if(joystick_button_state == LOW){
      char action[32];
      strcpy(action, options_admin_first_row[i]);
      strcat(action, options_admin_second_row[i]);
      byte action_admin = action_from_option(action);
      if(action_admin == 0) {
        state = TEMP_HUM;
      }
      else if(action_admin == 1) {
        state = DISTANCE;
      }
      else if(action_admin == 2) {
        state = TIME;
      }
      lcd.clear();
    }
}

void show_distance() {
  long dist = read_distance();
  lcd.setCursor(4, 0);
  lcd.print("Distance");
  lcd.print("                ");
  lcd.setCursor(6, 1);
  lcd.print(dist);
  lcd.print("cm         ");
  if(joystick(X_AXIS)) {
    state = ADMIN;
    lcd.clear();
  }
}

void show_temp_hum() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  lcd.setCursor(0,0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C   ");

  lcd.setCursor(0,1);
  lcd.print("Humd: ");
  lcd.print(humidity);
  lcd.print("%   ");
  if(joystick(X_AXIS)) {
    state = ADMIN;
    lcd.clear();
  }
}

void clock() {
  unsigned long counter = millis();
  lcd.setCursor(4,0);
  lcd.print("CONTADOR");
  lcd.setCursor(7,1);
  lcd.print(counter/SECONDS_TO_MILI);
  lcd.print("s    ");

   if(joystick(X_AXIS)) {
    state = ADMIN;
    lcd.clear();
  }
}

byte action_from_option(char* accion) {
  if (strcmp(accion, "Ver Temperatura") == 0) {
    return 0;
  }
  else if (strcmp(accion, "Ver distancia sensor") == 0) {
    return 1;
  }
  else if (strcmp(accion, "Ver contador") == 0) {
    return 2;
  }
  else if (strcmp(accion, "Modificar precios") == 0) {
    return 3;
  }
}

void button_timing() {
  if (!change) {
    return;             
  }
  change = false;  

  unsigned long now = millis();
  if (now - last_change_time < DEBOUNCE_MS) {
    return;
  }
  last_change_time = now;  

  int val = digitalRead(BUTTON);  
  if (val == LOW) {
    inicio = millis();
    return;             
  }

  fin = millis();
  unsigned long time_pressed = fin - inicio;

  Serial.print("Tiempo pulsado (ms): ");
  Serial.println(time_pressed);

  if (time_pressed >= 2000 && time_pressed < 3000 && state != ADMIN) {
    Serial.println("Servicio");
    state = WAIT_FOR_CLIENT;
    person_detected = false;
    lcd.clear();
  }

  else if (time_pressed >= 5000) {
    Serial.println("Admin");
    if(state != ADMIN) {
      state = ADMIN;
    }
    else {
     state = WAIT_FOR_CLIENT;
    person_detected = false;
    }
    lcd.clear();
  }
}

void dhtCallback() {
  if (!person_detected) {
    dhtThread.enabled = false;
    return;
  }
  
  if (temporizador(timer, SERVICE_DURATION)) {
    state = MENU_COFFEE;
    dhtThread.enabled = false;
    lcd.clear();
    return;
  }

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  lcd.setCursor(0,0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C   ");

  lcd.setCursor(0,1);
  lcd.print("Humd: ");
  lcd.print(humidity);
  lcd.print("%   ");

}

void parpadear_led() {
  if (temporizador(last_led1_time, LED_INTERVAL)) {
    led_1_state = !led_1_state;
    iteracion++;
  }
  if (led_1_state) {
      digitalWrite(LED_RED_PIN, HIGH);
  } else {
      digitalWrite(LED_RED_PIN, LOW);
  }

}

bool temporizador(unsigned long &last, unsigned long interval) {
  unsigned long now = millis();
  if (now - last >= interval) {
    last = now;
    return true;
  }
  return false;
}

long read_distance() {
  long duration;
  float distance;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout de 30 ms

  if (duration == 0) {
    return -1;
  }
  distance = duration * 0.0343f / 2.0f;

  return distance;  
}

int joystick(byte axis) {
  int value = 0;
  int n = 0;

  if(axis == 1){
    value = analogRead(X_JOYSTICK);
  }
  else if(axis == 2) {
    value = analogRead(Y_JOYSTICK);
  }

  if(value <= 400) {
    n = -1;
  }

  else if(value > 400 && value <= 900) {
    n = 0;
  }

  else if(value > 900) {
    n = 1;
  }
  return n;
}
