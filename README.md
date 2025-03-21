
### fractal multi thread

## Conjunto de Mandelbrot

O Conjunto de Mandelbrot é uma imagem famosa feita a partir de uma fórmula matemática simples:

Fórmula:
$$ z = z^2 + c $$

- Começamos com z = 0 e um número complexo c (um ponto da tela).
- Aplicamos a fórmula várias vezes.
- Se o valor de z fugir para o infinito, o ponto c não faz parte do conjunto.
- Se z ficar limitado, o ponto c faz parte.
- Como a nossa abordagem do algoritmo desenha funciona:
- O programa divide a tela em muitos pixels.
- Cada pixel vira um número complexo c.
- Roda a fórmula para cada c um certo número de vezes.
- Se z explodir (normalmente quando a distancia do centro é maior do que 2), pinta o pixel de uma cor (fora do conjunto).
- Se não explodir, pinta o pixel de preto (dentro do conjunto).
- Faz isso para todos os pixels.
- No final, o nosso programa monta a famosa imagem com espirais e formas infinitas.

## Rodando

Compilação do programa C para benchmark:
```sh
gcc -lm -lpthread main.c -o main
```
Ao final da execução, o programa terá salvo uma imagem no seu diretório da mesma gerada pelo benchmark.

Compilação do programa C utilizando a [biblioteca gráfica SDL2](https://www.libsdl.org/) para uma visualização interativa do conjunto:
```sh
gcc -DUSAR_SDL_ -lm -lpthread $(pkg-config --libs sdl2) main.c -o main
```
Use as teclas `h`, `j`, `k`, `l` para mover-se para esquerda, para baixo, para cima e para direita respectivamente. Use as teclas `w` para ampliar o zoom e `s` para diminuir o zoom.

## Benchmark
Resultados obtidos com o benchmark da versão em C:
```txt
número de threads: 1, tempo de execução médio: 1.198899 segundos
número de threads: 2, tempo de execução médio: 1.017455 segundos
número de threads: 3, tempo de execução médio: 0.985460 segundos
número de threads: 4, tempo de execução médio: 0.727330 segundos
número de threads: 5, tempo de execução médio: 0.711200 segundos
número de threads: 6, tempo de execução médio: 0.632278 segundos
número de threads: 7, tempo de execução médio: 0.646971 segundos
número de threads: 8, tempo de execução médio: 0.586908 segundos
número de threads: 9, tempo de execução médio: 0.557743 segundos
número de threads: 10, tempo de execução médio: 0.507573 segundos
número de threads: 11, tempo de execução médio: 0.467412 segundos
número de threads: 12, tempo de execução médio: 0.452030 segundos
número de threads: 13, tempo de execução médio: 0.436142 segundos
número de threads: 14, tempo de execução médio: 0.430776 segundos
número de threads: 15, tempo de execução médio: 0.406652 segundos
número de threads: 16, tempo de execução médio: 0.396492 segundos
número de threads: 17, tempo de execução médio: 0.407281 segundos
número de threads: 18, tempo de execução médio: 0.433568 segundos
número de threads: 19, tempo de execução médio: 0.447050 segundos
número de threads: 20, tempo de execução médio: 0.405458 segundos
número de threads: 21, tempo de execução médio: 0.173068 segundos
número de threads: 22, tempo de execução médio: 0.157213 segundos
número de threads: 23, tempo de execução médio: 0.164733 segundos
número de threads: 24, tempo de execução médio: 0.162460 segundos
```
Parâmetros:
- Altura da imagem: 512 pixels
- Largura da imagem: 512 pixels
- Profundidade de cores: 24bits (3 bytes)
- Posição horizontal: -0.630802899999805
- Posição Vertical: -0.449996201018602
- Número de amostragem por cada teste: 10 iterações

É necessário alterar e recompilar os parâmetros no código fonte.

Especificações da máquina:
- CPU: 12th Gen Intel i5-1235U (12) @ 4.400GHz
- Número de núcleos: 12
- Número de threads por núcleo: 2 (total 24)
- GPU: Intel Alder Lake-UP3 GT2
- Memória: 15679MiB
