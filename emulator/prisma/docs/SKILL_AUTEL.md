# SKILL_AUTEL.md — Ingeniería Inversa del Menú de Scanner AUTEL

## 1. Contexto del Equipo

**Modelo:** Autel MaxiSYS MS906 Pro / MS906BT / MS906TS  
**Firmware:** V18.00  
**Arquitectura:** Tablet Android + VCI inalámbrico (MaxiVCI V100)  
**Cobertura:** Diagnóstico OBD-II/EOBD y diagnósticos OE-level para múltiples marcas

---

## 2. Arquitectura del Sistema

```
┌─────────────────────────────────────┐
│         MaxiSys Display Tablet       │
│  ┌───────────────────────────────┐  │
│  │   Job Menu (Aplicaciones)     │  │
│  │   ┌─────────────────────────┐ │  │
│  │   │ Diagnostics             │ │  │
│  │   │ Data Manager            │ │  │
│  │   │ TPMS Service            │ │  │
│  │   │ MaxiFix                 │ │  │
│  │   │ Settings                │ │  │
│  │   │ Shop Manager            │ │  │
│  │   │ Update                  │ │  │
│  │   │ VCI Manager             │ │  │
│  │   │ Remote Desk             │ │  │
│  │   │ Support                 │ │  │
│  │   │ Training                │ │  │
│  │   │ Quick Link              │ │  │
│  │   │ MaxiScope               │ │  │
│  │   │ Digital Inspection      │ │  │
│  │   └─────────────────────────┘ │  │
│  └───────────────────────────────┘  │
└───────────────┬─────────────────────┘
                │ Bluetooth / USB
                ▼
┌─────────────────────────────────────┐
│      MaxiVCI V100 (VCI Device)      │
│    Interfaz de comunicación CAN      │
└───────────────┬─────────────────────┘
                │ OBD-II / CAN / LIN / FlexRay
                ▼
┌─────────────────────────────────────┐
│          Vehículo (ECU/ECM)          │
│    PCM [40] - Módulo de control      │
└─────────────────────────────────────┘
```

---

## 3. Estructura del Menú Principal (Job Menu)

El scanner opera mediante **navegación táctil jerárquica** (touch-driven). La pantalla inicial presenta el **Job Menu** con las siguientes aplicaciones principales:

```
MaxiSys Job Menu
├── 1. Diagnostics          (Diagnóstico)
├── 2. Data Manager         (Gestor de Datos)
├── 3. TPMS Service         (Servicio TPMS - solo MS906TS)
├── 4. MaxiFix              (Plataforma de reparación)
├── 5. Settings             (Configuración)
├── 6. Shop Manager         (Gestor de Taller)
├── 7. Update               (Actualizaciones)
├── 8. VCI Manager          (Gestor de Conexión VCI)
├── 9. Remote Desk          (Escritorio Remoto)
├── 10. Support             (Soporte Técnico)
├── 11. Training            (Capacitación)
├── 12. Quick Link          (Enlaces Rápidos)
├── 13. MaxiScope           (Osciloscopio)
└── 14. Digital Inspection  (Inspección Digital)
```

---

## 4. Sección Diagnostics — Estructura Completa

### 4.1 Acceso a Diagnosis

Al seleccionar **Diagnostics** desde el Job Menu:
1. Se muestra el **Vehicle Menu** (lista de marcas)
2. Se selecciona marca → modelo → año → motor
3. Se establece comunicación con el vehículo

### 4.2 Vehicle Menu Layout

```
Vehicle Menu
├── [Marca]
│   ├── [Modelo]
│   │   ├── [Año]
│   │   │   └── [Variante de motor]
│   │   └── ...
│   └── ...
├── EOBD / OBD-II (acceso rápido)
└── Búsqueda por VIN (Auto VIN Scan)
```

### 4.3 Estructura de Diagnosis (Menú Principal de Diagnosis)

Una vez establecida la comunicación:

