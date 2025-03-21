
// Todo esse grosso foi traduzido de um projeto que fiz anteriormente:
// https://github.com/silva-guimaraes/mandelbrot-explorador/


#include <complex.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>


#ifdef USAR_SDL_
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_timer.h>
#endif /* ifdef benchmark */


#define max(a, b) a > b ? a : b

typedef struct {
    uint8_t *buffer;
    int altura, largura, canais;
} imagem;

#define tamanho_bytes(x) x.altura *x.largura *x.canais

void salvar_imagem(imagem imagem) {

    FILE *arquivo_saida = fopen("output.ppm", "w");

    fprintf(arquivo_saida, "P3\n\n%d %d\n255\n", imagem.largura, imagem.altura);
    for (int i = 0; i < tamanho_bytes(imagem); i++) {
        fprintf(arquivo_saida, "%d ", imagem.buffer[i]);
    }
    fflush(arquivo_saida);
    fclose(arquivo_saida);
}

int iterar(double complex c, int maximo_iteracoes) {
    double complex z = 0 + 0i;
    for (int i = 0; i < maximo_iteracoes; i++) {
        z = z * z + c;
        if (pow(cimag(z), 2) + pow(creal(z), 2) > 4)
            return i;
    }
    return -1;
}

typedef struct {
    int r, g, b;
} cor;

cor cor_polinomio_bernstein(int iteracoes_total, int maximo_iteracoes) {
    double normalizado = (double)iteracoes_total / maximo_iteracoes;
    return (cor){
        .r = 9 * (1 - normalizado) * normalizado * normalizado * normalizado * 255,
        .g = 15 * (1 - normalizado) * normalizado * normalizado * 255,
        .b = 8.5 * (1 - normalizado) * normalizado * 255,

    };
}

cor cor_preto_branco(int iteracoes_total, int maximo_iteracoes) {
    double c = (double)iteracoes_total / maximo_iteracoes * 255;
    return (cor) { c,c,c };
}

typedef struct {
    imagem imagem;
    int i_inicio;
    int i_fim;
    double centro_offset_horizontal;
    double centro_offset_vertical;
    int zoom;
    int maximo_iteracoes;
} gerar_args;

void *gerar_mandelbrot(void *x) {

    int err = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    if (err) {
        fprintf(stderr, "falha na configuração de thread! %s\n", strerror(err));
        return NULL;
    }
    // funções em pthread_create precisam ser genéricas. a assinatura
    // da função pede que os parâmetros da função incluam apenas um único ponteiro void.
    //
    // aqui passamos um ponteiro de uma struct com vários outros elementos 
    // para atender as necessidades dessa função.
    gerar_args args = *(gerar_args *)x;
    imagem imagem = args.imagem;
    // faixa de linhas que serão geradas por essa thread. de 0 até altura máxima
    // da imagem
    int i_inicio = args.i_inicio;
    int i_fim = args.i_fim;
    // nível de zoom
    int zoom = args.zoom;
    //  isso movimenta a câmera
    double centro_offset_horizontal = args.centro_offset_horizontal;
    double centro_offset_vertical = args.centro_offset_vertical;
    // número máximo de iterações do algoritmo
    // qualquer iteracao que atinja esse número é considerada convergente. 
    // qualquer iteração que escape de certa distancia antes desse número
    // é considerado divergente.
    int maximo_iteracoes = args.maximo_iteracoes;

    // desafio dessa função: mapear coordenadas de pixels da imagem
    // para coordenadas do plano complex.

    //  dado que sempre iremos representar todos os pixels como uma
    //  certa faixa do plano complexo, diminuir sua dimensão se traduz diretamente
    //  em ampliar o conjunto exibido na tela.
    double plano_complexo_largura = 4 / pow(2, zoom);
    double plano_complexo_altura = 4 / pow(2, zoom);

    // queremos que o centro da imagem seja a coordenada (0, 0). subitrair
    // esses dois parâmetros pela coordenado do pixel faz com que tenhamos
    // esse posicionamento.
    double plano_complexo_meia_largura = plano_complexo_largura / 2;
    double plano_complexo_meia_altura = plano_complexo_altura / 2;

    // diz a distância entre cada pixel para que seja posivel traduzir
    // as coordenadas
    double pixel_altura = plano_complexo_altura / imagem.altura;
    double pixel_largura = plano_complexo_largura / imagem.largura;

    // itera por cada pixel da imagem
    for (int i = i_inicio; i < i_fim; i++) {
        for (int j = 0; j < imagem.largura; j++) {

            double x = j * pixel_largura - plano_complexo_meia_largura +
                centro_offset_horizontal;
            double y = i * pixel_altura - plano_complexo_meia_altura +
                centro_offset_vertical;

            int iteracoes = iterar(x + y * I, maximo_iteracoes);

            cor c = cor_polinomio_bernstein(iteracoes, maximo_iteracoes);

            if (iteracoes < 0)
                c = (cor) { 0 };

            // o conjunto de bytes que representa a imagem é uma array contigua.
            // isso transforma as coordenadas dos pixels para um único indice
            // no framebuffer.
            int offset = i * imagem.largura * imagem.canais + j * imagem.canais;

            // todas as threads estão operando no mesmo framebuffer quando essa operação ocorre.
            // o resultado é apenas coerente pois nenhuma linha da imagem é compartilhada por mais de uma thread.
            imagem.buffer[offset + 0] = c.r;
            imagem.buffer[offset + 1] = c.g;
            imagem.buffer[offset + 2] = c.b;
        }
    }
    // necessário para o pthread_create.
    return NULL;
}

