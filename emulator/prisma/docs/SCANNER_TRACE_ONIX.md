# Traza del escáner contra el vehículo real (Chevrolet Onix)

> **Referencia capturada del vehículo real** (mismo VIN que usa el emulador:
> `9BGKL48T0HB130763`). Útil para alinear el emulador (`src/elm327.cpp`) con el
> comportamiento real: máscaras de PIDs, nombre de ECU en modo 09, y qué DIDs
> modo 22 responden y con qué formato.
>
> **Conclusiones clave (analizadas):**
> - El modo 01 del Onix real publica máscara `0100 → BE 3F B8 13` (MSB-first:
>   PIDs 01,03,04,05,06,07, 0B-10, 11, 13, 14, 15, 1C, 1F, 20). El emulador es
>   un superconjunto (BF FF BF D2) — intencional.
> - PIDs no implementados (p. ej. `0116`, `0118`, `011A` y `014F`) → el real
>   responde `41 <pid> 00 00 00 00` (ceros) o NO DATA; el emulador devuelve
>   NO DATA.
> - `090A` (nombre de ECU) real = `"TCM-Engine Control"` (el emulador ahora lo
>   replica). CALID (`0904`) real ≠ `1505708/52124404`.
> - **Modo 22: solo 5 DIDs responden en el real** con estos formatos:
>   `1564 → 62 15 64 29` (1 byte), `1940 → 62 19 40 23` (1 byte = °C+40),
>   `11A1 → 62 11 A1 00 00` (2 bytes = segundos), `1201 → 62 12 01 00 00`
>   (2 bytes = 0), `2345 → 62 23 45 00` (1 byte = 0). El resto → NO DATA
>   (el real NO responde NRC `7F`; el emulador sí lo hace, a propósito).
> - El real no responde los DIDs CANSF de ScanGauge (`119F`, `119E`, `11A6`,
>   `1251/119D`, `162F-1636`, `1193-119A`): el emulador los implementa como
>   superconjunto para el escáner AUTEL.

---

=============================================
LISTADO Y EXPLICACION DE COMANDOS OBD Y PIDS
=============================================

A continuacion se detallan todos los comandos de diagnostico OBD observados
(excluyendo los comandos AT de configuracion del ELM327 y la capa Bluetooth),
con sus respectivas respuestas y la decodificacion de los datos.


SECCION 1: MODO 01 - PIDS ESTANDAR (SAE J1979)
----------------------------------------------

Los comandos del modo 01 son universales para todos los vehiculos OBD-II.
La aplicacion los utiliza para leer los valores de los sensores en tiempo real.

Comando: 0100
Respuesta: 7E8064100BE3FB813
Significado: Solicita la lista de PIDs soportados del 01 al 20.
Decodificacion: Los bytes de datos (0xBE3FB813) actuan como una mascara de bits.
Cada bit indica si un PID concreto esta soportado. Por ejemplo, el bit 0
corresponde al PID 01, el bit 1 al PID 02, etc. La respuesta confirma que el
vehiculo soporta la mayoria de los PIDs basicos.

Comando: 0101
Respuesta: 7E806410182000000
Significado: Estado del sistema de monitoreo a bordo (tipo de OBD).
Decodificacion: El byte 0x41 (65 decimal) indica que el vehiculo cumple con
el estandar OBD-II (CALID = 1) y que no hay fallos pendientes en este momento.

Comando: 0103
Respuesta: 7E80441030200
Significado: Sistema de combustible y estado del bucle de control.
Decodificacion: El primer byte de datos (0x03) indica "Bucle cerrado" (Closed Loop),
lo que significa que la ECU esta utilizando los sensores de oxigeno para ajustar
la mezcla de combustible de forma activa.

Comando: 0104
Respuesta: 7E803410438 (ejemplo) o 7E80341041A (otro ejemplo)
Significado: Carga del motor calculada.
Decodificacion: El valor en hexadecimal 0x38 equivale a 56 decimal.
Formula: Carga (%) = (Valor / 1.25). Por lo tanto, 56 / 1.25 = 44.8 % de carga.
Es un dato que varia segun la aceleracion y las condiciones de conduccion.

