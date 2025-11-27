# Controlador Máquina Expendedora

## Introducción

El objetivo de la práctica es crear una **máquina expendedora**.  
El sistema integra varios componentes (sensores, actuadores y una interfaz de usuario) y se organiza mediante una **máquina de estados**, evitando el uso de `delay()` para mantener el programa siempre reactivo.

## Arquitectura

Los **componentes principales** del sistema son:

- `LCD 16x2`  
- `Joystick`  
- `LED verde`  
- `LED rojo`  
- `Botón` externo  
- `Sensor de temperatura y humedad (DHT11)`  
- `Sensor de ultrasonidos (HC-SR04)`  
- `Arduino Uno`

## Recursos Software

El programa se estructura en **estados** controlados por una variable global `state`.  
Cada estado representa una fase distinta del funcionamiento de la máquina:

- Estado de inicio / arranque  
- Estados de servicio (detección de cliente, información, menú, dispensado)  
- Estado de administración (admin)

A lo largo del programa se utilizan:

- **Temporización con `millis()`** (sin `delay()`)  
- **Interrupciones** para el botón y el joystick  
- **Threads** (con `Thread.h`) para tareas periódicas como la lectura del sensor de ultrasonidos  
- **Watchdog** (`avr/wdt.h`) para añadir robustez ante posibles bloqueos

### Inicio

En el estado inicial, el sistema realiza dos acciones:

- El `LED rojo` parpadea **3 veces**.  
- La pantalla LCD muestra el mensaje `[CARGANDO...]`.

Para implementar esto, **no se utiliza `delay()`**, ya que bloquearía el flujo del programa y haría que el sistema dejara de ser reactivo.  
En su lugar:

- Se usa la función `millis()`, que devuelve el tiempo transcurrido desde que se encendió la placa.  
- Se ha creado una función `clock_timer(...)` que recibe como parámetro un intervalo de tiempo y comprueba si ha transcurrido o no.  
- De esta manera, el parpadeo del LED rojo y el mensaje de carga se gestionan de forma **no bloqueante**.

Una vez que el LED rojo termina de parpadear, se actualiza la variable `state` para pasar al siguiente estado (inicio del servicio).

Las **interrupciones** del botón normal y del pulsador del joystick se **inicializan en este momento**, y no antes, por dos razones principales:

1. Si se activan en el `setup()`, existe la posibilidad de que, si el usuario pulsa el botón justo al arrancar, se “salte” la fase de `CARGANDO...` y `ESPERANDO CLIENTE` y el sistema pase directamente al modo servicio o admin.  
2. No se pueden desactivar las interrupciones sin afectar a `millis()`, ya que si se desactivan las interrupciones del microcontrolador, `millis()` deja de actualizarse.

Por tanto, la solución adoptada es **activar las interrupciones después de la secuencia de arranque**, cuando el sistema ya está estable y listo para interactuar con el usuario.

### Servicio

La parte de servicio se ha dividido en **cuatro estados** principales:

1. **Detección de cliente (sensor ultrasonidos)**  
   - Se utiliza un **thread** para leer periódicamente el sensor de ultrasonidos sin bloquear el programa.  
   - El sistema permanece en este estado hasta que detecta que alguien se acerca a menos de **1 metro** de distancia.  
   - Cuando esto ocurre, el estado cambia automáticamente al siguiente.

2. **Mostrar temperatura y humedad (DHT11)**  
   - En este estado, la pantalla LCD muestra la **temperatura** y la **humedad relativa** medida por el sensor DHT11.  
   - Esta información se mantiene en pantalla durante unos **5 segundos**, gestionados con `millis()` y la función `clock_timer(...)`.  
   - Tras esos 5 segundos, se cambia al estado del menú.

3. **Menú de selección de bebida (joystick)**  
   - En este estado, el display muestra un **menú de bebidas** con sus precios.  
   - La navegación por el menú se hace con el joystick:
     - Mover el joystick **hacia abajo** avanza en las opciones.  
     - Mover el joystick **hacia arriba** retrocede en las opciones.  
   - Para **seleccionar** una bebida, se pulsa el botón integrado en el joystick (detectado por interrupción).

