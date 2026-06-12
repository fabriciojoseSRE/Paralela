# Paralelização de Algoritmos com OpenMP

**Programação Paralela e Concorrente — CEFET/RJ, UNED Nova Friburgo**

---

## Observação sobre a metodologia de medição

> **Importante (leia antes de submeter):** os números das tabelas e os gráficos
> deste documento marcados como **EXEMPLO ILUSTRATIVO** servem apenas para mostrar
> o *formato* e a *ordem de grandeza* esperados. **Você deve substituí-los pelos
> dados reais medidos na sua máquina.** Para coletar os dados automaticamente,
> compile com `make` e rode `./run_benchmarks.sh`; depois `python3 gera_graficos.py`
> gera as tabelas e os gráficos de coluna a partir do `resultados.csv` produzido.
>
> Conforme o enunciado, escolha o maior tamanho de modo que a versão **serial**
> leve ~20–30 s. O tempo é medido com `omp_get_wtime()` **apenas em volta do
> algoritmo** (não inclui leitura/escrita de arquivos nem geração de dados). Cada
> medição é repetida 3 vezes e usamos o **menor** tempo. As entradas são geradas
> uma única vez e reutilizadas por serial e paralelo (mesmos dados).
>
> Ambiente sugerido no texto: CPU de 4 núcleos físicos, `gcc -O2 -fopenmp`,
> número de threads controlado por `OMP_NUM_THREADS`.

As métricas usadas:

- **Speedup:** S(p) = T_serial / T_paralelo(p)
- **Eficiência:** E(p) = S(p) / p

onde *p* é o número de threads.

---

# 1. Multiplicação Matricial

## 1.1. Descrição do algoritmo

Dadas duas matrizes quadradas *A* e *B* de ordem *n*, o produto *C = A × B* é
definido por:

> C[i][j] = Σ (k = 0 … n−1) A[i][k] · B[k][j]

Ou seja, cada elemento da matriz resultado é o **produto escalar** da linha *i* de
*A* pela coluna *j* de *B*.

**Exemplo (2×2):**

```
A = | 1  2 |     B = | 5  6 |
    | 3  4 |         | 7  8 |

C[0][0] = 1·5 + 2·7 = 19
C[0][1] = 1·6 + 2·8 = 22
C[1][0] = 3·5 + 4·7 = 43
C[1][1] = 3·6 + 4·8 = 50

C = | 19  22 |
    | 43  50 |
```

## 1.2. Complexidade assintótica (pior caso)

Há *n²* elementos a calcular, e cada um exige um somatório de *n* produtos:

> **O(n³)**

## 1.3. Implementação serial (trecho central)

```c
for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
        float soma = 0.0f;
        for (int k = 0; k < n; k++)
            soma += A[i * n + k] * B[k * n + j];
        C[i * n + j] = soma;
    }
}
```

(Código completo em `matrixmult_serial.c`.)

## 1.4. Regiões críticas e dependências de dados

- **Regiões críticas:** **nenhuma.** Cada thread, ao calcular um par (i, j),
  escreve em uma posição **exclusiva** `C[i][j]`. Não há duas iterações do laço
  externo escrevendo na mesma posição de memória.
- **Dependências de dados entre iterações de threads:** **nenhuma.** O cálculo de
  C[i][j] não usa nenhum outro elemento de C, apenas linhas de A e colunas de B
  (que são apenas **lidas**). As iterações são totalmente independentes — caso
  ideal para paralelismo.

## 1.5. Implementação paralela (OpenMP)

```c
#pragma omp parallel for collapse(2) schedule(static)
for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
        float soma = 0.0f;
        for (int k = 0; k < n; k++)
            soma += A[i * n + k] * B[k * n + j];
        C[i * n + j] = soma;
    }
}
```

**Estruturas usadas (e por quê):**

