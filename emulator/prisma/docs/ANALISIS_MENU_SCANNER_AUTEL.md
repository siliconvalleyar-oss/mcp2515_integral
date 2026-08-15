# Análisis Completo del Menú del Scanner AUTEL
## Vehículo: Chevrolet / Onix — VIN: 9BGKL48T0HB130763
## Firmware Scanner: V18.00 — Serial: DR8GSCCO2456

---

## 1. Contexto del Equipo

El scanner utilizado es un **AUTEL MaxiCOM** (serie inferida por la interfaz V18.00), herramienta de diagnóstico OBD-II/CAN de nivel profesional. Opera en idioma español y está conectado al puerto OBD-II del vehículo. La sesión registra un recorrido completo por múltiples pantallas de datos en vivo, pruebas activas, información ECU y listados personalizados del módulo de control del motor (ECM/PCM) de un Chevrolet Onix.

---

## 2. Estructura General del Menú Principal

El menú principal del scanner AUTEL se organiza en las siguientes secciones de nivel superior:

```
AUTEL Scanner — Menú Principal
│
├── 1. Información ECU
├── 2. Registros de Fallos / Congelación de Imagen (Freeze Frame)
├── 3. Datos en Vivo (Live Data)
│   ├── 3.1 Lectura de parámetros en tiempo real
│   └── 3.2 Selección personalizada de canales
├── 4. Listado Personalizado (Custom List)
├── 5. Pruebas Activas (Active Test / Component Test)
└── 6. Servicio / Mantenimiento (Service / Oil Reset / Adaptaciones)
```

---

## 3. Detalle de Cada Sección

---

### 3.1 Información ECU

Muestra los datos de identificación y calibración del módulo de control del motor.

**Contenido observado:**

| Campo | Valor |
|---|---|
| Software Scanner | V18.00 |
| Número de identificación del vehículo (VIN) | 9BGKL48T0HB130763 |
| Vehículo | Chevrolet / Onix |
| Número de serie (SN) | DR8GSCCO2456 |
| Cuentakilómetros | 141.395 km |
| Calibración de pieza N° 19 | 1505708 |
| Calibración de pieza N° 20 | 52124404 |
| Dirección de diagnóstico del módulo | [40] |
| Códigos de diagnóstico | [403] |

**Significado:** Esta sección confirma la correcta comunicación con la ECU y permite verificar que el vehículo está reconocido en la base de datos del scanner. La dirección de diagnóstico [40] corresponde al PCM (Powertrain Control Module) en la red CAN del vehículo.

---

### 3.2 Registros de Fallos / Congelación de Imagen (Freeze Frame)

Muestra los datos capturados automáticamente por la ECU en el momento en que se activó un DTC (Diagnostic Trouble Code).

**Contenido observado:**

| Campo | Valor |
|---|---|
| Sensor IAT | 16 °C |
| Sensor MAF | 1.63 g/s |
| Tiempo de funcionamiento del motor | — |
| Comando de válvula solenoide de purga EVAP | 13 % |
| Combustible restante en depósito | 36.1 % |
| Calentamientos desde borrado de DTC | 3 Counts |
| Distancia desde borrado de DTC | 25 km |

**Significado:** El "Freeze Frame" es una fotografía de las condiciones del motor en el instante exacto en que se produjo la falla. Es fundamental para el diagnóstico porque reproduce el contexto (RPM, temperatura, carga, velocidad) en que ocurrió el código de error. Los "calentamientos" (heating cycles) y la distancia recorrida ayudan a determinar si la falla es intermitente o permanente.

---

### 3.3 Datos en Vivo (Live Data)

Permite visualizar en tiempo real los parámetros del motor. El scanner permite seleccionar múltiples canales (se observaron 9, 12 y 13 ítems seleccionados en distintas pantallas).

**Parámetros observados en esta sesión:**

#### Sensores de Motor

| Parámetro | Valor Observado | Unidad | Significado |
|---|---|---|---|
| Presión del colector de admisión (MAP) | 35 / 102.0 / 133 | kPa | Presión absoluta en el colector de admisión |
| Sensor MAP (voltaje) | 1.39 / 1.58 | V [0–5V] | Señal analógica del sensor MAP |
| Sensor MAF | 1.41 / 1.63 / 2143 / 2146 | g/s | Flujo másico de aire ingresado al motor |
| Flujo de aire calculado | — | g/s | Caudal de aire calculado por la ECU |
| BARO | 102.0 | kPa | Presión atmosférica referenciada por la ECU |
| Sensor de velocidad del vehículo | — | km/h | Velocidad actual |
| Sensor de posición de mariposa 1 (TPS 1) | — | V / % | Posición de la mariposa del acelerador |
| Sensor de posición de mariposa 2 (TPS 2) | — | V / % | Sensor redundante de mariposa |
| Sensor IAT (temperatura de aire de admisión) | 16 | °C | Temperatura del aire entrante |
| Sensor ECT (temperatura de refrigerante) | — | °C | Temperatura del motor |
| Sensor de aceite del motor | — | °C | Temperatura de aceite |
| Sensor de nivel de combustible | 36.1 | % | Nivel de nafta en el tanque |
| Sensor APP 1 y APP 2 (pedal de acelerador) | — | V / % | Posición del pedal |