4. **Dispensado de bebida (LED verde + texto)**  
   - Una vez elegida la bebida, se entra en un estado de “dispensando”:  
     - Durante un tiempo aleatorio entre **4 y 8 segundos** (controlado con `millis()`), el **LED verde incrementa progresivamente su intensidad** utilizando PWM (por ejemplo, con `analogWrite`), simulando el proceso de llenado.  
     - Cuando el LED llega a la intensidad máxima y se ha cumplido el tiempo aleatorio, el LCD muestra el mensaje `"RETIRE BEBIDA"`.  
   - Al finalizar este estado, se vuelve al estado de detección de cliente, completando el ciclo de servicio.

Además, en **cualquier momento del servicio**, si el usuario pulsa el botón **2–3 segundos**, el sistema interpreta la pulsación como un retorno manual y **vuelve al estado de servicio inicial**, permitiendo reiniciar el flujo sin necesidad de completar el proceso.

### Admin

El sistema cuenta con un **modo administrador** accesible con una pulsación larga del botón:

- Para entrar o salir del modo admin, es necesario pulsar el botón durante **5 segundos o más**.  
- Si se detecta una pulsación larga y **no estamos** en admin, se entra en el menú de administración.  
- Si ya estamos en admin y se vuelve a pulsar 5 segundos, se sale de este modo y se regresa al flujo normal.

Dentro del menú admin se pueden realizar varias acciones:

- Consultar la **distancia** medida por el sensor de ultrasonidos.  
- Ver la **temperatura y humedad** actual.  
- Consultar el **contador** de servicios (por ejemplo, cuántas bebidas se han dispensado).  
- **Modificar los precios** de las bebidas, subiéndolos o bajándolos de **5 en 5 céntimos** utilizando el joystick:
  - Mover el joystick hacia arriba/bajo cambia el precio.  
  - El pulsador del joystick se puede utilizar para confirmar o moverse entre campos.

Además, si el usuario pulsa el botón **2–3 segundos mientras está en admin**, el sistema cancela la administración y **vuelve directamente al servicio**, evitando tener que navegar hasta la opción de salida.

## Librerías empleadas

En el desarrollo del proyecto se han utilizado las siguientes librerías:

- `LiquidCrystal.h`  
  Para el control de la pantalla LCD (inicialización, posicionamiento del cursor, impresión de textos, etc.).

- `DHT.h`  
  Para la lectura del sensor de **temperatura y humedad**, gestionando el protocolo y el cálculo de los valores.

- `Thread.h`  
  Para implementar **threads cooperativos**, que permiten ejecutar tareas periódicas (como la lectura del sensor de ultrasonidos) sin bloquear el bucle principal.

- `avr/wdt.h`  
  Para configurar el **watchdog timer** y conseguir que el sistema se reinicie automáticamente en caso de bloqueo o fallo grave.

## Demostración del funcionamiento

[![Vídeo de demostración](https://img.youtube.com/vi/IHMvAtmqGPg/0.jpg)](https://www.youtube.com/watch?v=IHMvAtmqGPg)

## Esquemático del circuito

A continuación se muestra el **esquemático del circuito**, donde se pueden ver las conexiones de todos los componentes (LCD, joystick, LEDs, botón, sensor DHT, sensor de ultrasonidos y Arduino Uno):

<img width="624" alt="imagen" src="https://github.com/user-attachments/assets/b47fbb33-87e9-4d95-9651-2c41dc1c508e" />

## Conclusiones

La práctica ha servido para montar y programar una máquina expendedora completa usando Arduino Uno. Durante el proyecto se ha trabajado con una máquina de estados clara, temporización sin bloqueos usando millis(), interrupciones para que el sistema responda rápido, y threads para gestionar tareas periódicas sin cargar el loop(). También se ha añadido un watchdog para evitar bloqueos y un modo administrador con lectura de sensores y ajuste de precios.

En resumen, ha sido un proyecto muy útil para entender cómo combinar sensores, actuadores y lógica de control en un sistema embebido real, manteniendo siempre la reactividad y una interfaz sencilla para el usuario.