void multi_thread(imagem imagem, pthread_t threads[], int numero_threads, int i_inicio, int i_fim,
                  double centro_offset_horizontal,
                  double centro_offset_vertical, int zoom,
                  int maximo_iteracoes) {
    int divisao_linhas = imagem.altura / numero_threads;
    int err;
    gerar_args argumentos[100];

    for (int i = 0; i < numero_threads; i++) {
        gerar_args args = {
            .imagem = imagem,
            .zoom = zoom,
            .maximo_iteracoes = maximo_iteracoes,
            .i_inicio = divisao_linhas * i,
            .i_fim = divisao_linhas * (i + 1),
            .centro_offset_vertical = centro_offset_vertical,
            .centro_offset_horizontal = centro_offset_horizontal,
        };
        if (i + 1 == numero_threads) {
            args.i_fim = imagem.altura;
        }
        argumentos[i] = args;
    }

    for (int i = 0; i < numero_threads; i++) {
        err = pthread_create(threads + i, NULL, &gerar_mandelbrot, argumentos + i);
        if (err)
            fprintf(stderr, "falha ao criar thread! %s\n", strerror(err));
    }

    // aguarda para que todas as threads terminem.
    for (int i = 0; i < numero_threads; i++)
        pthread_join(threads[i], NULL);
    // enquanto isso estiver esperando o SDL não estará recenbendo atualizações.
    // o programa fica completamente irresponsivo como consequencia e
    // é uma pessima experiencia.
}

void single_thread(imagem imagem, int i_inicio, int i_fim,
                   double centro_offset_horizontal,
                   double centro_offset_vertical, int zoom,
                   int maximo_iteracoes) {
    gerar_args args = {
        .imagem = imagem,
        .zoom = zoom,
        .maximo_iteracoes = maximo_iteracoes,
        .i_inicio = i_inicio,
        .i_fim = i_fim,
        .centro_offset_vertical = centro_offset_vertical,
        .centro_offset_horizontal = centro_offset_horizontal,
    };

    gerar_mandelbrot(&args);
}

#define cores_canais 3

#ifndef USAR_SDL_
#define imagem_altura 512
#define imagem_largura 512
#else
#define imagem_altura 256
#define imagem_largura 256
#endif