```
Diagnosis
├── Auto Scan                    (Escaneo automático de todos los sistemas)
├── Control Units                (Selección manual de unidad de control)
│
├── ECU Information              (Información de la ECU)
├── Read Codes                   (Lectura de códigos DTC)
├── Erase Codes                  (Borrado de códigos DTC)
├── Live Data                    (Datos en vivo / Parámetros en tiempo real)
├── Active Test                  (Pruebas activas / Componentes)
└── Special Functions            (Funciones especiales / Adaptaciones)
```

---

## 5. Detalle de Cada Función de Diagnosis

### 5.1 ECU Information

**Propósito:** Muestra información específica de la unidad de control testeada.

**Campos típicos:**
- Tipo de unidad
- Números de versión
- Especificaciones del fabricante
- VIN del vehículo
- Número de serie del módulo
- Cuentakilómetros

**Navegación:**
- Back / ESC → regresa al menú anterior

---

### 5.2 Read Codes (Lectura de Códigos DTC)

**Propósito:** Recupera y muestra los DTCs (Diagnostic Trouble Codes) del sistema de control del vehículo.

**Estructura de la pantalla:**
```
Read Codes
├── Code Column          (Código DTC)
├── Status Column        (Estado: Confirmed, Pending, etc.)
├── Description Column   (Descripción del código)
└── Snowflake Icon       (Freeze Frame disponible)
```

**Opciones:**
- Ver Freeze Frame por cada DTC
- Back / ESC → regresar

---

### 5.3 Erase Codes (Borrado de DTCs)

**Propósito:** Borra los códigos de error almacenados después de reparaciones.

**Precondiciones:**
- Llave de encendido en posición ON (RUN)
- Motor apagado

**Flujo:**
1. Tap "Erase Codes"
2. Confirmación de advertencia
3. Ejecución del borrado
4. Mensaje de confirmación

---

### 5.4 Live Data (Datos en Vivo)

**Propósito:** Visualización en tiempo real de parámetros del vehículo.

**Estructura de pantalla:**
```
Live Data
├── Diagnostics Toolbar (parte superior)
│   ├── Setting (unidades, modos)
│   ├── Search
│   ├── Print
│   └── More options
│
├── Main Section
│   ├── Name Column      (Nombre del parámetro)
│   │   ├── Check Box    (Seleccionar item)
│   │   └── Drop-down    (Modo de visualización)
│   ├── Value Column     (Valor actual)
│   ├── Range Column     (Mínimo - Máximo)
│   └── Unit Column      (Unidad de medida)
```

**Modos de visualización disponibles:**
1. **Text Mode** (predeterminado) - Lista de parámetros con valores
2. **Analog Gauge Mode** - Medidor analógico gráfico
3. **Waveform Graph Mode** - Gráfico de forma de onda
   - Botones: Text / Scale / Zoom-in
4. **Digital Gauge Mode** - Medidor digital gráfico

**Botones funcionales:**
| Botón | Función |
|-------|---------|
| Back | Regresar / Salir |
| Record | Iniciar grabación de datos en vivo |
| Freeze | Congelar datos actuales |
| Previous Frame | Retroceder frame en freeze |
| Next Frame | Avanzar frame en freeze |
| Resume | Reanudar visualización normal |
| Flag | Marcar punto en grabación |
| Clear Data | Limpiar valores almacenados |
| To Top | Mover item seleccionado al inicio |
| Graph Merge | Combinar gráficos (solo Waveform) |

**Selección personalizada:**
- Checkbox izquierdo: seleccionar/deseleccionar parámetros
- Drop-down derecho: cambiar modo de visualización por parámetro

---

### 5.5 Active Test (Pruebas Activas)

**Propósito:** Comandar manualmente actuadores para verificar funcionamiento.

**Funcionamiento:**
- El tester envía comandos a la ECU
- La ECU activa/desactiva actuadores
- Se verifica la integridad del sistema

