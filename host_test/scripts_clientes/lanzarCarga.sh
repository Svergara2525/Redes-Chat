#!/bin/bash

# lanzar_carga.sh
#
# Uso:
#   ./lanzar_carga.sh baja 1 3
#   ./lanzar_carga.sh media 1 3
#   ./lanzar_carga.sh alta 1 3
#
# Después, para escalar:
#   ./lanzar_carga.sh baja 1 10
#   ./lanzar_carga.sh media 1 10
#   ./lanzar_carga.sh alta 1 10

if [ $# -ne 3 ]; then
    echo "Uso: $0 <baja|media|alta> <grupo> <num_clientes>"
    echo ""
    echo "Ejemplos:"
    echo "  $0 baja 1 3"
    echo "  $0 media 1 3"
    echo "  $0 alta 1 3"
    echo "  $0 alta 1 10"
    exit 1
fi

PERFIL=$1
GRUPO=$2
NUM_CLIENTES=$3

case "$PERFIL" in
    baja)
        INTERVALO=5
        ;;
    media)
        INTERVALO=2
        ;;
    alta)
        INTERVALO=1
        ;;
    *)
        echo "Perfil inválido: $PERFIL"
        echo "Usa: baja, media o alta"
        exit 1
        ;;
esac

mkdir -p logs_carga

echo "============================================="
echo " Lanzando perfil de carga: $PERFIL"
echo " Grupo: $GRUPO"
echo " Clientes: $NUM_CLIENTES"
echo " Intervalo: cada $INTERVALO segundos"
echo "============================================="

for i in $(seq 1 "$NUM_CLIENTES"); do
    CLIENT_ID="${PERFIL}-client-${i}"
    MENSAJE="Mensaje de prueba perfil=${PERFIL} cliente=${i}"

    echo "Lanzando $CLIENT_ID..."

    ./cliente_send "$CLIENT_ID" "$GRUPO" "$MENSAJE" "$INTERVALO" \
        > "logs_carga/${CLIENT_ID}.log" 2>&1 &

    echo $! >> "logs_carga/pids_${PERFIL}.txt"
done

echo ""
echo "Clientes lanzados."
echo "PIDs guardados en logs_carga/pids_${PERFIL}.txt"
echo ""
echo "Para parar esta carga:"
echo "  ./parar_carga.sh $PERFIL"