- `parallel for` distribui as iterações do laço externo entre as threads.
- `collapse(2)` **funde** os dois laços externos (i e j) em um único espaço de
  iterações de tamanho *n²*. Isso aumenta o grão de paralelismo e melhora o
  balanceamento quando *n* é pequeno em relação ao número de threads.
- `schedule(static)`: como todas as iterações têm o mesmo custo (*n* operações),
  a divisão estática em blocos iguais é a mais eficiente (menor overhead).
- As variáveis `k` e `soma` são declaradas **dentro** do laço, logo são
  automaticamente **privadas** por thread, evitando condição de corrida.

(Código completo em `matrixmult_paralelo.c`.)

## 1.6. Comparação de desempenho — *EXEMPLO ILUSTRATIVO*

Tamanhos escolhidos: 500×500, 1000×1000, 2000×2000.

**Tempo de execução (s)**

| Tamanho | 1 (serial) | 2 threads | 4 threads |
|--------:|-----------:|----------:|----------:|
| 500     | 0,050      | 0,027     | 0,024     |
| 1000    | 0,431      | 0,226     | 0,177     |
| 2000    | 8,848      | 4,934     | 2,619     |

**Speedup**

| Tamanho | 2 threads | 4 threads |
|--------:|----------:|----------:|
| 500     | 1,85      | 2,05      |
| 1000    | 1,91      | 2,43      |
| 2000    | 1,79      | 3,38      |

**Eficiência**

| Tamanho | 2 threads | 4 threads |
|--------:|----------:|----------:|
| 500     | 0,93      | 0,51      |
| 1000    | 0,95      | 0,61      |
| 2000    | 0,90      | 0,84      |

**Comentário:** é o algoritmo que mais se beneficia do paralelismo — trabalho
totalmente independente, sem regiões críticas. A eficiência **cresce** com o
tamanho (mais trabalho por thread dilui o overhead de criação das threads).

---

# 2. Selection Sort

## 2.1. Descrição do algoritmo

A cada passada *i*, o algoritmo **procura o menor elemento** do subvetor que
começa em *i* e o **troca** com a posição *i*. Após a passada *i*, as posições
0..i estão definitivamente ordenadas.

**Exemplo** com `[5, 2, 9, 1]`:

```
i=0: menor do trecho [5,2,9,1] é 1 -> troca com pos 0 -> [1, 2, 9, 5]
i=1: menor do trecho [2,9,5]    é 2 -> ja esta na pos  -> [1, 2, 9, 5]
i=2: menor do trecho [9,5]      é 5 -> troca com pos 2 -> [1, 2, 5, 9]
resultado: [1, 2, 5, 9]
```

## 2.2. Complexidade assintótica (pior caso)

A *i*-ésima passada faz *n − i − 1* comparações; somando, (n−1)+(n−2)+…+1:

> **O(n²)**

## 2.3. Implementação serial (trecho central)

```c
for (int i = 0; i < n - 1; i++) {
    int idx_min = i;
    for (int j = i + 1; j < n; j++)
        if (v[j] < v[idx_min]) idx_min = j;
    if (idx_min != i) {
        float tmp = v[i]; v[i] = v[idx_min]; v[idx_min] = tmp;
    }
}
```

(Código completo em `selection_serial.c`.)

## 2.4. Regiões críticas e dependências de dados

- **Dependência entre iterações do laço EXTERNO:** **sim, forte.** A passada *i*
  troca o conteúdo de `v[i]`, alterando o subvetor que a passada *i+1* vai
  examinar. Portanto **o laço externo NÃO pode ser paralelizado** (cada iteração
  depende do estado deixado pela anterior).
- **Laço INTERNO (busca do mínimo):** as comparações são independentes entre si;
  o que precisa de cuidado é a **atualização do mínimo global** (`idx_min`), que
  seria uma **região crítica** se várias threads escrevessem nela ao mesmo tempo.

## 2.5. Implementação paralela (OpenMP)

