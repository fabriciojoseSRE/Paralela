#!/usr/bin/env python3
"""
gera_graficos.py
Le resultados.csv (gerado por run_benchmarks.sh) e produz:
  - tabelas (tempo, speedup, eficiencia) impressas no terminal e em tabelas.txt
  - graficos de coluna: speedup x tamanho e eficiencia x tamanho (2 e 4 threads)

Uso: python3 gera_graficos.py
"""
import csv
import collections
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

THREADS = [1, 2, 4]

def carrega(path="resultados.csv"):
    # dados[alg][n][t] = tempo
    dados = collections.defaultdict(lambda: collections.defaultdict(dict))
    with open(path) as f:
        for row in csv.DictReader(f):
            alg, n, t, tempo = row["algoritmo"], int(row["tamanho"]), int(row["threads"]), float(row["tempo"])
            dados[alg][n][t] = tempo
    return dados

def metricas(dados):
    """Retorna speedup[alg][n][t] e eficiencia[alg][n][t]."""
    sp = collections.defaultdict(lambda: collections.defaultdict(dict))
    ef = collections.defaultdict(lambda: collections.defaultdict(dict))
    for alg in dados:
        for n in dados[alg]:
            t_serial = dados[alg][n].get(1)
            for t in THREADS:
                if t in dados[alg][n] and t_serial:
                    s = t_serial / dados[alg][n][t]
                    sp[alg][n][t] = s
                    ef[alg][n][t] = s / t
    return sp, ef

def imprime_tabelas(dados, sp, ef, saida="tabelas.txt"):
    linhas = []
    for alg in dados:
        ns = sorted(dados[alg])
        for titulo, fonte, fmt in [("Tempo de execucao (s)", dados, "{:.4f}"),
                                   ("Speedup", sp, "{:.3f}"),
                                   ("Eficiencia", ef, "{:.3f}")]:
            linhas.append(f"\n### {alg} - {titulo}")
            linhas.append("Tamanho |  1 (serial) |   2 threads |   4 threads")
            for n in ns:
                cels = []
                for t in THREADS:
                    v = fonte[alg][n].get(t)
                    cels.append(fmt.format(v) if v is not None else "   -   ")
                linhas.append(f"{n:>7} | {cels[0]:>11} | {cels[1]:>11} | {cels[2]:>11}")
    txt = "\n".join(linhas)
    print(txt)
    with open(saida, "w") as f:
        f.write(txt + "\n")

def grafico_colunas(metrica, nome, ylabel, titulo):
    for alg in metrica:
        ns = sorted(metrica[alg])
        x = np.arange(len(ns))
        largura = 0.35
        v2 = [metrica[alg][n].get(2, 0) for n in ns]
        v4 = [metrica[alg][n].get(4, 0) for n in ns]
        fig, ax = plt.subplots(figsize=(7, 4.5))
        b2 = ax.bar(x - largura/2, v2, largura, label="2 threads")
        b4 = ax.bar(x + largura/2, v4, largura, label="4 threads")
        # valor exato (2 casas decimais) acima de cada barra
        ax.bar_label(b2, fmt="%.2f", padding=2, fontsize=8)
        ax.bar_label(b4, fmt="%.2f", padding=2, fontsize=8)
        ax.set_xticks(x); ax.set_xticklabels([str(n) for n in ns])
        ax.set_xlabel("Tamanho da entrada"); ax.set_ylabel(ylabel)
        ax.set_title(f"{titulo} - {alg}")
        ax.legend(); ax.grid(axis="y", alpha=0.3)
        # folga no topo para os rotulos nao serem cortados
        ax.margins(y=0.12)
        fig.tight_layout()
        out = f"{nome}_{alg}.png"
        fig.savefig(out, dpi=120); plt.close(fig)
        print("salvo:", out)

if __name__ == "__main__":
    dados = carrega()
    sp, ef = metricas(dados)
    imprime_tabelas(dados, sp, ef)
    grafico_colunas(sp, "speedup", "Speedup", "Speedup x Tamanho")
    grafico_colunas(ef, "eficiencia", "Eficiencia", "Eficiencia x Tamanho")