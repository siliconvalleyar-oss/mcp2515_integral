# Configuración

## Archivo de Configuración

El proyecto soporta un archivo de configuracion `config/obd2_rpi.conf`
con formato clave=valor.

### Opciones

| Clave | Default | Descripción |
|-------|---------|-------------|
| `bt_mac` | `00:1D:A5:07:23:6E` | MAC del ELM327 |
| `bt_channel` | `1` | Canal RFCOMM |
| `bt_timeout` | `30` | Timeout de conexion (segundos) |
| `spi_device` | `/dev/spidev0.0` | Dispositivo SPI |
| `spi_speed` | `8000000` | Velocidad SPI (Hz) |
| `gpio_chip` | `0` | Chip gpiochip |
| `pin_dc` | `25` | GPIO para Data/Command |
| `pin_rst` | `17` | GPIO para Reset |
| `obd_poll_ms` | `800` | Intervalo polling OBD (ms) |
| `display_ms` | `400` | Intervalo refresco display (ms) |
| `auto_rotate` | `true` | Auto-rotacion de paginas |
| `rotate_interval` | `6` | Intervalo auto-rotacion (segundos) |
| `enable_gm` | `true` | Habilitar comandos GM modo 22 |
| `log_dir` | `.` | Directorio para logs CSV |
| `log_on_start` | `false` | Iniciar logging automaticamente |

### Argumentos CLI

```
./bin/obd2_rpi [MAC] [SPI_DEV] [PIN_DC] [PIN_RST] [CONFIG_PATH]
```

Usar `-` para CONFIG_PATH si no se quiere especificar.

### Prioridad

1. Argumentos CLI (mayor prioridad)
2. Archivo de configuracion
3. Valores por defecto

### Ejemplo

```bash
# Solo MAC
./bin/obd2_rpi 00:1D:A5:07:23:6E

# Todo manual
./bin/obd2_rpi 00:1D:A5:07:23:6E /dev/spidev0.0 25 17

# Con archivo de config
./bin/obd2_rpi 00:1D:A5:07:23:6E /dev/spidev0.0 25 17 config/obd2_rpi.conf

# Solo config (args con -)
./bin/obd2_rpi - - - - config/obd2_rpi.conf
```
