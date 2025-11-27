# Controlador Máquina Expendedora

## Introducción

El objetivo de la práctica es crear una **máquina expendedora** totalmente funcional usando un Arduino Uno.  
El sistema integra sensores, actuadores e interacción con el usuario mediante una **máquina de estados**.  
Todo se controla sin `delay()` para mantener el sistema reactivo, usando **millis()**, **interrupciones**, **threads cooperativos** y **watchdog**.

---

## Arquitectura general

Los **componentes principales** del sistema son:

- LCD 16x2  
- Joystick con pulsador  
- LED verde  
- LED rojo  
- Botón externo  
- Sensor DHT11  
- Sensor HC-SR04  
- Arduino Uno

---

## Recursos Software

El programa se estructura mediante una **máquina de estados**, controlada por:

```cpp
enum STATES {
  INIT,
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
```

Se emplean:

- Temporización con `millis()`
- Interrupciones en el botón y el joystick  
- Threads cooperativos (`Thread.h`)  
- Watchdog (`avr/wdt.h`) para reinicios automáticos  

---

## Inicio

Al iniciar:

- El LED rojo parpadea 3 veces  
- El LCD muestra `[CARGANDO...]`  
- Se configuran interrupciones  
- Se pasa a `WAIT_FOR_CLIENT`  

### Parpadeo sin bloqueos

```cpp
void parpadear_led() {
  if (clock_timer(last_led1_time, LED_INTERVAL)) {
    led_1_state = !led_1_state;
    iteracion++;
  }
  digitalWrite(LED_RED_PIN, led_1_state);
}
```

### Temporizador no bloqueante

```cpp
bool clock_timer(unsigned long &last, unsigned long interval) {
  unsigned long now = millis();
  if (now - last >= interval) {
    last = now;
    return true;
  }
  return false;
}
```

---

## Servicio

### 1. Detección de cliente

```cpp
dhtThread.onRun(dhtCallback);
dhtThread.setInterval(DHT_INTERVAL);
```

Lectura HC-SR04:

```cpp
long read_distance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;

  return duration * 0.0343f / 2.0f;
}
```

---

### 2. Mostrar temperatura y humedad

```cpp
lcd.setCursor(0, 0);
lcd.print("Temp: ");
lcd.print(temperature);
lcd.print("C");

lcd.setCursor(0, 1);
lcd.print("Humd: ");
lcd.print(humidity);
lcd.print("%");
```

---

### 3. Menú de bebidas

```cpp
int joystick(byte axis) {
  int value = analogRead(axis == 1 ? X_JOYSTICK : Y_JOYSTICK);
  if (value <= 400) return 1;
  if (value > 900) return -1;
  return 0;
}
```

---

### 4. Dispensado

```cpp
brightness = (elapsed * 255) / total_ms;
analogWrite(LED_GREEN_PIN, brightness);
lcd.print("RETIRE BEBIDA");
```

---

## Modo Administrador

```cpp
if (pressed) {
  pressed = false;
  byte action_admin = action_from_option(action);
}
```

```cpp
if (lecture == -1) price += 0.05;
else if (lecture == 1) price -= 0.05;
```

---

## Esquemático del circuito

Incluye un diagrama creado en Fritzing o Tinkercad mostrando todas las conexiones de:

- LCD  
- Joystick  
- LED verde y rojo  
- Botón externo  
- DHT11  
- HC-SR04  
<img width="624" alt="imagen" src="https://github.com/user-attachments/assets/9c5ce90a-8a6a-49a2-b314-4e8a3cb46602" />

---

## Vídeo de demostración

Incluye un vídeo explicando funcionamiento y lógica del sistema.  

**Enlace al vídeo:**  
[[https://youtu.be/TU_LINK_AQUI](https://youtu.be/TU_LINK_AQUI)](https://img.youtube.com/vi/IHMvAtmqGPg/maxresdefault.jpg
)

---

## Conclusiones

La práctica ha permitido integrar sensores, actuadores y una arquitectura basada en máquina de estados.  
Se ha trabajado con programación reactiva sin bloqueos, interrupciones, threads cooperativos y watchdog para asegurar un sistema robusto y estable.
