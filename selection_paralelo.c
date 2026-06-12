/*
 * selection_paralelo.c
 * Selection Sort - versao PARALELA (OpenMP)
 *
 * Uso: ./selection_paralelo <tamanho> <vetor.in>
 * Saida: vetor_ordenado.out
 *
 * Estrategia: o laco EXTERNO e' inerentemente sequencial (cada passada depende
 * da anterior, pois a troca em v[i] muda o sub-vetor das proximas passadas).
 * O que se paraleliza e' o laco INTERNO de BUSCA DO MINIMO.
 *
 * Para encontrar o minimo (valor + indice) em paralelo usamos uma REDUCAO
 * definida pelo usuario (declare reduction), pois o OpenMP nao tem reducao
 * pronta que devolva tambem o INDICE do menor elemento.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

/* Par (valor, indice) usado na reducao de minimo */
typedef struct { float val; int idx; } MinIdx;

/* Combinador: fica com o menor valor; em empate, o menor indice */
#pragma omp declare reduction(minloc : MinIdx :                         \
        omp_out = (omp_in.val < omp_out.val ||                          \
                  (omp_in.val == omp_out.val && omp_in.idx < omp_out.idx)) \
                  ? omp_in : omp_out)                                    \
        initializer(omp_priv = {1e30f, -1})

static int arquivo_existe(const char *nome) {
    FILE *f = fopen(nome, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}

static void gera_vetor(const char *nome, int n) {
    FILE *f = fopen(nome, "w");
    if (!f) { perror("fopen (gerar)"); exit(EXIT_FAILURE); }
    srand(42);
    for (int i = 0; i < n; i++)
        fprintf(f, "%f\n", (float)(rand() % 1000000) / 100.0f);
    fclose(f);
}

static void carrega_vetor(const char *nome, float *v, int n) {
    FILE *f = fopen(nome, "r");
    if (!f) { perror("fopen (carregar)"); exit(EXIT_FAILURE); }
    for (int i = 0; i < n; i++)
        if (fscanf(f, "%f", &v[i]) != 1) {
            fprintf(stderr, "Erro lendo %s\n", nome); exit(EXIT_FAILURE);
        }
    fclose(f);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <tamanho> <vetor.in>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int n = atoi(argv[1]);
    const char *arq = argv[2];

    if (!arquivo_existe(arq)) gera_vetor(arq, n);

    float *v = malloc((size_t)n * sizeof(float));
    if (!v) { fprintf(stderr, "Sem memoria\n"); return EXIT_FAILURE; }
    carrega_vetor(arq, v, n);

    /* ---- Medicao APENAS do algoritmo ---- */
    double t0 = omp_get_wtime();

    for (int i = 0; i < n - 1; i++) {
        MinIdx m = { v[i], i };

        /*
         * Laco interno paralelizado. Cada thread varre uma faixa do vetor e
         * mantem seu minimo local; ao final, a reducao 'minloc' combina os
         * minimos locais em um unico minimo global (valor + indice).
         * A combinacao final e a REGIAO CRITICA, tratada internamente pela
         * clausula reduction (nao ha 'critical' explicito).
         */
        #pragma omp parallel for reduction(minloc:m) schedule(static)
        for (int j = i + 1; j < n; j++) {
            if (v[j] < m.val) { m.val = v[j]; m.idx = j; }
        }

        if (m.idx != i) {
            float tmp = v[i];
            v[i] = v[m.idx];
            v[m.idx] = tmp;
        }
    }

    double t1 = omp_get_wtime();
    /* -------------------------------------- */

    FILE *out = fopen("vetor_ordenado.out", "w");
    if (!out) { perror("fopen (saida)"); return EXIT_FAILURE; }
    for (int i = 0; i < n; i++) fprintf(out, "%f\n", v[i]);
    fclose(out);

    fprintf(stderr, "[paralelo] n=%d  threads=%d  tempo=%.6f s\n",
            n, omp_get_max_threads(), t1 - t0);

    free(v);
    return EXIT_SUCCESS;
}
