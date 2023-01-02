# Miasm
Assembler para o MICO X.

## Code format
'''asm
<label>:         tudo depois de ':' eh ignorado
  <inst> <args>; tudo depois de ';' eh ignorado
'''

* <label> -> deve ser um nome com ate 100 caracteres, letras minusculas apenas,
pode conter numeros e undescore, exceto no primeiro caractere.
* <inst> -> deve ter dois espacos entre o inicio da inst e o inicio da linha, deve ser alguma das 16 strings possiveis.
* <args> -> depende da <inst>.
    * nop -> sem argumentos, sem espaco entre nop e ';'. Ex: nop;
    * add -> "a, b, c;", onde a, b, c sao registradores; C <- A + B; Ex: add r3, rd, r0;
    * sub -> "a, b, c;", onde a, b, c sao registradores; C <- A - B; Ex: sub r3, rd, r0;
    * and -> "a, b, c;", onde a, b, c sao registradores; C <- A & B; Ex: and r3, rd, r0;
    * or -> "a, b, c;", onde a, b, c sao registradores; C <- A | B; Ex: or r3, rd, r0;
    * xor -> "a, b, c;", onde a, b, c sao registradores; C <- A ^ B; Ex: xor r3, rd, r0;
    * not -> "a, b;", onde a, b sao registradores; B <- !A; Ex: not r3, r0;
    * ori -> "a, b, C;", onde a, b sao registradores e C e uma constante positiva; B <- A | C; Ex: ori r3, rd, 12;
    * xori -> "a, b, C;", onde a, b sao registradores e C e uma constante positiva; B <- A ^ C; Ex: xori r3, rd, 12;
    * addi -> "a, b, C;", onde a, b sao registradores e C e uma constante positiva; B <- A + C; Ex: addi r3, rd, 12;
    * dsp -> "a;", onde a e um registrador; Mostra o vaor em a; Ex: dsp rd;
    * jmp -> "E;", onde E e uma constante negativa ou positiva; IP <- IP + E Ex: jmp -4;
    * jz -> "a, b, E; onde a, e sao registradores e E e uma constante negativa ou positiva;
        IP <- IP + (A == B ? E : 1); Ex: jz r2 r3 10;
    * halt -> sem argumentos, sem espaco entre halt e ';'; Ex: halt;

OBS: Os padroes devem ser seguidos estritamente como na descricao, por exemplo:
para o padrao "  addi a, b, C;", se for escrito da forma "  addi a,  b,c;" nao vai funcionar.

#Exemplo
Mostra os 12 primeiros numeros da sequencia de fibonacci.

'''asm
  addi r0, rf, 1;  
  addi r0, r1, 0;   a = r1
  dsp r1;
  addi r0, r2, 1;   b = r2
  dsp r2;
  addi r0, r3, 10;  n = r3 n = 10


loop_1:
  add r1, r2, r4;   fib = r4, fib = a + b
  dsp r4;
  add r0, r2, r1;   a = b
  add r0, r4, r2;   b = fib
  sub r3, rf, r3;   n = n - 1
  jz r0, r3, fim;
  jmp loop_1;

  
fim:
  halt;
'''
# Compiling
gcc -Wall -std=c11 miasm.c -o miasm

# Runing
./miasm input.mico > output.hex

Abra o logisim e na memoria do programa edite ela e abra o arquivo output.hex, escolha o formato hexadecimal e salve.

