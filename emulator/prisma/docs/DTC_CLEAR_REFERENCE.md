# Referencia: Borrado de DTCs — OBD2 / UDS

> El protocolo OBD2 solo contempla UN comando para borrar códigos de falla.
> **No existe borrado selectivo** (no se puede borrar un DTC individual).

---

## 1. Comandos de borrado

### Mode 04 — Clear Diagnostic Information (OBD2)

```
TX: 04
RX: 44
```

- Borra **TODOS** los DTCs confirmados
- Borra **TODOS** los DTCs pendientes
- Apaga la MIL (Check Engine)
- Resetea warmup counter y distance since clear

### Mode 14 — ClearDiagnosticInformation (UDS)

```
TX: 14 FF FF FF
RX: 54
```

- Función equivalente al Mode 04 pero vía UDS
- `FF FF FF` = borrar todos los grupos de DTCs

---

## 2. Qué NO existe

| Comando inexistente | Razón |
|---------------------|-------|
| `04 B2585` | No hay parámetro de selección |
| `04 01` | No hay sub-modo |
| `14 FF FF FF 0001` | No se puede filtrar por DTC |
| `CLEAR B2585` | No existe en OBD2 |

---

## 3. Comportamiento del emulador

### Al iniciar
```
DTCs activos:
  B2585 — Park Lamps Control Circuit (izq)
  B3867 — Right Park Lamp Control Circuit
  B3881 — Tail Lamp Circuit
  B3882 — Right Tail Lamp Circuit
```

### Después de `04 00` o `14 FF FF FF`
```
03 00 → 43 00          (sin DTCs)
07 00 → 47 00          (sin pending)
0A 00 → 4A 00          (sin permanentes)
19 02 AF → 59 02 01 FF 00  (sin DTCs UDS)
```

### Al reiniciar la app
Los DTCs reaparecen (por diseño, para testing).

---

## 4. Flujo completo de testing

```
1. Iniciar emulador
   → DTCs visibles: B2585, B3867, B3881, B3882

2. Scanner lee DTCs
   03 00 → 43 08 A5 85 A8 67 A8 81 A8 82
   19 02 AF → 59 02 01 FF 04 A5 85 09 A8 67 09 A8 81 09 A8 82 09

3. Borrar DTCs
   04 00 → 44  (o 14 FF FF FF → 54)

4. Verificar borrado
   03 00 → 43 00
   07 00 → 47 00
   0A 00 → 4A 00

5. Reiniciar emulador
   → DTCs reaparecen (nueva sesión)
```

---

## 5. Codificación de DTCs OBD2

### Formato de 2 bytes

```
Byte 1 (alto):  [TT][X][XXXX]  ← tipo + primer dígito + segundo dígito
Byte 2 (bajo):  [XXXX][XXXX]  ← tercer dígito + cuarto dígito
```

### Tipos de DTC

| Bits 7-6 | Tipo | Ejemplo |
|----------|------|---------|
| 00 | P (Powertrain) | P0301 |
| 01 | C (Chassis) | C0035 |
| 10 | B (Body) | B2585 |
| 11 | U (Network) | U0100 |

### Ejemplo: B2585

```
B = 10 (Body)
2 = 0010
5 = 0101
8 = 1000
5 = 0101

Byte 1 = 10 00 0101 = 0xA5
Byte 2 = 1000 0101 = 0x85

Código binario: 0xA585
```

### Tabla de los 4 DTCs del emulador

| DTC | Código | Byte 1 | Byte 2 | Descripción |
|-----|--------|--------|--------|-------------|
| B2585 | 0xA585 | 0xA5 | 0x85 | Park Lamps Control Circuit (izq) |
| B3867 | 0xA867 | 0xA8 | 0x67 | Right Park Lamp Control Circuit |
| B3881 | 0xA881 | 0xA8 | 0x81 | Tail Lamp Circuit |
| B3882 | 0xA882 | 0xA8 | 0x82 | Right Tail Lamp Circuit |

---

## 6. Cheat Sheet

```
LO QUE QUIERES HACER              COMANDO / SOLUCIÓN
─────────────────────────────────────────────────────
Leer DTCs confirmados             03 00
Leer DTCs pendientes              07 00
Leer DTCs permanentes             0A 00
Leer DTCs (UDS)                   19 02 AF
Borrar TODOS los DTCs             04 00  (o 14 FF FF FF)
Borrar SOLO un DTC específico     IMPOSIBLE (no existe)
Borrar y que no vuelvan           Repara la falla eléctrica
```