Paralelizamos o **laço interno**, usando uma **redução definida pelo usuário**
para encontrar o par (menor valor, índice) — o OpenMP não tem redução pronta que
devolva o **índice** do mínimo.

```c
typedef struct { float val; int idx; } MinIdx;

#pragma omp declare reduction(minloc : MinIdx :                          \
        omp_out = (omp_in.val < omp_out.val ||                           \
                  (omp_in.val == omp_out.val && omp_in.idx < omp_out.idx))\
                  ? omp_in : omp_out)                                     \
        initializer(omp_priv = {1e30f, -1})

for (int i = 0; i < n - 1; i++) {
    MinIdx m = { v[i], i };
    #pragma omp parallel for reduction(minloc:m) schedule(static)
    for (int j = i + 1; j < n; j++)
        if (v[j] < m.val) { m.val = v[j]; m.idx = j; }
    if (m.idx != i) {
        float tmp = v[i]; v[i] = v[m.idx]; v[m.idx] = tmp;
    }
}
```

**Estruturas usadas (e por quê):**

- `declare reduction(minloc:...)`: define uma operação de redução **customizada**.
  Cada thread mantém um mínimo local (cópia privada inicializada com um valor
  enorme `1e30f`); ao final, o OpenMP combina os mínimos locais. **Essa combinação
  final é a região crítica**, gerenciada automaticamente — não usamos `critical`
  explícito, o que é mais eficiente.
- `schedule(static)`: as comparações têm custo uniforme.

(Código completo em `selection_paralelo.c`.)

## 2.6. Comparação de desempenho — *EXEMPLO ILUSTRATIVO*

Tamanhos: 20.000, 40.000, 80.000 (O(n²) cresce rápido; ajuste para ~20–30 s).

**Tempo de execução (s)**

| Tamanho | 1 (serial) | 2 threads | 4 threads |
|--------:|-----------:|----------:|----------:|
| 20000   | 1,60       | 1,05      | 0,78      |
| 40000   | 6,40       | 4,00      | 2,70      |
| 80000   | 25,70      | 15,30     | 9,60      |

**Speedup**

| Tamanho | 2 threads | 4 threads |
|--------:|----------:|----------:|
| 20000   | 1,52      | 2,05      |
| 40000   | 1,60      | 2,37      |
| 80000   | 1,68      | 2,68      |

**Eficiência**

| Tamanho | 2 threads | 4 threads |
|--------:|----------:|----------:|
| 20000   | 0,76      | 0,51      |
| 40000   | 0,80      | 0,59      |
| 80000   | 0,84      | 0,67      |

**Comentário:** speedup razoável, mas abaixo do ideal. O motivo é que a região
paralela é **criada e destruída a cada passada** do laço externo (n−1 vezes), e o
trabalho de cada passada **encolhe** (a última passada compara só 1 elemento).
Mesmo assim, como o custo total é dominado pelas passadas iniciais (grandes), o
ganho é apreciável.

---

# 3. Heap Sort

## 3.1. Descrição do algoritmo

O Heap Sort usa um **heap binário máximo** representado em vetor (o filho de *i*
está em 2i+1 e 2i+2; o pai está em (i−1)/2). Tem duas fases:

1. **Construção do heap (build):** a partir do último nó interno (n/2−1) até a
   raiz, aplica-se a operação *desce* (sift-down) em cada nó, garantindo que todo
   pai seja ≥ seus filhos.
2. **Extração:** repetidamente troca-se a raiz (o **máximo**) com o último
   elemento do heap, reduz-se o heap em 1 e aplica-se *desce* na nova raiz. O
   máximo vai sendo "depositado" no final do vetor, deixando-o ordenado.

**Exemplo** com `[3, 1, 6, 5, 2, 4]`:

```
build -> max-heap: [6, 5, 4, 1, 2, 3]
extrai 6 -> [5, 3, 4, 1, 2 | 6]
extrai 5 -> [4, 3, 2, 1 | 5, 6]
... -> [1, 2, 3, 4, 5, 6]
```