Comando: 0105
Respuesta: 7E803410596
Significado: Temperatura del refrigerante del motor.
Decodificacion: 0x59 = 89 decimal. Formula: Temperatura (ºC) = Valor - 40.
Por tanto, 89 - 40 = 49 grados centigrados (motor en calentamiento o temperatura normal de crucero).

Comando: 0106
Respuesta: 7E80341068D
Significado: Ajuste de combustible a corto plazo (Bank 1, Sensor 1).
Decodificacion: 0x8D = 141. Formula: Ajuste (%) = ((Valor - 128) * 100) / 128.
Resultado: (141 - 128) * 100 / 128 = 10.16 %. Es positivo, indicando que la
mezcla se esta enriqueciendo ligeramente para compensar una mezcla pobre detectada.

Comando: 0107
Respuesta: 7E803410780
Significado: Ajuste de combustible a largo plazo (Bank 1, Sensor 1).
Decodificacion: 0x80 = 128. Formula: ((128 - 128) * 100) / 128 = 0 %.
Significa que el ajuste a largo plazo esta en su punto neutro, sin
desviaciones acumuladas.

Comando: 010B
Respuesta: 7E803410B66
Significado: Presion absoluta del colector de admision (MAP).
Decodificacion: 0x66 = 102. El valor se lee directamente en kilopascales (kPa).
Indica 102 kPa, lo que es normal para la altitud y condiciones atmosfericas
estandar cuando el motor esta en ralentí.

Comando: 010C
Respuesta: 7E804410C00CD (motor parado) y 7E804410C03B2 (motor en arranque)
Significado: Revoluciones del motor (RPM).
Decodificacion: Los dos bytes de datos (A y B) se combinan en un solo numero.
Formula: RPM = ((ByteA * 256) + ByteB) / 4.
Para 0x03B2: (3 * 256 + 178) = 946. 946 / 4 = 236.5 RPM (arranque o ralentí muy bajo).
Para 0x00CD: (0 * 256 + 205) = 205. 205 / 4 = 51.25 RPM (motor claramente detenido).

Comando: 010D
Respuesta: 7E803410D00
Significado: Velocidad del vehiculo (VSS).
Decodificacion: 0x00 = 0 km/h. El valor se lee directamente en km/h.

Comando: 010E
Respuesta: 7E803410E4D
Significado: Avance de encendido.
Decodificacion: 0x4D = 77. Formula: Avance (grados) = (Valor - 128) / 2.
(77 - 128) / 2 = -25.5 grados. Un valor negativo indica que el encendido
esta retrasado respecto al punto muerto superior, comun en ralentí para
controlar las emisiones.

Comando: 0133
Respuesta: 7E803413366
Significado: Presion barometrica absoluta.
Decodificacion: 0x33 = 51. El valor se lee en kPa. Indica 51 kPa, que
corresponde a una altitud moderadamente elevada o a una calibracion
particular de este sensor.

Comando: 0140
Respuesta: 7E8064140FED28000
Significado: PIDs soportados del 41 al 60.
Decodificacion: Similar al comando 0100, pero para la siguiente gama de
parametros. Los datos 0xFED28000 indican que PIDs especificos como el
de la posicion del acelerador (0x41) estan disponibles.

Comando: 014F
Respuesta: 7E806414F00000000
Significado: PIDs soportados del 4F al 60.
Decodificacion: Mascara de bits para la gama alta. En este caso, el valor
0x00000000 indica que ningun PID en ese rango esta soportado.

Observacion sobre el Modo 01:
Los comandos 0116 (Posicion del acelerador), 0118 (Presion del riel de combustible)
y 011A (Temperatura de los gases de escape) fueron probados repetidamente,
pero todos devolvieron "NO DATA". Esto confirma que el modulo de control
del motor de este Chevrolet no implementa esos PIDs especificos.


SECCION 2: MODO 09 - INFORMACION DEL VEHICULO
----------------------------------------------

Los comandos del modo 09 recuperan datos fijos almacenados en la ECU,
como el VIN y las calibraciones.

Comando: 0902
Respuesta: 7E81014490201394247
          7E8214B4C3438543048
          7E82242313330373633
