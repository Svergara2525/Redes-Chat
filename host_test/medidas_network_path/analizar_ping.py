#!/usr/bin/env python3

import re
import csv
import glob
import statistics
from pathlib import Path

def analizar_ping(path):
    rtts = []
    transmitted = ""
    received = ""
    packet_loss = ""

    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            m_time = re.search(r"time=([\d.]+)\s*ms", line)
            if m_time:
                rtts.append(float(m_time.group(1)))

            m_loss = re.search(
                r"(\d+)\s+packets transmitted,\s+(\d+)\s+received,.*?([\d.]+)%\s+packet loss",
                line
            )
            if m_loss:
                transmitted = int(m_loss.group(1))
                received = int(m_loss.group(2))
                packet_loss = float(m_loss.group(3))

    if not rtts:
        return None

    return {
        "fichero": Path(path).name,
        "muestras": len(rtts),
        "rtt_min_ms": min(rtts),
        "rtt_media_ms": statistics.mean(rtts),
        "rtt_max_ms": max(rtts),
        "rtt_varianza": statistics.variance(rtts) if len(rtts) > 1 else 0.0,
        "paquetes_transmitidos": transmitted,
        "paquetes_recibidos": received,
        "perdida_paquetes_porcentaje": packet_loss,
    }

def main():
    archivos = sorted(glob.glob("ping_*.txt"))
    resultados = []

    if not archivos:
        print("No hay ficheros ping_*.txt")
        return

    print("Resumen ping IPv6")
    print("=" * 110)
    print(f"{'Fichero':15} {'N':>5} {'Min(ms)':>10} {'Media(ms)':>12} {'Max(ms)':>10} {'Varianza':>12} {'Loss(%)':>10}")
    print("-" * 110)

    for archivo in archivos:
        r = analizar_ping(archivo)
        if r is None:
            print(f"{archivo}: sin datos")
            continue

        resultados.append(r)
        print(
            f"{r['fichero']:15} "
            f"{r['muestras']:5d} "
            f"{r['rtt_min_ms']:10.3f} "
            f"{r['rtt_media_ms']:12.3f} "
            f"{r['rtt_max_ms']:10.3f} "
            f"{r['rtt_varianza']:12.6f} "
            f"{r['perdida_paquetes_porcentaje']:10}"
        )

    with open("resumen_ping.csv", "w", newline="") as f:
        campos = [
            "fichero",
            "muestras",
            "rtt_min_ms",
            "rtt_media_ms",
            "rtt_max_ms",
            "rtt_varianza",
            "paquetes_transmitidos",
            "paquetes_recibidos",
            "perdida_paquetes_porcentaje",
        ]
        writer = csv.DictWriter(f, fieldnames=campos)
        writer.writeheader()
        writer.writerows(resultados)

    print()
    print("Guardado: resumen_ping.csv")

if __name__ == "__main__":
    main()