**Componentes testeables típicos:**
- Válvulas solenoides (EVAP, EGR, etc.)
- Relés (ventilador, bomba de combustible, A/C)
- Inyectores de combustible
- Cuerpo de mariposa (TPS)
- Motor de arranque
- Calefactores (sondas lambda)
- Compresor de A/A
- Iluminación (luces, testigos)

**Estructura de pantalla:**
```
Active Test
├── Lista de componentes disponibles
├── Controles Active Test (parte superior)
│   ├── ON/OFF
│   ├── Slider (valor variable)
│   └── Botones específicos
└── Data Stream (información de parámetros)
```

**Mensajes de éxito:**
- "Command Finished"
- "Activation Successful"

**Salida:** Botón ESC

---

### 5.6 Special Functions (Funciones Especiales / Adaptaciones)

**Propósito:** Realizar adaptaciones y calibraciones de componentes post-reparación.

**Estructura de pantalla de Adaptation Operation:**
```
Adaptation Operation
├── Parte 1: Descripción de la operación + estado (Completed, Activated...)
├── Parte 2: Precondiciones requeridas
├── Parte 3: Condiciones actuales del vehículo (comparación)
└── Parte 4: Instrucciones de botones funcionales
```

**Funciones especiales típicas:**
- Reinicio de compensación de flujo de aire (IAC)
- Calibración de posición de mariposa (TPS)
- Aprendizaje de sensor de posición de árbol de levas
- Reset de adaptaciones de combustible
- Programación de inmovilizador

---

## 6. Sección Service (Mantenimiento)

**Acceso:** Menú principal de Diagnosis → Service

**Funciones de servicio principales:**
```
Service
├── Oil Reset Service
│   └── Reinicio del recordatorio de vida del aceite
├── TPMS Programming Service
│   └── Programación de sensores de presión de neumáticos
├── EPB Service
│   └── Mantenimiento del freno de estacionamiento electrónico
├── ABS/SRS Services
│   ├── Bleeding automatizado
│   ├── Test de motor de bomba
│   └── Información de módulo
├── SAS Calibration Service
│   └── Calibración del sensor de ángulo de dirección
└── DPF Regeneration Service
    └── Regeneración del filtro de partículas diesel
```

**Características:**
- Menú dirigido por comandos ejecutivos
- Guía paso a paso mediante instrucciones en pantalla
- Verificación automática de precondiciones
- Confirmación de finalización

---

## 7. Generic OBD-II Operations

**Acceso rápido:** Vehicle Menu → EOBD button

**Funciones OBD-II estándar:**
```
OBD-II Functions
├── Auto Scan
│   └── Escaneo automático por protocolo
├── Protocol
│   └── Selección manual de protocolo
│       ├── ISO 9141-2
│       ├── ISO 14230-4 (KWP2000)
│       ├── ISO 15765-4 (CAN)
│       ├── SAE J1850 PWM
│       └── SAE J1850 VPW
├── Read Codes
├── Erase Codes
├── Live Data
├── Freeze Frame
└── I/M Readiness
```

---

## 8. Sistema de Navegación y Controles

### 8.1 Controles Físicos

| Botón | Función |
|-------|---------|
| **Power** | Encendido/Apagado |
| **Home** | Volver al Job Menu principal |
| **ESC** | Retroceder / Salir de función |
| **Seleccionar / Aceptar** | Confirmar selección |
| **Flechas ↑ ↓ ← →** | Navegar por menús |

### 8.2 Navegación Táctil

- **Tap:** Seleccionar item
- **Swipe Up/Down:** Scroll en listas largas
- **Checkbox:** Seleccionar/deseleccionar parámetros
- **Drop-down arrow:** Abrir submenu de opciones

### 8.3 Toolbar Buttons (Diagnostics)

| Botón | Icono | Función |
|-------|-------|---------|
| Back | ← | Regresar / Salir |
| Print | 🖨️ | Imprimir pantalla |
| Record | ● | Grabar datos en vivo |
| Freeze | ❄️ | Congelar datos |
| Flag | 🚩 | Marcar punto en grabación |
| Clear Data | 🗑️ | Limpiar valores |
| To Top | ↑↑ | Mover item al inicio |
| Graph Merge | ⧉ | Combinar gráficos |
| Setting | ⚙️ | Configuración de unidades |

