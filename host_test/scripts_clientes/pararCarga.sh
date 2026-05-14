#!/bin/bash

# parar_carga.sh
#
# Uso:
#   ./parar_carga.sh baja
#   ./parar_carga.sh media
#   ./parar_carga.sh alta

if [ $# -ne 1 ]; then
    echo "Uso: $0 <baja|media|alta>"
    exit 1
fi

PERFIL=$1
PID_FILE="logs_carga/pids_${PERFIL}.txt"

if [ ! -f "$PID_FILE" ]; then
    echo "No existe $PID_FILE"
    echo "No hay procesos registrados para el perfil $PERFIL"
    exit 1
fi

echo "Parando clientes del perfil $PERFIL..."

while read -r PID; do
    if kill -0 "$PID" 2>/dev/null; then
        kill "$PID"
        echo "Proceso $PID parado"
    fi
done < "$PID_FILE"

rm -f "$PID_FILE"

echo "Carga $PERFIL detenida."