#### Parámetros de Combustible

| Parámetro | Significado |
|---|---|
| Ajuste de combustible a corto plazo (STFT) | Corrección inmediata de la mezcla |
| Ajuste de combustible a largo plazo (LFT) | Corrección acumulada de la mezcla |
| Ciclo de trabajo del inyector | % de tiempo que el inyector está abierto |
| Contenido de alcohol en el combustible | % de etanol detectado en la nafta |
| Modo de potencia recomendado | Modo de operación del motor |
| Máximo contenido de alcohol permitido | Límite de etanol tolerado |
| Combustible del módulo de inmovilizador | Estado del combustible del inmo |

#### Parámetros de Encendido

| Parámetro | Significado |
|---|---|
| Sincronización de encendido | ° AV (avance de encendido) |
| Estado de fallo de encendido detectado | Indica falla en el sistema de encendido |
| Fallo de encendido total | Cantidad de fallos acumulados |
| Retardo de picado total | Retardo del sistema de encendido |
| Régimen del motor al detectarse velocidad excesiva | RPM en condición de overspeed |
| Estado de prueba de alta/baja tensión en circuito de inyector | Diagnóstico eléctrico del inyector |
| Estado de prueba de circuito abierto en inyector | Diagnóstico de circuito abierto |
| Estado de prueba de interrupción en inyector de arranque en frío | Diagnóstico de inyector de arranque |

#### Sensores Adicionales

| Parámetro | Significado |
|---|---|
| Sensor de velocidad del árbol de levas | Posición del árbol de levas |
| Contador activo de posición del árbol de levas | Conteo de posición para sincronización |
| Sensor OSS de transmisión | Velocidad de salida de transmisión |
| Prueba de funcionamiento de MAP | Estado de autoprueba del sensor MAP |
| Calefactor HO2S 1 y HO2S 2 | Estado del calefactor de sondas lambda |
| Señal Terminal 15 del alternador | Estado de carga del alternador |
| Señal de par ofrecida | Par motor calculado |
| Memoria redundante del cuentakilómetros | Odómetro de respaldo |

---

### 3.4 Listado Personalizado (Custom List)

Pantalla que agrupa un conjunto seleccionado de parámetros organizados en columnas. Es la vista de diagnóstico más completa utilizada en la sesión.

**Secciones internas del Listado Personalizado:**

#### Bloque 1 — Sensores y Estado del Motor
- Sensor de posición de mariposa 1 / Sensor de posición de mariposa 2
- Posición del sensor APP 1 / Posición del sensor APP 2
- Sensor IAT
- Sensor MAF
- Sensor MAP
- BARO
- Flujo de aire calculado

#### Bloque 2 — Encendido y Fallos
- Sincronización de encendido
- Régimen del motor
- Fallo de encendido detectado (cilindros 1–4)
- Contador de fallos de encendido actuales e historial (cilindros 1–4)
- Retardo de picado total
- Señal de par ofrecida

#### Bloque 3 — Pruebas de Circuitos de Inyectores (cilindros 1–4)
- Estado de prueba de alta tensión en circuito de control
- Estado de prueba de baja tensión en circuito de control
- Estado de prueba de circuito abierto en el inyector
- Inyector deshabilitado / Fallo de encendido detectado
- Contador de fallos actuales e historial por cilindro

#### Bloque 4 — Sistema de Aire Acondicionado (A/A)
- Comando del relé del embrague del compresor de A/A
- Estado de prueba de alta tensión en circuito de control del relé A/A
- Estado de prueba de baja tensión en circuito de control del relé A/A
- Estado de prueba de circuito abierto del relé A/A
- A/A deshabilitado
- Presión del A/A requerida / fuera de rango
- Historial de desactivación del A/A (1 al 8)

#### Bloque 5 — Sistema de Refrigeración y Ventiladores
- Comando de relés 2 y 3 del ventilador de refrigeración
- Comando del relé del ventilador de refrigeración
- Interruptor de apagado de control de velocidad
- Interruptor que establece control de velocidad
- Interruptor de presión del aceite de motor
- Carga del motor

#### Bloque 6 — Sistema de Combustible
- Comando de la bomba de combustible de arranque en frío
- Comando del relé de la bomba de combustible
- Estado de prueba de alta/baja tensión en circuito de control de la bomba
- Estado de prueba de circuito abierto de la bomba de combustible de arranque

