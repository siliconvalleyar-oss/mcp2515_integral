#!/bin/bash
# Ajustar la MAC del ELM327 antes de instalar
SERVICE_FILE="obd2_rpi.service"
echo "MAC actual del ELM327 en el servicio:"
grep ExecStart $SERVICE_FILE
read -p "Ingrese la MAC del ELM327 (Enter para conservar actual): " mac
if [ -n "$mac" ]; then
    sed -i "s/[0-9A-Fa-f:]\{17\}/$mac/g" $SERVICE_FILE
    echo "MAC actualizada a: $mac"
fi
sudo cp $SERVICE_FILE /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable obd2_rpi
sudo systemctl start obd2_rpi
echo "[OK] Servicio instalado y arrancado"
echo "     sudo systemctl status obd2_rpi"
echo "     sudo journalctl -u obd2_rpi -f"