## 3.2. Complexidade assintótica (pior caso)

Construção é O(n); a extração faz *n* operações *desce*, cada uma O(log n):

> **O(n log n)**

## 3.3. Implementação serial (trecho central)

```c
/* Fase 1: construcao */
for (int i = n / 2 - 1; i >= 0; i--)
    desce(v, i, n);
/* Fase 2: extracao */
for (int fim = n - 1; fim > 0; fim--) {
    troca(&v[0], &v[fim]);
    desce(v, 0, fim);
}
```

onde `desce(v, i, tam)` desce o elemento até restaurar a propriedade de heap.
(Código completo em `heapsort_serial.c`.)

## 3.4. Regiões críticas e dependências de dados

- **Fase de construção:** dentro de um **mesmo nível** da árvore, os nós têm
  **sub-árvores disjuntas**, logo as operações *desce* **não** têm dependência
  entre si — podem rodar em paralelo. **Entre níveis há dependência:** um pai só
  pode descer depois que as sub-árvores dos filhos já são heaps.
- **Fase de extração:** **dependência total entre iterações.** Cada `desce(0, fim)`
  opera sobre o estado deixado pela troca da iteração anterior. **Essa fase é
  intrinsecamente sequencial.**
- **Regiões críticas:** não usamos seções críticas explícitas; a sincronização
  entre níveis na construção é feita pela **barreira implícita** ao fim de cada
  `parallel for`.

## 3.5. Implementação paralela (OpenMP)

```c
int ultimo_interno = n / 2 - 1;
int nivel_max = (int)floor(log2((double)(ultimo_interno + 1)));
for (int nivel = nivel_max; nivel >= 0; nivel--) {
    int primeiro = (1 << nivel) - 1;
    int ultimo   = (1 << (nivel + 1)) - 2;
    if (ultimo > ultimo_interno) ultimo = ultimo_interno;

    /* nos do mesmo nivel = sub-arvores disjuntas = sem dependencia */
    #pragma omp parallel for schedule(dynamic, 64)
    for (int i = primeiro; i <= ultimo; i++)
        desce(v, i, n);
}
/* Fase de extracao permanece SEQUENCIAL */
for (int fim = n - 1; fim > 0; fim--) {
    troca(&v[0], &v[fim]);
    desce(v, 0, fim);
}
```

**Estruturas usadas (e por quê):**

- Paralelizamos **nível a nível**: para cada nível processamos seus nós em
  paralelo e a **barreira implícita** garante que o nível inferior terminou antes
  de subir (respeitando a dependência entre níveis).
- `schedule(dynamic, 64)`: o custo de *desce* varia (alguns nós descem mais que
  outros), então a distribuição dinâmica equilibra melhor a carga.
- A fase de extração **não** foi paralelizada por ser sequencial por natureza.

(Código completo em `heapsort_paralelo.c`.)

## 3.6. Comparação de desempenho — *EXEMPLO ILUSTRATIVO*

Tamanhos: 2.000.000, 4.000.000, 8.000.000.

**Tempo de execução (s)**

| Tamanho   | 1 (serial) | 2 threads | 4 threads |
|----------:|-----------:|----------:|----------:|
| 2.000.000 | 1,50       | 1,42      | 1,36      |
| 4.000.000 | 3,20       | 3,00      | 2,85      |
| 8.000.000 | 22,00      | 20,40     | 19,30     |

**Speedup**

| Tamanho   | 2 threads | 4 threads |
|----------:|----------:|----------:|
| 2.000.000 | 1,06      | 1,10      |
| 4.000.000 | 1,07      | 1,12      |
| 8.000.000 | 1,08      | 1,14      |

**Eficiência**

| Tamanho   | 2 threads | 4 threads |
|----------:|----------:|----------:|
| 2.000.000 | 0,53      | 0,28      |
| 4.000.000 | 0,53      | 0,28      |
| 8.000.000 | 0,54      | 0,29      |