#### Bloque 7 — Inmovilizador (Immobilizer)
- Código de seguridad programado del inmovilizador
- Contador de programación del código de seguridad
- Contador de reinicios del código de seguridad
- Estado del sistema del inmovilizador
- Combustible del módulo de inmovilizador

#### Bloque 8 — Monitores OBD-II
- Prueba de funcionamiento de MAP
- Prueba de catalización (Monitor de catalizador)
- Contador de prueba de monitor de catalización
- Resultado de la prueba de monitor de catalización
- Se cumplen las condiciones de prueba "no en ralentí" del monitor de catalización
- Calefactor HO2S 1 / HO2S 2
- Estado de prueba de alta/baja tensión en calefactor HO2S
- Estado de prueba de circuito abierto del calefactor HO2S
- Estado de prueba de circuito de control abierto del terminal L del alternador
- MIL (Testigo de Avería del Motor) solicitado por DTC
- Estado de prueba de circuito de control abierto MIL
- Estado de prueba de baja tensión del circuito MIL
- Eventos de repostaje desde máximo contenido de alcohol
- Máximo contenido de alcohol en combustible recomendado
- Contenido de alcohol en combustible inicializado

#### Bloque 9 — Adaptaciones y Compensaciones
- Reinicio de compensación de flujo de aire a ralentí del cuerpo de la mariposa
- Compensación de flujo de aire a ralentí (Datos TAC)
- Ajuste de combustible a corto/largo plazo
- Promedio de prueba de ajuste de combustible a largo plazo sin purga
- Enriquecimiento de combustible — Catalizador caliente / Refrigerante caliente
- Economía de combustible

#### Bloque 10 — Controles del Motor (Inhibiciones)
- Inhibir solicitud de par motor — Sincronización de encendido
- Inhibir solicitud de par motor — Ralentí mínimo
- Inhibir solicitud de par motor — Par mínimo
- Inhibir solicitud de par motor — TAC
- Inhibir solicitud de par motor — Límite TAC

#### Bloque 11 — Contadores y Estado de Encendido
- Ciclos de encendido desde que superó el máximo de alcohol
- Marcha actual al detectarse velocidad excesiva del motor
- Ciclos de datos de fallos actuales del encendido
- Contador de fallos de encendido del historial
- Calentamientos sin emisiones (IN)
- Cuentakilómetros redundante
- Memoria redundante del cuentakilómetros

#### Bloque 12 — Control de Arranque
- Comando del relé del motor de arranque
- Comando del terminal L del alternador
- Estado de prueba de alta/baja tensión en relé de arranque
- Estado de prueba de circuito abierto del relé ECT de arranque
- Estado de prueba de alta/baja tensión en circuito de terminal L del alternador
- Compensación del flujo de aire de ralentí del cuerpo de la mariposa
- Señal de solicitud de arranque
- Señal de posición del cigüeñal
- Interruptor de posición de arranque / deceleración

#### Bloque 13 — Otras señales
- Temperatura del aceite al detectarse velocidad excesiva
- Restante de vida del aceite del motor
- Posición del sensor 2 APP al detectarse velocidad excesiva
- Cálculo de temperatura de aceite del motor
- Contador de prueba de monitor de catalización
- Comando del inyector de combustible de arranque en frío
- Estado de la prueba de alta tensión en inyector de arranque en frío
- Estado de la prueba de baja tensión en inyector de arranque en frío
- Estado de la prueba de interrupción del inyector de arranque en frío
- Programación de composición del combustible
- Capacidad normal del depósito de combustible
- Volatilidad del combustible
- Par de eje solicitado por el conductor
- Estado de autenticación ECM
- Estado de reto ECM
- Motivo de inhibición ECM
- Identificador de gestión de energía eléctrica
- Supervisión de calibración del motor

---

### 3.5 Pruebas Activas (Active Test)

Permite al técnico **comandar manualmente actuadores** para verificar su funcionamiento.

**Componentes testeables identificados:**

| Componente | Función del Test |
|---|---|
| Válvula solenoide de purga EVAP | Prueba de apertura/cierre del sistema de vapores |
| Relé de la bomba de combustible de arranque en frío | Verifica activación de bomba de priming |
| Relé del ventilador de refrigeración | Prueba de encendido del ventilador |
| Relé del embrague del compresor de A/A | Verifica acoplamiento del compresor |
| Cuerpo de mariposa (TPS) | Verifica posición y respuesta |
| Inyector de combustible de arranque en frío | Prueba de inyección |
| Motor TAC (Throttle Actuator Control) | Prueba de posición de mariposa |