---

## 9. Ingeniería Inversa — Metodología desde OCR

### 9.1 Fuente de Datos

Las capturas de pantalla del scanner son la fuente primaria para reconstruir el menú. Cada imagen contiene texto renderizado de la interfaz táctil.

### 9.2 Proceso de Extracción OCR

```
Captura de pantalla → Tesseract OCR → Texto crudo → Limpieza → Estructura
```

**Pasos:**
1. **Captura:** Imagen de la pantalla del scanner
2. **OCR con Tesseract:** Extracción de texto
   - Configuración: `--psm 3` (detección automática de página)
   - Idioma: `spa` (español) o `eng` (inglés)
3. **Limpieza de texto:**
   - Eliminación de caracteres corruptos
   - Corrección de errores de reconocimiento
   - Normalización de términos técnicos
4. **Identificación de patrones:**
   - Encabezados de sección
   - Items de menú
   - Valores numéricos y unidades
   - Estados de prueba

### 9.3 Patrones de Reconocimiento de Menú

#### Patrón 1: Menú Principal
```
Número. Título de opción
```
Ejemplo:
```
1. Procesar imagen desde input/
2. Ver imagenes en input/
3. Procesar todas las imagenes
4. Salir
```

#### Patrón 2: Submenú con selección múltiple
```
N ítems seleccionados
[Checkbox] Nombre del parámetro
```
Ejemplo:
```
9 ítems seleccionados
☑ Sensor MAF
☑ Sensor MAP
☐ Sensor IAT
```

#### Patrón 3: Parámetros con valores
```
Nombre del parámetro          Valor   Unidad
```
Ejemplo:
```
Sensor MAF                   1.63    g/s
Presión del colector         35      kPa
```

#### Patrón 4: Pruebas activas
```
Nombre del componente         Estado
```
Ejemplo:
```
Válvula solenoide de purga EVAP    ON
Relé del ventilador                 OFF
```

#### Patrón 5: Navegación inferior
```
Limpiar todo    Seleccionar    Aceptar    ESC
```

### 9.4 Técnicas de Validación Cruzada

1. **Consistencia entre capturas:** Si un parámetro aparece en múltiples capturas, validar su nombre y posición
2. **Conocimiento del dominio:** Corregir términos técnicos mal reconocidos
   - TAC → Throttle Actuator Control
   - HO2S → Heated Oxygen Sensor
   - APP → Accelerator Pedal Position
   - MIL → Malfunction Indicator Lamp
3. **Agrupación lógica:** Los parámetros relacionados deben aparecer juntos
4. **Validación numérica:** Los valores deben estar en rangos físicos plausibles

### 9.5 Reconstrucción de la Estructura

**Nivel 1:** Identificar encabezados de sección (ECU Information, Live Data, etc.)  
**Nivel 2:** Extraer items de menú y submenús  
**Nivel 3:** Mapear parámetros y valores  
**Nivel 4:** Documentar flujos de navegación  
**Nivel 5:** Validar contra el manual oficial  

---

## 10. Ejemplo de Reconstrucción desde OCR (Caso Real)

### Entrada OCR (cruda):
```
9 ítems seleccionados

Comando de la válvula solenoide de purga EVAP
0
Sensor del nivel de combustible Q E
a Sensor lAT «é 28
Ed ur de arranque ug 3 | Es

Sensor MAF

| Ode ame catoutado Q |

Cantidad bandera: O
```

### Salida procesada:
```yaml
menu: Live Data
selected_items: 9
parameters:
  - name: Comando de la válvula solenoide de purga EVAP
    value: 0
    unit: %
  - name: Sensor del nivel de combustible
    value: 36.1
    unit: %
  - name: Sensor IAT
    value: 28
    unit: °C
  - name: Sensor MAF
    value: 1.41
    unit: g/s
  - name: Flujo de aire calculado
    value: null
    unit: g/s
```