Significado: Solicitud del VIN (Vehicle Identification Number).
Decodificacion: La respuesta es una trama larga fragmentada en varias lineas.
La aplicacion concatena los bytes de datos de cada linea y los convierte a
caracteres ASCII. Con estos bytes se forma el VIN de 17 caracteres que identifica
unicamente al vehiculo (fabricante, modelo, ano y numero de serie).

Comando: 0904
Respuesta: 7E81083490408313236
          7E82137373833304141
          7E82200000000000032
          7E82334353836343430
          7E82400000000000000
          7E82500323435383634
          7E82633360000000000
          7E82700000031323636
          7E82837323736000000
          7E82900000000003234
          7E82A35383334393800
          7E82B00000000000000
          7E82C32343538333831
          7E82D34000000000000
          7E82E00003234353833
          7E82F38313600000000
          7E82000000000323435
          7E82138363434340000
          7E822000000000000AA
Significado: Solicitud de informacion de calibracion.
Decodificacion: Esta larga secuencia contiene los numeros de parte del
software, hardware y calibraciones especificas de la ECU del motor.
La aplicacion los interpreta como cadenas ASCII para mostrar al usuario
informacion tecnica detallada del sistema de gestion del motor.

Comando: 090A
Respuesta: 7E81017490A0145434D
          7E821002D456E67696E
          7E82265436F6E74726F
          7E8236C0000AAAAAAAA
Significado: Solicitud del nombre del fabricante o nombre de la unidad de control.
Decodificacion: Al decodificar los bytes ASCII de la respuesta se obtiene
la cadena "TCM-Engine Control". Esto identifica la unidad que respondio
como el modulo de control del motor (ECM) con funciones de gestion de
transmision integradas.


SECCION 3: MODO 22 - PIDS ESPECIFICOS DEL FABRICANTE (GM / CHEVROLET)
----------------------------------------------------------------------

El modo 22 (ReadDataByIdentifier) es un servicio de diagnostico extendido
que no esta estandarizado. Cada fabricante asigna sus propios identificadores
(IDs) para leer datos internos de la ECU, como presion de inyeccion,
sincronizacion de cilindros, o estados de transmision.

Lista de comandos probados (ID de 6 digitos):
221564, 221940, 221997, 221993, 221998, 221994, 221999, 221995, 22119E,
22162F, 221630, 221631, 221632, 221633, 221634, 221635, 221636, 221251,
22119D, 22199A, 223201, 221192, 220052, 221171, 22114B, 2211A1, 221470,
222344, 222345, 221154, 2219DE, 221170, 22F432, 221145, 221172, 221141,
221193, 221194, 221195, 221196, 221197, 221198, 221199, 22129A, 221538,
2211A6, 22125D, 22125E, 221992, 221205, 221201, 221206, 221202, 221207,
221203, 221208, 221204, 2211EA, 2211F8, 2211EB, 2211F9, 2211EC, 2211FA,
2211ED, 2211FB, 1ADF, 221161, 22199E, 22199F, 22119F.

Ejemplos de respuestas positivas obtenidas (casos excepcionales):
- Comando: 221564 -> Respuesta: 7E80462156429
- Comando: 221940 -> Respuesta: 7E80462194023
- Comando: 2211A1 -> Respuesta: 7E8056211A10000
- Comando: 221201 -> Respuesta: 7E8056212010000
- Comando: 222345 -> Respuesta: 7E80462234500

Observacion principal sobre el Modo 22:
A pesar de que la aplicacion intenta decenas de identificadores diferentes,
la gran mayoria de las respuestas son "NO DATA". Esto indica que el modulo
del motor (identificado con la cabecera 7E0) no tiene implementados esos
IDs especificos, o que los parametros solicitados estan bloqueados y
requieren una cabecera de comunicacion diferente (por ejemplo, enviar
directamente a la transmision 7E1 o al BCM 241). La aplicacion prueba
todos estos IDs de forma sistematica para "descubrir" que datos
adicionales puede ofrecer el vehiculo. En este modelo de Chevrolet,
casi todos los IDs resultan inactivos, a excepcion de los 5 listados
arriba, que devolvieron tramas de datos especificos que la app
procesaria internamente para mostrar parametros como sincronizacion
de inyeccion (221564), presion de riel, o estados de la caja de cambios.
