#!/usr/bin/env bash
# run_benchmarks.sh
# Coleta Tempo de execucao, Speedup e Eficiencia para todos os algoritmos.
# Rode em uma maquina com 2+ nucleos fisicos. Gera o arquivo resultados.csv
#
# Uso:  make && ./run_benchmarks.sh
#
# Cada combinacao e' executada REPET vezes e usamos o MENOR tempo (mais estavel).
# As entradas sao geradas UMA vez e reutilizadas (serial e paralelo usam os
# mesmos dados).

set -e
REPET=3
THREADS_LIST="1 2 4"        # 1 = serial
CSV="resultados.csv"

# ---- AJUSTE OS TAMANHOS: que o serial leve ~20-30 s no maior caso ----
declare -A SIZES
SIZES[matrixmult]="500 1000 2000"
SIZES[selection]="100000 200000 400000"
SIZES[heapsort]="100000 200000 8000000"
SIZES[histograma]="100000 200000 400000"

# extrai "tempo=NUMERO" do stderr do programa
extrai_tempo() { grep -oE 'tempo=[0-9.]+' | head -1 | cut -d= -f2; }

echo "algoritmo,tamanho,threads,tempo" > "$CSV"

bench_vetor() {  # $1=algoritmo $2=binario_base
    local alg=$1 base=$2
    for n in ${SIZES[$alg]}; do
        local IN="${alg}_${n}.in"
        rm -f "$IN"
        # gera a entrada uma vez (a 1a execucao cria o arquivo)
        ./${base}_serial $n "$IN" >/dev/null 2>&1
        for t in $THREADS_LIST; do
            local best=""
            for r in $(seq 1 $REPET); do
                if [ "$t" -eq 1 ]; then
                    tmp=$(./${base}_serial $n "$IN" 2>&1 >/dev/null | extrai_tempo)
                else
                    tmp=$(OMP_NUM_THREADS=$t ./${base}_paralelo $n "$IN" 2>&1 >/dev/null | extrai_tempo)
                fi
                if [ -z "$best" ] || awk "BEGIN{exit !($tmp < $best)}"; then best=$tmp; fi
            done
            echo "$alg,$n,$t,$best" >> "$CSV"
            echo "  $alg n=$n threads=$t -> ${best}s"
        done
        rm -f "$IN"
    done
}

bench_matrix() {
    for n in ${SIZES[matrixmult]}; do
        local A="mm_${n}_A.in" B="mm_${n}_B.in"
        rm -f "$A" "$B"
        ./matrixmult_serial $n "$A" "$B" >/dev/null 2>&1
        for t in $THREADS_LIST; do
            local best=""
            for r in $(seq 1 $REPET); do
                if [ "$t" -eq 1 ]; then
                    tmp=$(./matrixmult_serial $n "$A" "$B" 2>&1 >/dev/null | extrai_tempo)
                else
                    tmp=$(OMP_NUM_THREADS=$t ./matrixmult_paralelo $n "$A" "$B" 2>&1 >/dev/null | extrai_tempo)
                fi
                if [ -z "$best" ] || awk "BEGIN{exit !($tmp < $best)}"; then best=$tmp; fi
            done
            echo "matrixmult,$n,$t,$best" >> "$CSV"
            echo "  matrixmult n=$n threads=$t -> ${best}s"
        done
        rm -f "$A" "$B"
    done
}

echo ">> Multiplicacao matricial"; bench_matrix
echo ">> Selection sort";          bench_vetor selection selection
echo ">> Heap sort";               bench_vetor heapsort  heapsort
echo ">> Histograma";              bench_vetor histograma histograma

echo
echo "Pronto! Resultados em $CSV"
echo "Gere as tabelas/graficos com:  python3 gera_graficos.py"