---

## 11. Mapeo de la Interfaz a Código

### 11.1 Estructura de Datos Recomendada

```cpp
struct MenuItem {
    std::string id;
    std::string label;
    std::string parent;
    std::vector<std::string> children;
    bool isLeaf;
};

struct Parameter {
    std::string name;
    std::string value;
    std::string unit;
    std::string min;
    std::string max;
};

struct ScreenCapture {
    std::string filename;
    std::string section;
    std::vector<MenuItem> menuItems;
    std::vector<Parameter> parameters;
    std::string timestamp;
};
```

### 11.2 Generación del Menú desde OCR

```cpp
std::vector<MenuItem> reconstructMenu(const std::string& ocrText) {
    std::vector<MenuItem> menu;
    std::istringstream iss(ocrText);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Patrón: "N. Título"
        std::regex menuPattern(R"((\d+)\.\s+(.+))");
        std::smatch match;
        if (std::regex_search(line, match, menuPattern)) {
            MenuItem item;
            item.id = match[1];
            item.label = match[2];
            item.isLeaf = false;
            menu.push_back(item);
        }
        
        // Patrón: "N ítems seleccionados"
        std::regex selectedPattern(R"((\d+)\s+ítems\s+seleccionados)");
        if (std::regex_search(line, match, selectedPattern)) {
            // Marcar contexto como Live Data
        }
    }
    
    return menu;
}
```

---

## 12. Notas sobre la Calidad del OCR

### Errores Comunes en Capturas de Scanner

| Error OCR | Texto Correcto | Causa |
|-----------|----------------|-------|
| TAC → TAO | TAC → Throttle Actuator Control | Confusión O/A |
| HOS → HO2S | HO2S → Heated Oxygen Sensor | Falta de contexto |
| AAE → APP | APP → Accelerator Pedal Position | Confusión A/E |
| AA → A/A | A/A → Air Conditioning | Abreviatura |
| 28 → 28°C | Temperatura | Falta de símbolo de grado |
| 35 [0.255] KkPa | 35 kPa | Formato numérico |

### Estrategias de Corrección

1. **Diccionario técnico automotriz:** Lista de términos válidos
2. **Corrección contextual:** Usar parámetros adyacentes para inferir el correcto
3. **Validación de unidades:** Verificar que las unidades sean coherentes
4. **Filtrado de ruido:** Eliminar caracteres especiales no relevantes

---

## 13. Referencia Rápida — Estructura del Menú AUTEL

```
┌─────────────────────────────────────────────────────────────┐
│                    AUTEL MAXISYS MS906 PRO                   │
│                      Job Menu (Principal)                    │
├──────────────┬──────────────┬──────────────────────────────┤
│ 1. Diagnostics│ 5. Settings │ 9.  Remote Desk               │
│ 2. Data Manager│6.Shop Mgr │ 10. Support                  │
│ 3. TPMS Svc  │ 7. Update   │ 11. Training                 │
│ 4. MaxiFix   │ 8. VCI Mgr  │ 12. Quick Link               │
│              │             │ 13. MaxiScope                 │
│              │             │ 14. Digital Inspection        │
└──────────────┴──────────────┴──────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                      DIAGNOSIS                               │
├─────────────────────────────────────────────────────────────┤
│ Auto Scan                                                    │
│ Control Units                                                │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ ECU Information                                         │ │
│ │ Read Codes                                              │ │
│ │ Erase Codes                                             │ │
│ │ Live Data                                               │ │
│ │   ├── Text Mode                                         │ │
│ │   ├── Analog Gauge Mode                                 │ │
│ │   ├── Waveform Graph Mode                               │ │
│ │   └── Digital Gauge Mode                                │ │
│ │ Active Test                                             │ │
│ │   ├── Válvulas solenoides                               │ │
│ │   ├── Relés                                             │ │
│ │   ├── Inyectores                                        │ │
│ │   ├── Cuerpo de mariposa                                │ │
│ │   └── ...                                               │ │
│ │ Special Functions                                       │ │
│ │   ├── Adaptaciones                                      │ │
│ │   ├── Calibraciones                                     │ │
│ │   └── Programaciones                                    │ │
│ └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                        SERVICE                               │
├─────────────────────────────────────────────────────────────┤
│ Oil Reset Service                                           │
│ TPMS Programming Service                                    │
│ EPB Service                                                 │
│ ABS/SRS Services                                            │
│ SAS Calibration Service                                     │
│ DPF Regeneration Service                                    │
└─────────────────────────────────────────────────────────────┘
```

