
type RGB = tuple[int, int, int]

imagem_saida_altura, imagem_saida_largura, cores_canais = 500, 500, 3

imagem_saida_buffer_tamanho = imagem_saida_altura * imagem_saida_largura * cores_canais

imagem_saida_buffer: bytearray = bytearray(imagem_saida_buffer_tamanho)

def salvar_imagem():
    with open('output.ppm', 'w') as arquivo_saida:
        arquivo_saida.write(f'P3\n\n{imagem_saida_largura} {imagem_saida_altura}\n255\n')
        arquivo_saida.write(' '.join(map(str, imagem_saida_buffer)))
        arquivo_saida.write('\0') 

def iterar(pixel_pos: complex, max: int) -> tuple[None, int] | tuple[complex, None]:
    z = complex(0, 0)
    c = pixel_pos
    for i in range(max):
        z = z**2 + c
        if (z.imag**2 + z.real**2) > 4:
            return None, i
    return z, None

def clamp(x: int) -> int:
    return min(x, 255)

def colorir_pixel(i, j, x: RGB):
    offset = i*imagem_saida_largura * cores_canais + j * cores_canais
    r, g, b = x

    imagem_saida_buffer[offset + 0] = clamp(r)
    imagem_saida_buffer[offset + 1] = clamp(g)
    imagem_saida_buffer[offset + 2] = clamp(b)



def gerar_buffer():
    plano_complexo_largura: float = 4
    plano_complexo_altura:  float = 4

    centro_offset_horizontal: float = 0
    centro_offset_vertical:   float = 0

    plano_complexo_meia_altura:  float = plano_complexo_altura / 2
    plano_complexo_meia_largura: float = plano_complexo_largura / 2

    pixel_altura:  float = plano_complexo_altura / imagem_saida_altura
    pixel_largura: float = plano_complexo_largura / imagem_saida_largura

    maximo_iteracoes: int = 100

    for i in range(imagem_saida_altura):
        for j in range(imagem_saida_largura):

            cor: RGB

            ponto_real = pixel_largura * j - plano_complexo_meia_largura + centro_offset_horizontal
            ponto_imag = pixel_altura * i - plano_complexo_meia_altura + centro_offset_vertical

            pixel_pos = complex(ponto_real, ponto_imag)
            
            _, iteracoes_total = iterar(pixel_pos, maximo_iteracoes)

            caso_convergencia_finita: bool = iteracoes_total is None

            if caso_convergencia_finita:
                cor = (0, 0, 0)
            else:
                normalizado: float = iteracoes_total / maximo_iteracoes
                # https://en.wikipedia.org/wiki/Bernstein_polynomial
                cor = (
                    int(9   * (1 - normalizado) * normalizado**3 * 255),
                    int(15  * (1 - normalizado) * normalizado**2 * 255),
                    int(8.5 * (1 - normalizado) * normalizado**1 * 255)
                )

            colorir_pixel(i, j, cor)


gerar_buffer()
salvar_imagem()