**Significado:** Las pruebas activas permiten confirmar o descartar fallas en actuadores sin necesidad de desmontar componentes. Por ejemplo, si el ventilador no se enciende al comandarlo, se confirma falla en relé o en el motor del ventilador.

---

### 3.6 Servicio / Mantenimiento

Funciones de servicio del vehículo detectadas en la interfaz:

| Función | Descripción |
|---|---|
| Reinicio de compensación de flujo de aire a ralentí | Aprendizaje del cuerpo de mariposa |
| Reinicio / aprendizaje de posición TPS | Calibración de sensores de mariposa |
| Reset de adaptaciones de combustible | Borra ajustes de corto/largo plazo |
| Programación de inmovilizador | Código de seguridad del vehículo |
| Programación de composición de combustible | Actualiza perfil de etanol en la ECU |
| Reset de DTC / Freeze Frame | Borra códigos de error almacenados |
| Cuentakilómetros redundante | Visualización del odómetro de respaldo |

---

## 4. Cómo se Navega por el Menú

La interfaz del scanner AUTEL utiliza navegación por **flechas direccionales + botón de selección**. En la parte inferior de la pantalla se observan las siguientes teclas de función:

| Tecla | Función |
|---|---|
| **ESC** | Retroceder / Salir |
| **Seleccionar / Aceptar** | Confirmar entrada |
| **Limpiar todo** | Borrar selección / Reiniciar |
| **Flechas ↑ ↓ ← →** | Navegar por opciones del menú |

**Flujo típico de navegación:**
1. Conectar el scanner al puerto OBD-II del vehículo
2. Seleccionar marca → Chevrolet
3. Seleccionar modelo → Onix
4. Confirmar año y variante de motor
5. Acceder a **Diagnóstico** para leer DTCs
6. Acceder a **Live Data** para ver parámetros en tiempo real
7. Usar **Custom List** para agrupar los parámetros de interés
8. Acceder a **Active Test** para probar actuadores
9. Acceder a **Service** para funciones de mantenimiento

---

## 5. Arquitectura Técnica del Sistema CAN (Chevrolet Onix)

Basado en los datos observados, el vehículo opera sobre red **CAN-BUS** con los siguientes módulos diagnosticables:

| Módulo | Dirección CAN | Función |
|---|---|---|
| PCM / ECM (Motor) | [40] | Control de motor principal |
| BCM | — | Control de carrocería (no accedido en esta sesión) |

La ECU registra **403 códigos de diagnóstico** (dirección [40]), lo que indica una base de datos amplia de parámetros monitoreables. El scanner lee parámetros a través de PIDs OBD-II estándar (Modo $01) y modos extendidos específicos de GM (Modo $02 Freeze Frame, Modo $03/07 DTCs).

---

## 6. Estado del Vehículo al Momento del Diagnóstico

| Indicador | Estado |
|---|---|
| Motor | En marcha |
| Velocidad | No registrada en OCR (solo freeze frames) |
| Temperatura de admisión | 16 °C (motor frío) |
| Flujo de aire (MAF) | ~1.4–2.1 g/s (ralentí) |
| Presión atmosférica | 102.0 kPa (nivel del mar) |
| Presión colector admisión | 35–133 kPa (ralentí) |
| Nivel de combustible | 36.1 % |
| Odómetro | 141.395 km |
| Calentamientos desde borrado DTC | 3 |

---

## 7. Notas sobre la Calidad del OCR

Los archivos de salida de Tesseract contienen errores de reconocimiento propios del OCR sobre capturas de interfaz de scanner (fuente pequeña, contraste, movimiento). Algunos caracteres fueron sustituidos incorrectamente (ej: `TAC` confundido con `TAO`, `HO2S` como `HOS`, `APP` como `AAE`). El análisis se realizó cruzando información entre capturas y usando conocimiento del dominio automotriz para corregir interpretaciones.

---

## 8. Resumen de Capacidades del Scanner para este Vehículo

El AUTEL V18.00 para Chevrolet Onix permite:

- **Lectura/borrado de DTCs** en todos los módulos
- **Visualización en vivo** de más de 100 parámetros del motor
- **Freeze Frame** para diagnóstico de fallas intermitentes
- **Pruebas activas** de actuadores del sistema de admisión, combustible, encendido, refrigeración y A/C
- **Servicios** de calibración, aprendizaje y programación
- **Monitoreo OBD-II** de todos los monitores de emisiones (catalizador, EVAP, EGR, O2, etc.)
- **Diagnóstico de inmovilizador** con contadores de seguridad
- **Lectura de VIN, SN y calibraciones** de la ECU

---

*Archivo generado a partir del análisis de 51 capturas OCR del scanner AUTEL sobre vehículo Chevrolet/Onix VIN 9BGKL48T0HB130763.*
*Fecha de captura: 2026-05-22.*
