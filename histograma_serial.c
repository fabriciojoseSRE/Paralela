/*
 * histograma_serial.c
 * Histograma de um conjunto de valores inteiros - versao SERIAL
 *
 * Uso:
 *   ./histograma_serial                 -> le "vetor.in"
 *   ./histograma_serial <tamanho>       -> le/gera "vetor.in" com <tamanho> ints
 *   ./histograma_serial <tamanho> <arq> -> le/gera <arq> com <tamanho> ints
 * Saida: histograma.out  (linhas "valor contador")
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

static int arquivo_existe(const char *nome) {
    FILE *f = fopen(nome, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}

/* Gera 'n' inteiros aleatorios na faixa [-500, 499] */
static void gera_vetor(const char *nome, int n) {
    FILE *f = fopen(nome, "w");
    if (!f) { perror("fopen (gerar)"); exit(EXIT_FAILURE); }
    srand(7);
    for (int i = 0; i < n; i++)
        fprintf(f, "%d\n", (rand() % 1000) - 500);
    fclose(f);
}

/* Le todos os inteiros do arquivo; retorna o vetor e escreve a contagem em *n */
static int *carrega_vetor(const char *nome, int *n) {
    FILE *f = fopen(nome, "r");
    if (!f) { perror("fopen (carregar)"); exit(EXIT_FAILURE); }
    int cap = 1024, qtd = 0, x;
    int *v = malloc((size_t)cap * sizeof(int));
    while (fscanf(f, "%d", &x) == 1) {
        if (qtd == cap) { cap *= 2; v = realloc(v, (size_t)cap * sizeof(int)); }
        v[qtd++] = x;
    }
    fclose(f);
    *n = qtd;
    return v;
}

int main(int argc, char *argv[]) {
    const char *arq = "vetor.in";
    int tamanho_pedido = -1;

    if (argc >= 2) tamanho_pedido = atoi(argv[1]);
    if (argc >= 3) arq = argv[2];

    if (!arquivo_existe(arq)) {
        if (tamanho_pedido <= 0) {
            fprintf(stderr, "Arquivo %s nao existe e nenhum tamanho foi dado.\n", arq);
            return EXIT_FAILURE;
        }
        gera_vetor(arq, tamanho_pedido);
    }

    int n;
    int *v = carrega_vetor(arq, &n);
    if (n == 0) { fprintf(stderr, "Vetor vazio\n"); return EXIT_FAILURE; }

    /* Faixa de valores (definicao do tamanho do histograma) - fora da medicao */
    int vmin = v[0], vmax = v[0];
    for (int i = 1; i < n; i++) {
        if (v[i] < vmin) vmin = v[i];
        if (v[i] > vmax) vmax = v[i];
    }
    int faixa = vmax - vmin + 1;
    long *hist = calloc((size_t)faixa, sizeof(long));
    if (!hist) { fprintf(stderr, "Sem memoria\n"); return EXIT_FAILURE; }

    /* ---- Medicao APENAS do algoritmo ---- */
    double t0 = omp_get_wtime();

    for (int i = 0; i < n; i++)
        hist[v[i] - vmin]++;

    double t1 = omp_get_wtime();
    /* -------------------------------------- */

    FILE *out = fopen("histograma.out", "w");
    if (!out) { perror("fopen (saida)"); return EXIT_FAILURE; }
    for (int k = 0; k < faixa; k++)
        fprintf(out, "%d %ld\n", vmin + k, hist[k]);
    fclose(out);

    fprintf(stderr, "[serial]  n=%d  faixa=%d  tempo=%.6f s\n", n, faixa, t1 - t0);

    free(v); free(hist);
    return EXIT_SUCCESS;
}
