#!/usr/bin/env python3

import csv
import glob
import statistics
from pathlib import Path

def analizar_csv(path):
    latencias = []

    with open(path, newline="") as f:
        reader = csv.DictReader(f)

        for row in reader:
            try:
                latencias.append(float(row["latency_ms"]))
            except (KeyError, ValueError):
                pass

    if not latencias:
        return None

    total = len(latencias)
    media = statistics.mean(latencias)
    minimo = min(latencias)
    maximo = max(latencias)

    if total > 1:
        varianza = statistics.variance(latencias)
    else:
        varianza = 0.0

    cumplen = sum(1 for x in latencias if x < 200)
    porcentaje_cumplen = (cumplen / total) * 100

    return {
        "fichero": Path(path).name,
        "mensajes": total,
        "media_ms": media,
        "min_ms": minimo,
        "max_ms": maximo,
        "varianza": varianza,
        "cumplen_200ms": cumplen,
        "porcentaje_200ms": porcentaje_cumplen,
    }

def main():
    archivos = sorted(glob.glob("latencias_prueba_*.csv"))

    if not archivos:
        print("No se han encontrado ficheros latencias_prueba_*.csv")
        return

    resumen = []

    for archivo in archivos:
        resultado = analizar_csv(archivo)
        if resultado is not None:
            resumen.append(resultado)

    print("Resumen de latencias")
    print("=" * 100)
    print(f"{'Fichero':30} {'N':>8} {'Media':>10} {'Min':>10} {'Max':>10} {'Varianza':>12} {'<200ms':>10}")
    print("-" * 100)

    for r in resumen:
        print(
            f"{r['fichero']:30} "
            f"{r['mensajes']:8d} "
            f"{r['media_ms']:10.2f} "
            f"{r['min_ms']:10.2f} "
            f"{r['max_ms']:10.2f} "
            f"{r['varianza']:12.2f} "
            f"{r['porcentaje_200ms']:9.2f}%"
        )

    with open("resumen_latencias.csv", "w", newline="") as f:
        fieldnames = [
            "fichero",
            "mensajes",
            "media_ms",
            "min_ms",
            "max_ms",
            "varianza",
            "cumplen_200ms",
            "porcentaje_200ms",
        ]

        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(resumen)

    print()
    print("Resumen guardado en resumen_latencias.csv")

if __name__ == "__main__":
    main()