**Comentário:** speedup baixíssimo. Pela **Lei de Amdahl**, só a fase de
construção (O(n)) é paralelizável, enquanto a fase dominante de extração
(O(n log n)) é **sequencial**. Como a fração sequencial é grande, o ganho máximo
teórico é pequeno, e a eficiência despenca com mais threads.

---

# 4. Histograma

## 4.1. Descrição do algoritmo

Um histograma conta **quantas vezes** cada valor inteiro aparece no conjunto.
Determina-se o menor (`vmin`) e o maior (`vmax`) valor; cria-se um vetor de
contadores de tamanho `vmax − vmin + 1`; para cada elemento, incrementa-se o
contador da posição correspondente (`v[i] − vmin`).

**Exemplo** com `[-2, 2, 3, 4, 5, -2, 2, 7]`:

```
valor: -2 -1  0  1  2  3  4  5  6  7
cont:   2  0  0  0  2  1  1  1  0  1
```

## 4.2. Complexidade assintótica (pior caso)

Uma passada pelos *n* elementos (a inicialização dos contadores é O(faixa)):

> **O(n)**

## 4.3. Implementação serial (trecho central)

```c
for (int i = 0; i < n; i++)
    hist[v[i] - vmin]++;
```

(Código completo em `histograma_serial.c`.)

## 4.4. Regiões críticas e dependências de dados

- **Região crítica:** o incremento `hist[bin]++` é uma **condição de corrida**:
  várias threads podem tentar incrementar o **mesmo** contador simultaneamente,
  e a operação `++` (ler-somar-escrever) não é atômica.
- **Dependência de dados entre iterações:** ocorre **quando dois elementos caem
  no mesmo bin** — as duas iterações disputam a mesma posição de memória.

## 4.5. Implementação paralela (OpenMP)

```c
#pragma omp parallel for reduction(+:hist[:faixa]) schedule(static)
for (int i = 0; i < n; i++)
    hist[v[i] - vmin]++;
```

**Estruturas usadas (e por quê):**

- `reduction(+:hist[:faixa])`: **redução de vetor** (OpenMP ≥ 4.5). Cada thread
  recebe uma **cópia privada** de todo o vetor `hist` e conta sem disputa; ao
  final, todas as cópias são somadas na global (essa soma final é a região
  crítica, tratada automaticamente). Escala muito melhor que usar
  `#pragma omp atomic` em cada incremento, que serializaria os acessos.
- `schedule(static)`: cada elemento custa o mesmo, então a divisão estática é a
  ideal.

(Código completo em `histograma_paralelo.c`.)

## 4.6. Comparação de desempenho — *EXEMPLO ILUSTRATIVO*

Tamanhos: 50, 100 e 200 milhões de inteiros (faixa de valores pequena, p. ex.
1000 bins).

**Tempo de execução (s)**

| Tamanho     | 1 (serial) | 2 threads | 4 threads |
|------------:|-----------:|----------:|----------:|
| 50.000.000  | 6,50       | 3,70      | 2,30      |
| 100.000.000 | 13,10      | 7,20      | 4,50      |
| 200.000.000 | 26,40      | 14,20     | 8,80      |

**Speedup**

| Tamanho     | 2 threads | 4 threads |
|------------:|----------:|----------:|
| 50.000.000  | 1,76      | 2,83      |
| 100.000.000 | 1,82      | 2,91      |
| 200.000.000 | 1,86      | 3,00      |

**Eficiência**

| Tamanho     | 2 threads | 4 threads |
|------------:|----------:|----------:|
| 50.000.000  | 0,88      | 0,71      |
| 100.000.000 | 0,91      | 0,73      |
| 200.000.000 | 0,93      | 0,75      |

**Comentário:** bom speedup. O custo extra é a alocação das cópias privadas e a
soma final (O(faixa × p)); por isso a eficiência cai um pouco com 4 threads.
Quando a **faixa** de valores é muito grande, as cópias privadas ficam caras e o
ganho diminui.

