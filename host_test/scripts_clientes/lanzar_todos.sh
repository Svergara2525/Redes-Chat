#!/bin/bash

# lanzar_todos.sh
#
# Uso:
#   ./lanzar_todos.sh 1 3
#   ./lanzar_todos.sh 1 10
#
# Parámetros:
#   1 -> grupo
#   2 -> número de clientes por perfil

if [ $# -ne 2 ]; then
    echo "Uso: $0 <grupo> <num_clientes_por_perfil>"
    echo ""
    echo "Ejemplos:"
    echo "  $0 1 3"
    echo "  $0 1 10"
    exit 1
fi

GRUPO=$1
NUM_CLIENTES=$2

mkdir -p logs_carga

echo "============================================="
echo " Lanzando todos los perfiles a la vez"
echo " Grupo: $GRUPO"
echo " Clientes por perfil: $NUM_CLIENTES"
echo " Total clientes: $((NUM_CLIENTES * 3))"
echo "============================================="

./lanzar_carga.sh baja "$GRUPO" "$NUM_CLIENTES"
./lanzar_carga.sh media "$GRUPO" "$NUM_CLIENTES"
./lanzar_carga.sh alta "$GRUPO" "$NUM_CLIENTES"

echo ""
echo "Todos los perfiles han sido lanzados."
echo ""
echo "Para parar todo:"
echo "  ./parar_todos.sh"