---

## 14. Flujo de Navegación Típico

```
1. Encender tablet → Job Menu
2. Tap "Diagnostics"
3. Seleccionar vehículo (Marca → Modelo → Año → Motor)
4. Establecer comunicación con VCI
5. Menú Diagnosis:
   - Auto Scan (escaneo completo)
   - o Control Units (selección manual)
6. Seleccionar módulo (ej: ECM/PCM)
7. Function Menu:
   - ECU Information
   - Read Codes
   - Live Data
   - Active Test
   - Special Functions
8. Seleccionar función deseada
9. Realizar prueba / visualizar datos
10. ESC para retroceder
11. ESC nuevamente para salir de Diagnosis
```

---

## 15. Comandos y Códigos Útiles

### 15.1 Comandos Tesseract para OCR

```bash
# OCR básico (español)
tesseract imagen.jpg output -l spa --psm 3

# OCR con datos de coordenadas
tesseract imagen.jpg output -l spa --psm 3 tsv

# OCR de múltiples imágenes en lote
for img in *.jpg; do
    tesseract "$img" "${img%.*}" -l spa --psm 3
done
```

### 15.2 Configuración Recomendada para Capturas de Scanner

```json
{
  "tesseract": {
    "lang": "spa+eng",
    "psm": 3,
    "oem": 1,
    "config": {
      "tessedit_char_whitelist": "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzáéíóúüñÁÉÍÓÚÜÑ()[]{}%/<>=-+.,;: "
    }
  }
}
```

---

## 16. Glosario de Términos

| Término | Significado |
|---------|-------------|
| **DTC** | Diagnostic Trouble Code |
| **ECU/ECM/PCM** | Unidad de control del motor |
| **VCI** | Vehicle Communication Interface |
| **CAN-BUS** | Controller Area Network |
| **OBD-II** | On-Board Diagnostics II |
| **MIL** | Malfunction Indicator Lamp |
| **TPS** | Throttle Position Sensor |
| **MAF** | Mass Air Flow Sensor |
| **MAP** | Manifold Absolute Pressure Sensor |
| **IAT** | Intake Air Temperature Sensor |
| **ECT** | Engine Coolant Temperature |
| **HO2S** | Heated Oxygen Sensor |
| **APP** | Accelerator Pedal Position |
| **EVAP** | Sistema de evaporación de combustible |
| **TAC** | Throttle Actuator Control |
| **STFT/LFT** | Short/Long Term Fuel Trim |
| **Freeze Frame** | Datos capturados en el momento del DTC |
| **Active Test** | Pruebas activas de actuadores |
| **Special Functions** | Funciones especiales/adaptaciones |
| **Service** | Funciones de mantenimiento programado |

---

## 17. Referencias

- Manual oficial: Autel MaxiSYS MS906 Pro (145 páginas)
- Firmware: V18.00
- Vehículo de prueba: Chevrolet Onix VIN 9BGKL48T0HB130763
- Dirección CAN: PCM [40]
- Cantidad de parámetros diagnosticables: 403

---

*Documento generado mediante ingeniería inversa a partir del manual oficial AUTEL y análisis de 51 capturas OCR del scanner.*
*Fecha de análisis: 2026-08-14*
