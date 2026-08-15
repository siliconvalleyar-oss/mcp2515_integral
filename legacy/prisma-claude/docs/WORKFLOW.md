# Workflow remoto de compilación

Comandos para compilar y ejecutar remotamente en la Raspberry Pi desde la PC de desarrollo.

## Compilación y ejecución remotas

```bash
ssh $USER@$HOSTNAME "cd /home/$USER/src/$LINK && make clean && make -j4 && sudo make run"
```
$LINK="prisma-emulator"
## Explicación

- `make clean` elimina binarios y objetos previos
- `make -j4` compila en paralelo usando 4 jobs
- `make run` ejecuta el scanner con `sudo`
- La ruta remota del proyecto es `/home/$USER/src/$LINK`

## Prerequisitos

- Acceso SSH configurado hacia ` $USER@$HOSTNAME`
- Dependencias instaladas en la Raspberry Pi: `libbcm2835-dev`, `build-essential`, `cmake`