void benchmark(imagem imagem) {
    pthread_t threads[100];

    double centro_offset_horizontal = -0.630802899999805,
    centro_offset_vertical = -0.449996201018602;

    for (int i = 1; i <= 24; i++) {
        double soma = 0;
        int numero_iteracoes = 10;
        for (int j = 0; j < numero_iteracoes; j++) {

            struct timespec start, end;
            clock_gettime(CLOCK_MONOTONIC, &start);

            multi_thread(
                imagem, threads, i, 0, imagem.altura,
                centro_offset_horizontal, centro_offset_vertical, 2, 200
            );

            clock_gettime(CLOCK_MONOTONIC, &end);

            double elapsed =
                (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
            soma += elapsed;
        }
        printf("número de threads: %d, tempo de execução médio: %.6f segundos\n", i,
               soma / numero_iteracoes);
    }
}

int main(void) {

    uint8_t buffer[imagem_largura * imagem_altura * cores_canais] = {0};

    imagem imagem = {
        .altura = imagem_altura,
        .largura = imagem_largura,
        .canais = cores_canais,
        .buffer = buffer,
    };

#ifndef USAR_SDL_

    benchmark(imagem);
    salvar_imagem(imagem);

#else
    int zoom = 0;
    double centro_offset_horizontal = -0.630802899999805,
    centro_offset_vertical = -0.449996201018602;

    pthread_t threads[100];
    int numero_threads = 12;
    printf("numero de threads: %d\n", numero_threads);

    multi_thread(imagem, threads, numero_threads, 0, imagem.altura, centro_offset_horizontal,
                 centro_offset_vertical, zoom, 200);


    // explorador interativo do conjunto utilizando a biblioteca gráfica SDL.
    // requer SDL2 como dependeria: https://www.libsdl.org/

    // cria a tela principal do programa
    SDL_Window *window =
        SDL_CreateWindow("Explorador", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 1024, 1024, SDL_WINDOW_SHOWN);

    if (!window) {
        printf("SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // SDL trabalha com o conceito de 'textures' e 'renderers'.
    // texturas são estruturas arbtrarias capazes de armazenas qualquer
    // coisa que se assemelhe a uma imagem.
    // 'renderizadores' são análogos a um canvas de uma pintura. é para lá
    // que todas as texturas são aplicadas antes de serem exibidas ao usuário.
    SDL_Renderer *renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             imagem.largura, imagem.altura);

    if (!texture) {
        printf("SDL_CreateTexture: %s\n", SDL_GetError());
        return 1;
    }

    // insere buffer com o conjunto renderizado à textura.
    SDL_UpdateTexture(texture, NULL, buffer, imagem.largura * imagem.canais);

    int rodando = 1;
    while (rodando) {
        SDL_Event event;

        int tempo_inicio = SDL_GetTicks();

        // controles:
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                rodando = 0;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                    case SDLK_ESCAPE:
                        rodando = 0;
                        break;
                    case SDLK_w:
                        zoom++;
                        break;
                    case SDLK_s:
                        zoom = max(zoom-1, 0);
                        break;
                    case SDLK_j:
                        centro_offset_vertical += 0.1;
                        break;
                    case SDLK_k:
                        centro_offset_vertical -= 0.1;
                        break;
                    case SDLK_1:
                        printf("numero de threads: %d\n", numero_threads);
                        numero_threads = max(numero_threads-1, 1);
                        break;
                    case SDLK_2:
                        printf("numero de threads: %d\n", numero_threads);
                        numero_threads += 1;
                        break;
                    case SDLK_l:
                        centro_offset_horizontal += 0.1;
                        break;
                    case SDLK_h:
                        centro_offset_horizontal -= 0.1;
                        break;
                }

                // dados foram modificados, re-renderizar conjunto agora:
                multi_thread(imagem, threads, numero_threads, 0, imagem.altura, centro_offset_horizontal,
                             centro_offset_vertical, zoom, 900);
                SDL_UpdateTexture(texture, NULL, buffer, imagem.largura * imagem.canais);

                int tempo_fim = SDL_GetTicks();
                int tempo_gasto_ms = tempo_fim - tempo_inicio;
                float tempo_gasto_s = (float) tempo_gasto_ms / 1000;

                printf("tempo renderizando: %.2f segundos (%.0f FPS)\n", tempo_gasto_s, 1 / tempo_gasto_s);
                fflush(stdout);
            }
        }

        // renderização:

        // infezlimente não é possível atualizar a textura enquanto threads estão
        // ativamente modificando o buffer.
        // SDL_UpdateTexture(texture, NULL, buffer, imagem.largura * imagem.canais);

        // limpa a tela
        SDL_RenderClear(renderer);
        // copia textura com o nosso conjunto para o framebuffer principal
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        // atualiza o framebuffer
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif /* ifndef USAR_SDL_ */
    return 0;
}
