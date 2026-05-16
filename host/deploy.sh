#!/bin/bash

echo "Arrancando contenedores..."
lxc start m1 m2 m3 2>/dev/null

sleep 2

echo "Matando servidores antiguos si existen..."
lxc exec m1 -- pkill -f servidor_central 2>/dev/null
lxc exec m2 -- pkill -f servidor_grupo_m2 2>/dev/null
lxc exec m3 -- pkill -f servidor_grupo_m3 2>/dev/null

sleep 1

echo "Levantando M1 - servidor central..."
lxc exec m1 -- sh -c "nohup stdbuf -oL -eL ./servidor_central > m1.log 2>&1 &"

echo "Levantando M2 - servidor grupo 1..."
lxc exec m2 -- sh -c "nohup stdbuf -oL -eL ./servidor_grupo_m2 1 4001 ff15::1 5001 > m2.log 2>&1 &"

echo "Levantando M3 - servidor grupo 2..."
lxc exec m3 -- sh -c "nohup stdbuf -oL -eL ./servidor_grupo_m3 2 4002 ff15::2 5002 > m3.log 2>&1 &"


sleep 2

echo "Comprobando puertos..."
lxc exec m1 -- ss -tulpen -6 | grep 3000
lxc exec m2 -- ss -tulpen -6 | grep 4001
lxc exec m3 -- ss -tulpen -6 | grep 4002

echo ""
echo "Servidores levantados."
echo "Logs:"
echo "  tail -f m1.log"
echo "  tail -f m2.log"
echo "  tail -f m3.log"
