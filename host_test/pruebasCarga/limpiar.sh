#!/bin/bash

BASE_DIR="$(cd "$(dirname "$0")/.." && pwd)"

rm -f "$BASE_DIR/scripts_clientes/latencias_grupo_1.csv"
rm -f "$BASE_DIR/scripts_clientes/latencias_grupo_2.csv"

rm -f "$BASE_DIR/resultados_carga/raw/"*.csv
rm -f "$BASE_DIR/resultados_carga/resumen/"*.csv
rm -f "$BASE_DIR/resultados_carga/graficas/"*.png

echo "Resultados anteriores eliminados."