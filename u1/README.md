# Implementação de algoritmo para cálculo de raiz quadrada em Assembly

## Algoritmo Utilizado e Restrições

Escolhemos o algoritmo dígito por dígito para realizar os cálculos das raízes. Em geral, considere que queremos calcular a raiz quadrada de um número S de 2 dígitos na base 10. Logo, precisamos encontrar os dígitos das dezenas X e das unidades Y tal que:

$S = (10X + Y)^{2} = 100X^{2} + 20XY + Y^{2}$

Inicialmente, dividimos os dígitos da direita para a esquerda em grupos de 2. Então calculamos um valor para X tal que $X^{2}$ seja menor ou igual que o primeiro grupo. Em seguida, subtraímos esse quadrado do grupo, isto é, subtraímos $100X^{2}$, restando apenas $S' = 20XY + Y^{2}$. Para o próximo digito da resposta, queremos um valor Y tal que 2X concatenado com Y e em seguida multiplicado por Y resulte em um valor menor ou igual ao bloco restante. No nosso exemplo, isso é o mesmo que calcular:
$$
(10(2X)+Y)Y = 20XY + Y^{2} = S'
$$

E tomamos a diferença novamente. Caso houvessem, continuaríamos o processos com os grupos de digitos restantes. Como o exemplo foi um quadrado perfeito, a resposta já seria o próprio número XY. O algoritmo segue até um determinado critério de parada caso não haja raízes exatas. 

Finalmente, quando não houver mais grupos de dígitos disponíveis, coloca-se um grupo 00 e continua o cálculo nas casas decimais da resposta. As demonstrações completas são dadas em [^1]

### Exemplo:
Queremos calcular a raiz de $17.943.696$. 

1. Dividimos em 2 grupos da direita para esquerda $17|94|36|96$
2. Para o primeiro, queremos um número $x$ tal que $x^{2} \leq 17$. Portanto, $X=4$ é o primeiro dígito da resposta;
3. Fazemos $17 - X^2 = 17 - 16 = 1$ e alinhamos o próximo grupo $94$ ao lado. Queremos agora um número $Y$ tal que $(80+Y)Y \leq 194$. Note que $(80+Y)$ equivale a tomar o dobro da resposta atual e concatenar o $Y$. Portanto, $Y = 2$;
4. Repetimos o processo: $194 - (80+Y)Y = 194 - 82 \times 2 = 194 - 164 = 30$. Alinhamos o próximo grupo $36$ ao lado. Queremos um número $Z$ tal que: $(840 + Z)Z \leq 3036$. Assim, $Z = 3$;
5. Fazendo por fim $3036 - (840 + Z)Z = 3036 - 2529 = 507$ e alinhando o último grupo ($96$) ao lado, pedimos um número M tal que $(8460 + M)M \leq 50796$. Com isso, encontramos M = 6 e resto 0. 

Mais exemplos são dados em [^1] e [^2].

Para facilitar o processo, utilizamos 2 registradores para representar a entrada e 2 registradores para representar a saída. Nesse sentido, a formatação da entrada é pressuposta como Q16.0 e a saída em Q8.6.

Com essas escolhas, o algoritmo é capaz de calcular raízes no intervalo $[0,  65.535]$, com respostas variando no intervalo [0, 255.984375].

## Erros Máximos
Conforme descrito no relatório, a implementação não sofre por conta de *overflow*. Com isso, o erro máximo relacionado é apenas o erro por conta da quantização. Se considerarmos apenas a resposta, o maior erro possível é igual a $2^{-6} = 0,015625$.

## Comparações com Outros Métodos
Em comparação com outros métodos, o algoritmo dígito por dígito é eficaz em se aproveitar das operações de deslocamento de bits entre os registradores. Diferentemente de demais métodos numéricos, não depende de estimativas iniciais. 

Além disso, sua iteração é ao longo do comprimento do número. Com isso, tem complexidade $\mathcal{O}(N)$, onde N é o tamanho do número binário.


## Implementação em Ponto Fixo

O diagrama da implementação em ponto fixo é dado a seguir:


## Observações
* Simulações realizadas utilizando a IDE MPLAB X;
* Compiler Toolchain utilizado: [avrasm2](https://aur.archlinux.org/packages/avrasm2).


## Referências
[^1]: [Digit by Digit Calculation (Wikipedia)](https://en.wikipedia.org/wiki/Square_root_algorithms#Digit-by-digit_calculation)

[^2]: [Calculating a SQRT digit-by-digit?](https://math.stackexchange.com/questions/512358/calculating-a-sqrt-digit-by-digit)


