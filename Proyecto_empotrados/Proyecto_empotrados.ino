#include <LiquidCrystal.h>
#include <DHT11.h>

#define DHT11PIN A5
#define ECHO_PIN A4
#define TRIG_PIN A3
#define Y_JOYSTICK A2
#define X_JOYSTICK A1
#define LED_RED_PIN 11
#define LED_GREEN_PIN 10
#define BUTTON_JOYSTICK 12
#define BUTTON 2

#define INTERVALO 500
void INICIO();
void SERVICIO();
long leerDistancia();

LiquidCrystal lcd(9,8,7,6,5,4);
int state;
bool person_detected;

void setup() {
  lcd.begin(16,2);
  Serial.begin(9600);
  pinMode(BUTTON_JOYSTICK, INPUT_PULLUP);
  pinMode(BUTTON, INPUT_PULLUP);

  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);

  pinMode(DHT11PIN, INPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(Y_JOYSTICK, INPUT);
  pinMode(X_JOYSTICK, INPUT);
  state = 0;


}

void loop() {
  if(state == 0) {
    INICIO();
  }
  if(state == 1){
    SERVICIO();
  }



}

void INICIO() {
  lcd.setCursor(0,0);
  lcd.print("CARGANDO...           ");
  for (int i = 0; i < 3; i++) {
    unsigned long tiempo_inicio = millis();
    parpadear_led(tiempo_inicio);
  }
  lcd.setCursor(0,0);
  lcd.print("Para activar    ");
  lcd.setCursor(0,1);
  lcd.print("acercarse a 1m");
  state = 1;
  person_detected = false;
}

void SERVICIO() {
  
  if (person_detected == false) {
    long dist = leerDistancia();
    lcd.setCursor(0,0);
    lcd.print("ESPERANDO        ");
    lcd.setCursor(0,1);
    lcd.print("CLIENTE ");
    
    if (dist < 100 && dist > 0){
      person_detected = true;
    }
  }
  else {
    lcd.setCursor(0,1);
    lcd.print("TEMP:                  ");
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
  distancia = duracion * 0.0343 / 2;

  return distancia;  
}

void parpadear_led(unsigned long tiempo_inicio) {
  unsigned long temp_act = millis();

  // LED ON durante INTERVALO
  while (temp_act - tiempo_inicio < INTERVALO) {
    digitalWrite(LED_RED_PIN, HIGH);
    temp_act = millis();
  }

  // LED OFF durante INTERVALO
  tiempo_inicio = millis();
  temp_act = millis();  // IMPORTANTE: reiniciar también temp_act
  while (temp_act - tiempo_inicio < INTERVALO) {
    digitalWrite(LED_RED_PIN, LOW);
    temp_act = millis();
  }
}