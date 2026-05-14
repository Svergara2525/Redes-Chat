#!/bin/bash

BASE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
CLIENT_DIR="$BASE_DIR/scripts_clientes"
OUT_DIR="$BASE_DIR/resultados_carga/raw"

mkdir -p "$OUT_DIR"

# Cambia br6 si vuestra interfaz se llama diferente.
IFACE="br6"

run_test_g1() {
    TEST_ID="$1"
    CARGA="$2"
    NUM_CLIENTES="$3"
    NUM_MENSAJES="$4"
    INTERVALO="$5"

    CSV="$OUT_DIR/test_$(printf "%02d" "$TEST_ID")_G1_${CARGA}_${NUM_MENSAJES}msg.csv"

    echo ""
    echo "=============================================="
    echo "TEST $TEST_ID - G1 - carga $CARGA"
    echo "Clientes: $NUM_CLIENTES"
    echo "Mensajes por cliente: $NUM_MENSAJES"
    echo "Intervalo: cada $INTERVALO segundos"
    echo "Salida: $CSV"
    echo "=============================================="

    rm -f "$CLIENT_DIR/latencias_grupo_1.csv"
    rm -f "$CSV"

    cd "$CLIENT_DIR" || exit 1

    ./cliente_recv receptor_g1 1 "$IFACE" &
    RECV_PID=$!

    sleep 2

    SENDER_PIDS=()

    for i in $(seq 1 "$NUM_CLIENTES"); do
        ./cliente_send "g1_c$i" 1 "test_${TEST_ID}_${CARGA}_G1" "$NUM_MENSAJES" "$INTERVALO" &
        SENDER_PIDS+=($!)
    done

    for pid in "${SENDER_PIDS[@]}"; do
        wait "$pid"
    done

    sleep 3

    kill "$RECV_PID" 2>/dev/null || true
    wait "$RECV_PID" 2>/dev/null || true

    if [ -f "$CLIENT_DIR/latencias_grupo_1.csv" ]; then
        mv "$CLIENT_DIR/latencias_grupo_1.csv" "$CSV"
        echo "CSV guardado en: $CSV"
    else
        echo "AVISO: no se ha generado latencias_grupo_1.csv"
    fi

    sleep 2
}

# Carga baja: 5 clientes, cada 5 segundos
run_test_g1 1 baja 5 1 5
run_test_g1 2 baja 5 10 5
run_test_g1 3 baja 5 100 5

# Carga media: 20 clientes, cada 2 segundos
run_test_g1 4 media 20 1 2
run_test_g1 5 media 20 10 2
run_test_g1 6 media 20 100 2

# Carga alta: 50 clientes, cada 1 segundo
run_test_g1 7 alta 50 1 1
run_test_g1 8 alta 50 10 1
run_test_g1 9 alta 50 100 1

echo ""
echo "Todas las pruebas de G1 han terminado."