---

# 5. Gráficos (item 3.7)

Os gráficos de coluna pedidos são gerados automaticamente por `gera_graficos.py`
a partir do `resultados.csv`:

- **3.7.1 — Speedup × tamanho da entrada** (2 e 4 threads, no mesmo gráfico):
  arquivos `speedup_<algoritmo>.png`.
- **3.7.2 — Eficiência × tamanho da entrada** (2 e 4 threads, no mesmo gráfico):
  arquivos `eficiencia_<algoritmo>.png`.

A pasta `exemplo_graficos/` contém versões geradas com os **dados ilustrativos**
acima, só para você ver o formato. **Substitua pelos gráficos gerados com seus
próprios dados.**

---

# 6. Comparação entre os algoritmos de ordenação (item 3.8)

Comparando Selection Sort e Heap Sort para um mesmo vetor de entrada:

**1) Qual gera mais cache-misses?**
O **Heap Sort**. A operação *desce* salta entre o índice do pai *i* e os filhos
*2i+1* / *2i+2*. Para heaps grandes, esses índices ficam **muito distantes em
memória**, gerando acessos espalhados (saltos longos) com péssima localidade
espacial → muitos cache-misses. Já o Selection Sort percorre, na busca do mínimo,
um subvetor **contíguo e sequencialmente**, padrão amigável à cache (e ao
pré-buscador de hardware). Portanto, o **Heap Sort** tem mais cache-misses.

**2) Qual se beneficiou mais do paralelismo?**
O **Selection Sort**. Sua parte cara — a busca do mínimo, O(n) por passada — é
**totalmente paralelizável**, e domina o custo O(n²) total. No Heap Sort, apenas
a construção O(n) é paralelizável; a fase dominante de extração O(n log n) é
**sequencial**, então (pela Lei de Amdahl) o ganho é mínimo. Assim, o Selection
Sort obtém speedup bem maior.

**3) Qual é o fator de balanceamento das implementações paralelas?**
Define-se o **fator de balanceamento** como a razão entre a carga da thread mais
ocupada e a carga média (ideal = 1,0, perfeitamente balanceado):

- **Selection Sort:** em cada passada, `schedule(static)` divide o subvetor em
  blocos **iguais** e todas as comparações custam o mesmo → balanceamento ≈ **1,0
  (quase perfeito)** *dentro de cada passada*. A perda de eficiência vem do
  **overhead** de abrir/fechar a região paralela a cada passada e do encolhimento
  do trabalho, não de desbalanceamento.
- **Heap Sort:** dentro de um nível, as operações *desce* têm custo parecido e
  usamos `schedule(dynamic)` → bom balanceamento **dentro de cada nível**. Porém,
  o **número de nós por nível varia muito** (o nível mais baixo tem ~n/2 nós; os
  níveis de cima têm poucos nós, às vezes menos que o número de threads), então
  nos níveis superiores há **threads ociosas** → desbalanceamento global maior.
  Felizmente esses níveis são baratos.

Para **medir** o fator de balanceamento na prática, instrumente cada thread com
`omp_get_wtime()` em uma região `parallel`, registre o tempo de trabalho de cada
uma (`tid = omp_get_thread_num()`) e calcule `max(tempos) / média(tempos)`.

---

# 7. Como compilar e executar

```bash
make                         # compila os 8 programas
OMP_NUM_THREADS=4 ./matrixmult_paralelo 1000 m1.in m2.in
./run_benchmarks.sh          # coleta resultados.csv (rode em maquina 2+ nucleos)
python3 gera_graficos.py     # gera tabelas.txt e os PNGs de speedup/eficiencia
make clean                   # remove binarios e arquivos gerados
```

Validação de correção: para cada algoritmo, a saída do binário paralelo é
**idêntica** à do serial usando a mesma entrada (verificado com `diff`).
