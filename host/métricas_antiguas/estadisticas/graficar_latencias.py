#!/usr/bin/env python3

import csv
import glob
from pathlib import Path

import matplotlib.pyplot as plt

def leer_latencias(path):
    latencias = []

    with open(path, newline="") as f:
        reader = csv.DictReader(f)

        for row in reader:
            try:
                latencias.append(float(row["latency_ms"]))
            except (KeyError, ValueError):
                pass

    return latencias

def grafica_linea(path, latencias):
    nombre = Path(path).stem

    plt.figure(figsize=(10, 5))
    plt.plot(range(1, len(latencias) + 1), latencias, marker="o", markersize=2, linewidth=1)
    plt.axhline(y=200, linestyle="--", linewidth=1)
    plt.title(f"Evolución de latencia - {nombre}")
    plt.xlabel("Mensaje recibido")
    plt.ylabel("Latencia extremo a extremo (ms)")
    plt.grid(True)
    plt.tight_layout()

    salida = f"{nombre}_linea.png"
    plt.savefig(salida, dpi=150)
    plt.close()

    print(f"Generada {salida}")

def grafica_histograma(path, latencias):
    nombre = Path(path).stem

    plt.figure(figsize=(10, 5))
    plt.hist(latencias, bins=20)
    plt.axvline(x=200, linestyle="--", linewidth=1)
    plt.title(f"Distribución de latencias - {nombre}")
    plt.xlabel("Latencia extremo a extremo (ms)")
    plt.ylabel("Frecuencia")
    plt.grid(True)
    plt.tight_layout()

    salida = f"{nombre}_histograma.png"
    plt.savefig(salida, dpi=150)
    plt.close()

    print(f"Generada {salida}")

def main():
    archivos = sorted(glob.glob("latencias_prueba_*.csv"))

    if not archivos:
        print("No se han encontrado ficheros latencias_prueba_*.csv")
        return

    for archivo in archivos:
        latencias = leer_latencias(archivo)

        if not latencias:
            print(f"{archivo}: sin datos")
            continue

        grafica_linea(archivo, latencias)
        grafica_histograma(archivo, latencias)

if __name__ == "__main__":
    main()
