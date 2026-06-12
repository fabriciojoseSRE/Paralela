/*
 * heapsort_paralelo.c
 * Heap Sort - versao PARALELA (OpenMP)
 *
 * Uso: ./heapsort_paralelo <tamanho> <vetor.in>
 * Saida: vetor_ordenado.out
 *
 * Estrategia: paraleliza-se apenas a FASE DE CONSTRUCAO do heap, processando
 * um NIVEL da arvore por vez. Dentro de um mesmo nivel, os nos tem sub-arvores
 * DISJUNTAS, logo as operacoes 'desce' nao tem dependencia de dados entre si e
 * podem rodar em paralelo. Entre niveis ha dependencia (um pai so pode descer
 * depois que os filhos ja sao heaps), garantida pela barreira implicita ao fim
 * de cada 'parallel for'.
 *
 * A FASE DE EXTRACAO e' intrinsecamente SEQUENCIAL: cada 'desce(0, fim)' depende
 * do estado deixado pela iteracao anterior. Por isso o ganho do heapsort com
 * paralelismo e' limitado (vide discussao no relatorio).
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>

static void troca(float *a, float *b) { float t = *a; *a = *b; *b = t; }

static void desce(float *v, int i, int tam) {
    for (;;) {
        int esq = 2 * i + 1, dir = 2 * i + 2, maior = i;
        if (esq < tam && v[esq] > v[maior]) maior = esq;
        if (dir < tam && v[dir] > v[maior]) maior = dir;
        if (maior == i) break;
        troca(&v[i], &v[maior]);
        i = maior;
    }
}

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

    /* Fase 1: construcao do max-heap, NIVEL A NIVEL (de baixo para cima) */
    int ultimo_interno = n / 2 - 1;
    if (ultimo_interno >= 0) {
        int nivel_max = (int)floor(log2((double)(ultimo_interno + 1)));
        for (int nivel = nivel_max; nivel >= 0; nivel--) {
            int primeiro = (1 << nivel) - 1;        /* primeiro indice do nivel */
            int ultimo   = (1 << (nivel + 1)) - 2;  /* ultimo indice do nivel   */
            if (ultimo > ultimo_interno) ultimo = ultimo_interno;

            /* Nos do mesmo nivel => sub-arvores disjuntas => sem dependencia */
            #pragma omp parallel for schedule(dynamic, 64)
            for (int i = primeiro; i <= ultimo; i++)
                desce(v, i, n);
        }
    }

    /* Fase 2: extracao do maximo - SEQUENCIAL (dependencia entre iteracoes) */
    for (int fim = n - 1; fim > 0; fim--) {
        troca(&v[0], &v[fim]);
        desce(v, 0, fim);
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
