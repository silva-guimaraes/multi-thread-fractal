
# Todo esse grosso foi traduzido de um projeto que fiz anteriormente:
# https://github.com/silva-guimaraes/mandelbrot-explorador/

# import concurrent.futures
import threading

lock = threading.Lock()

# não disponivel na versão mais recente do pypy (3.11)
# type RGB = tuple[int, int, int]

imagem_saida_altura, imagem_saida_largura, cores_canais = 1000, 1000, 3

imagem_saida_buffer_tamanho = imagem_saida_altura * imagem_saida_largura * cores_canais

imagem_saida_buffer: bytearray = bytearray(imagem_saida_buffer_tamanho)

def salvar_imagem(caminho: str) -> None:
    with open(caminho, 'w') as arquivo_saida:
        arquivo_saida.write(f'P3\n\n{imagem_saida_largura} {imagem_saida_altura}\n255\n')
        arquivo_saida.write(' '.join(map(str, imagem_saida_buffer)))
        arquivo_saida.write('\0') 

def caso_muito_distante(z: complex) -> bool:
    return z.imag*z.imag + z.real*z.real > 4

def f(z: complex, c: complex) -> complex:
    return z*z + c

def iterar(pixel_pos: complex, max: int) -> tuple[None, int] | tuple[complex, None]:
    z = complex(0, 0)
    c = pixel_pos
    for i in range(max):
        z = f(z, c)
        if caso_muito_distante(z):
            return None, i
    return z, None

def colorir_pixel(i, j, x): # x: RGB
    offset = i*imagem_saida_largura * cores_canais + j * cores_canais
    r, g, b = x

    imagem_saida_buffer[offset + 0] = max(min(r, 255), 0)
    imagem_saida_buffer[offset + 1] = max(min(g, 255), 0)
    imagem_saida_buffer[offset + 2] = max(min(b, 255), 0)

def cor_preto_branco(iteracoes_total: int, maximo_iteracoes: int): # -> RGB
    x = (iteracoes_total / maximo_iteracoes) * 255
    return (int(x), int(x), int(x))

def inverter_cor(cor): # -> RGB
    r, g, b = cor
    return (255 - r, 255 - g, 255 - b)

def cor_em_polinomio_bernstein(iteracoes_total: int, maximo_iteracoes: int): # -> RGB
    # https://en.wikipedia.org/wiki/Bernstein_polynomial
    normalizado: float = iteracoes_total / maximo_iteracoes
    return (
        int(9   * (1 - normalizado) * normalizado**3 * 255),
        int(15  * (1 - normalizado) * normalizado**2 * 255),
        int(8.5 * (1 - normalizado) * normalizado**1 * 255)
    )

# max
# centro_offset_horizontal: float = -0.630802899999805
# centro_offset_vertical:   float = -0.449996201018602

def gerar_buffer(
        maximo_iteracoes: int = 200,
        numero_threads: int = 12,
        zoom: float = 0,
        centro_offset: tuple[float, float] = (0, 0)
) -> bytearray:

    centro_offset_horizontal, centro_offset_vertical = centro_offset

    plano_complexo_largura: float = 4 / 2**zoom
    plano_complexo_altura:  float = 4 / 2**zoom

    plano_complexo_meia_altura:  float = plano_complexo_altura / 2
    plano_complexo_meia_largura: float = plano_complexo_largura / 2

    pixel_altura:  float = plano_complexo_altura / imagem_saida_altura
    pixel_largura: float = plano_complexo_largura / imagem_saida_largura

    class MyThread(threading.Thread):
        def __init__(self, linha_inicio: int, linha_final):
            super().__init__()
            self.linha_inicio: int = linha_inicio
            self.linha_final: int = linha_final

        def run(self) -> None:
            for i in range(self.linha_inicio, self.linha_final):
                for j in range(imagem_saida_largura):
                    ponto_real = pixel_largura * j - plano_complexo_meia_largura + centro_offset_horizontal
                    ponto_imag = pixel_altura * i - plano_complexo_meia_altura + centro_offset_vertical

                    pixel_pos = complex(ponto_real, ponto_imag)
                    
                    _, iteracoes_total = iterar(pixel_pos, maximo_iteracoes)

                    caso_divergencia_infinita: bool = iteracoes_total is not None

                    cor = (0, 0, 0) # RGB

                    if caso_divergencia_infinita:
                        cor = cor_em_polinomio_bernstein(iteracoes_total, maximo_iteracoes)

                    colorir_pixel(i, j, cor)

    divisao_linhas: int = int(imagem_saida_altura / numero_threads)
    threads: list[MyThread] = [
        MyThread(divisao_linhas*i, divisao_linhas*(i+1))
        for i in range(numero_threads)
    ]
    [t.start() for t in threads]
    [t.join() for t in threads]

    return imagem_saida_buffer


if __name__ == "__main__":
    gerar_buffer(zoom=7, maximo_iteracoes=700, centro_offset=(-0.630802899999805, -0.449996201018602))
    salvar_imagem('output.ppm')

