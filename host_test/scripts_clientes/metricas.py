#!/usr/bin/env python3

import csv
import sys
from statistics import mean, median

def percentile(values, p):
    if not values:
        return None

    values = sorted(values)
    k = (len(values) - 1) * (p / 100)
    f = int(k)
    c = min(f + 1, len(values) - 1)

    if f == c:
        return values[f]

    return values[f] + (values[c] - values[f]) * (k - f)

def analizar_csv(filename):
    latencias = []
    cumplen_200ms = 0
    total = 0

    with open(filename, newline="") as f:
        reader = csv.DictReader(f)

        for row in reader:
            try:
                lat = int(row["latency_ms"])
            except (KeyError, ValueError):
                continue

            latencias.append(lat)
            total += 1

            if lat < 200:
                cumplen_200ms += 1

    if total == 0:
        print("No hay datos suficientes.")
        return

    print("=============================================")
    print(f" Archivo analizado: {filename}")
    print("=============================================")
    print(f" Mensajes recibidos:        {total}")
    print(f" Latencia media:            {mean(latencias):.2f} ms")
    print(f" Latencia mediana:          {median(latencias):.2f} ms")
    print(f" Latencia mínima:           {min(latencias)} ms")
    print(f" Latencia máxima:           {max(latencias)} ms")
    print(f" Percentil 95:              {percentile(latencias, 95):.2f} ms")
    print(f" Percentil 99:              {percentile(latencias, 99):.2f} ms")
    print(f" Mensajes < 200 ms:         {cumplen_200ms}")
    print(f" Porcentaje < 200 ms:       {(cumplen_200ms / total) * 100:.2f}%")
    print("=============================================")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Uso: {sys.argv[0]} <latencias_grupo_X.csv>")
        sys.exit(1)

    analizar_csv(sys.argv[1])