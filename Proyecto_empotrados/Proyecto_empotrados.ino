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

#define LED_INTERVAL 500            
#define SERVICE_DURATION 5000       
#define DHT_INTERVAL 1000          
#define TAKE_DRINK_TIME 3000


LiquidCrystal lcd(9,8,7,6,5,4);
DHT dht(DHT11PIN, DHTTYPE);

Thread dhtThread;

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


byte state = 0;
bool person_detected = false;
bool menu_service = false;

unsigned long last_led_time = 0;
unsigned long service_start_time = 0;
unsigned long temp1 = 0;
unsigned long temp2 = 0;
unsigned long temp3 = 0;

bool led_1_state = false;
bool done = false;
bool coffee_selected = false;

byte iteracion = 0;
int last_state = 0;
int lectura = 0;
int i = 0;
int brillo_led = 0;
int brightness = 0;
int time = 0;
int step = 0;

char* cafes[] = {"Cafe Solo", "Cafe Cortado", "Cafe Doble", "Cafe Premium", "Chocolate"};
float precios[] = {1.00, 1.10, 1.25, 1.50, 2.00};

byte len = sizeof(cafes) / sizeof(cafes[0]);

enum AXIS {
 X_AXIS = 1,
 Y_AXIS = 2
};

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

  state = 0;
}
 // 67
void loop() {
  if (dhtThread.shouldRun()) {
    dhtThread.run();
  }

  if (state == 0) {
    INICIO();
  } else if (state == 1) {
    SERVICIO();
  }
}

void INICIO() {
  lcd.setCursor(0,0);
  lcd.print("CARGANDO...      ");
  parpadear_led();

  if (iteracion >= 6) {
    state = 1;
    person_detected = false;
    menu_service = false;
    lcd.clear();
  }
}

void SERVICIO() {
  if (!person_detected) {
    long dist = leerDistancia();

    lcd.setCursor(0,0);
    lcd.print("    ESPERANDO    ");
    lcd.setCursor(0,1);
    lcd.print("     CLIENTE     ");

    if (dist > 0 && dist < 100) {
      person_detected = true;
      menu_service = false;
      service_start_time = millis();  

      dhtThread.enabled = true;            
      lcd.clear();
    }

  } else {
    if (menu_service && person_detected) {
      byte joystick_button_state = digitalRead(BUTTON_JOYSTICK);

      if (last_state != lectura) {
        i += lectura;
        last_state = lectura;
      }
      lectura = joystick(Y_AXIS);
      Serial.println(i);
      if (i < 0){
        i = len - 1;
      }
      else if (i > len - 1){
        i = 0;
      }
      lcd.setCursor(0, 0);
      lcd.print(cafes[i]);
      lcd.print("                ");
      lcd.setCursor(11, 1);
      lcd.print((precios[i]));
      lcd.write(byte(0));

      if(joystick_button_state == LOW){
        coffee_selected = true;
        time = random(4,8);
        menu_service = false;
        done = false;
        step = 1024 / time;
        lcd.clear();
      }
    }
    if(coffee_selected){
      done = temporizador(temp1,time);
      if(temporizador(temp3,step)){
        analogWrite(LED_GREEN_PIN, brightness+=step);
      }
      if(done){
        if(temporizador(temp2,TAKE_DRINK_TIME)){
          person_detected = false;
        }
        lcd.setCursor(6,0);
        lcd.print("RETIRE   ");
        lcd.setCursor(6,1);
        lcd.print("BEBIDA         ");
      }

      lcd.setCursor(3,0);
      lcd.print("Preparando   ");
      lcd.setCursor(8,1);
      lcd.print("cafe         ");
      
    }
  }
}

long leerDistancia() {
  long duracion;
  float distancia;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duracion = pulseIn(ECHO_PIN, HIGH, 30000); // timeout de 30 ms

  if (duracion == 0) {
    return -1;
  }
  distancia = duracion * 0.0343f / 2.0f;

  return distancia;  
}

void parpadear_led() {
  if (temporizador(last_led_time, LED_INTERVAL)) {
    led_1_state = !led_1_state;
    iteracion++;
  }
  if (led_1_state) {
      digitalWrite(LED_RED_PIN, HIGH);
  } else {
      digitalWrite(LED_RED_PIN, LOW);
  }

}

void dhtCallback() {
  if (!person_detected || menu_service) {
    dhtThread.enabled = false;
    return;
  }

  unsigned long now = millis();
  if (now - service_start_time >= SERVICE_DURATION) {
    menu_service = true;
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

bool temporizador(unsigned long &last, unsigned long interval) {
  unsigned long now = millis();
  if (now - last >= interval) {
    last = now;
    return true;
  }
  return false;
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
