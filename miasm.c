#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define EXIT(n, f) exit(n); fclose(f)

size_t open_file(char *file_path, char **buffer){
    FILE *f = fopen(file_path, "rb");
    if (f == NULL){
        fprintf(stderr, "Could not open %s: %s\n", file_path, strerror(errno));
        exit(1);
    }

    if (fseek(f, 0, SEEK_END)){
        fprintf(stderr, "Erro seking in file %s: %s\n", file_path, strerror(errno));
        EXIT(1, f);
    }

    size_t file_size = ftell(f);
    if (file_size == -1){
        fprintf(stderr, "Erro getting size of %s: %s\n", file_path, strerror(errno));
        EXIT(1, f);
    }

    rewind(f);

    if ((*buffer = malloc(sizeof(char) * file_size + 1)) == NULL){
        fprintf(stderr, "Could not alloc memory buffer for input file\n");
        EXIT(1, f);
    }
    
    if (fread(*buffer, sizeof(char), file_size, f) != file_size){
        fprintf(stderr, "Could no read from %s: %s\n", file_path, strerror(errno));
        free(*buffer);
        EXIT(1, f);
    }
    (*buffer)[file_size] = '\0';

    fclose(f);
    return file_size;
}

enum Inst {
    NOP = 0,
    ADD,
    SUB,
    AND,
    OR,
    XOR,
    NOT,
    SLL,
    SLR,
    ORI,
    XORI,
    ADDI,
    DSP,
    JMP,
    JZ,
    HALT
};

static char *mennemonics[] = {
    "nop",
    "add",
    "sub",
    "and",
    "or",
    "xor",
    "not",
    "sll",
    "slr",
    "ori",
    "xori",
    "addi",
    "dsp",
    "jmp",
    "jz",
    "halt"
};
#define MAX_MENNEMONIC_SIZE 4
#define MAX_ARGS 4
#define ARR_SIZE(xs) (sizeof(xs)/sizeof(xs[0]))

typedef struct inst {
    enum Inst op;
    size_t addr;
    int args[MAX_ARGS];
} inst_t;

#define MAX_LABEL_SZ 100
#define MAX_LABELS   100

typedef struct label {
    char name[MAX_LABEL_SZ];
    int addr;
} label_t;

int compare(char *str1, char *str2){
    int i = 0;
    while(
            str1[i] != '\0' &&
            str2[i] != '\0' &&
            (str1[i] == str2[i])
         ) i++;

    return str1[i] == str2[i];
}

int is_char(char c){
    return ('a' <= c && c <= 'z');
}

int is_digit(char c){
    return ('0' <= c && c <= '9');
}

int is_hex(char c){
    return (is_digit(c) ||  ('a' <= c && c <= 'f'));
}

int is_space(char c){
    return (c == ' ' || c == '\n' || c == '\t');
}

int digit(char c){
    return c - '0';
}

int hex(char c){
    return (int) (c <= '9'? c - '0': (c - 'a') + 10);
}

char *get_end_of_line(char *buffer, int *is_empty){
    *is_empty = 1;
    while (*buffer != '\0' && *buffer != '\n'){
        *is_empty = *is_empty ? is_space(*buffer) : *is_empty;
        buffer++;
    }

    return buffer;
}

// Retorn 0 se for fim do arquivo
int advance_empty_lines(char **begin, char **end){
    int is_empty;
    *end = get_end_of_line(*begin, &is_empty);
    while (is_empty && (**end != '\0')){
        *begin = *end + 1;
        *end = get_end_of_line(*begin, &is_empty);
    }

    return **end == '\0';
}

int consume_str(char **begin, char *end, const char *str){
    while (*begin < end &&
           *str != '\0' &&
           **begin == *str)
    {
        (*begin)++;
        str++;
    }

    if (*str != '\0')
        return 0;
    return 1;
}

int get_inst(char *begin, char *end, enum Inst *op, char **prox){
    int i = 0;
    char mennemonic[MAX_MENNEMONIC_SIZE + 1] = {0};

    *prox = begin;
    if (!consume_str(prox, end, "  ")){
        fprintf(stderr, "Instructions must be 2 spaces from the begin of line.\nEx:\n  <op> <args>;\n");
        return 0;
    }

    while (is_char(**prox) && i < MAX_MENNEMONIC_SIZE)
        mennemonic[i++] = *((*prox)++);

    if (is_char(**prox)){
        fprintf(stderr, "Mennemonic to long: \"%s\"\n", mennemonic);
        return 0;
    }

    *op = -1;
    for (i = 0; i < ARR_SIZE(mennemonics); i++)
        if (compare(mennemonic, mennemonics[i])){
            *op = i;
            return 1;
        }
    fprintf(stderr, "Mennemonico \"%s\" desconhecido\n", mennemonic);
    return 0;
}

int get_label(char *begin, char *end, label_t *label, char **prox){
    int i = 0;
    *prox = begin;

    if (!is_char(**prox))
        return 0;

    while ((is_char(**prox) || **prox == '_' || is_digit(**prox))&& i < MAX_LABEL_SZ)
        label->name[i++] = *((*prox)++);


    if (is_char(**prox))
        return 0;

    return 1;
}

int get_reg(char *begin, char *end, int *reg, char **prox){
    *prox = begin;

    if (!is_char(**prox) || !consume_str(prox, end, "r"))
        return 0;

    if (!is_hex(**prox))
        return 0;
    
    *reg = hex(**prox);
    (*prox)++;
    return 1;
}

int get_const(char *begin, int *cst, char **prox, int neg){
    int mult = 1;
    if (neg && *begin == '-'){
        begin++;
        mult = -1;
    }
    *prox = begin;
    *cst = 0;

    if (!is_digit(**prox))
        return 0;
    while (is_digit(**prox)){
        *cst = *cst * 10 + digit(**prox);
        (*prox)++;
    }
    *cst *= mult;

    if (*cst > 0xffff)
        return 0;

    return 1;
}
int label_addr(label_t labels[MAX_LABELS], int labels_sz, label_t *l, int *addr){
    for (int i =0; i < labels_sz; i++){
        if (compare(labels[i].name, l->name)){
            *addr = labels[i].addr - l->addr;
            return 1;
        }
    }
    return 0;
}

int precompute_labels(char *begin, char *end, label_t labels[MAX_LABELS], int *labels_sz){
    int line_addr = 0;
    char *prox;
    while (*begin != '\0'){
        if (is_char(*begin)){
            if (!get_label(begin, end, &labels[(*labels_sz)++], &prox))
                return 0;
            labels[*labels_sz - 1].addr = line_addr;
            begin = prox;
            if (*begin != ':'){
                fprintf(stderr, "Label sem fechamento: %s\n", labels[*labels_sz - 1].name);
                fprintf(stderr, "Deve ser da seguinte forma:\n<label>:\n");
                return 0;
            }
        }else
            line_addr++;
        begin = end + 1;
        advance_empty_lines(&begin, &end);
    }

    return 1;
}

// $r = register
// $c[-] = const
// $l = label
int get_pattern_args(const char *pattern,
                     char *begin,
                     char *end,
                     int args[MAX_ARGS],
                     label_t labels[MAX_LABELS],
                     int labels_sz,
                     int addr)
{
    char *prox;
    int args_sz = 0;
    while (begin < end &&
           *pattern != '\0' &&
           args_sz < MAX_ARGS)
    {
        if (*pattern == '$'){
            pattern++;
            switch (*pattern){
                case 'r':{
                    pattern++;
                    if (!get_reg(begin, end, &args[args_sz], &prox)){
                        fprintf(stderr, "Registrador invalido: %.*s\n", (int)(end - begin), begin);
                        return 0;
                    }
                    begin = prox;
                }; break;
                case 'c':{
                    int neg = 0;
                    pattern++;
                    if (*pattern == '-'){
                        pattern++;
                        neg = 1;
                    }
                    if (!get_const(begin, &args[args_sz], &prox, neg)){
                        fprintf(stderr, "Constante invalida: %.*s\n", (int)(end - begin), begin);
                        return 0;
                    }
                    begin = prox;
                }; break;
                case 'l':{
                    pattern++;
                    if (!is_char(*begin)){
                        if (!get_const(begin, &args[args_sz], &prox, 1)){
                            fprintf(stderr, "Constante invalida: %.*s\n", (int)(end - begin), begin);
                            return 0;
                        }
                    }else{
                        label_t l = {0};
                        l.addr = addr;
                        if (!get_label(begin, end, &l, &prox)){
                            fprintf(stderr, "Label invalida: %.*s\n", (int)(end - begin), begin);
                            return 0;
                        }
                        if (!label_addr(labels, labels_sz, &l, &args[args_sz])){
                            fprintf(stderr, "Label inexistente: %s\n", l.name);
                            return 0;
                        }
                    }
                    begin = prox;
                }; break;
                default:{
                    fprintf(stderr, "Unkonw patter match [%c]\n", *pattern);
                    return 0;
                }
            }
            args_sz++;
        }else if (*begin != *pattern){
            fprintf(stderr, "Erro de formatacao o argumentos\n");
            return 0;
        }
        begin++;
        pattern++;
    }

    if (args_sz >= MAX_ARGS){
        fprintf(stderr, "Argumentos demais no pattern da instrucao: %s\n", pattern);
        return 0;
    }

    if (*pattern != '\0'){
        fprintf(stderr, "Erro de formatacao o argumentos\n");
        return 0;
    }

    return 1;
}

int main(int argc, char **argv){
    // TODO: comentarios
    // TODO: criar macros
    // TODO: melhorar o algoritmo de pattern match
    // TODO: Saida em binario
    // TODO: aruqivo de output
    if (argc < 2){
        fprintf(stderr, "Usage: %s <inputfile.mico>\n", argv[0]);
        exit(1);
    }
    char *file_path = argv[1];
    char *buffer = NULL;
    size_t buf_sz = open_file(file_path, &buffer);

    char *begin = buffer;
    char *end;
    advance_empty_lines(&begin, &end);

    char *prox;

    label_t labels[MAX_LABELS] = {0};
    int labels_sz = 0;

    inst_t *insts = malloc(sizeof(inst_t) * buf_sz / 4);
    if (insts == NULL){
        free(buffer);
        fprintf(stderr, "Nao foi possivela alocar mais memoria\n");
        return 1;
    }
    int insts_sz = 0;

    if (!precompute_labels(begin, end, labels, &labels_sz))
        return 1;

    while (*begin != '\0'){
        insts[insts_sz].addr = insts_sz;
        insts[insts_sz].args[0] = 0;
        insts[insts_sz].args[1] = 0;
        insts[insts_sz].args[2] = 0;
        insts[insts_sz].args[3] = 0;
        if (is_char(*begin)){
            if (!get_pattern_args("$l:", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz))
                return 1;
            begin = end + 1;
            advance_empty_lines(&begin, &end);
            continue;
        }

        if (!get_inst(begin, end, &insts[insts_sz].op, &prox))
            return 1;
        begin = prox;

        switch (insts[insts_sz].op){
            case NOP:{
                         if (!get_pattern_args(";", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao NOP no tem argumentos, ivalido: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                     }; break;
            case ADD:{
                         if (!get_pattern_args(" $r, $r, $r;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao AND esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                     }; break;
            case SUB:{
                         if (!get_pattern_args(" $r, $r, $r;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao SUB esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                     }; break;
            case AND:{
                         if (!get_pattern_args(" $r, $r, $r;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao AND esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                     }; break;
            case OR:{
                         if (!get_pattern_args(" $r, $r, $r;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao OR esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                    }; break;
            case XOR:{
                         if (!get_pattern_args(" $r, $r, $r;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao XOR esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                     }; break;
            case NOT: {
                         if (!get_pattern_args(" $r, $r;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao NOT esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                      }; break;
            case SLL:{
                         if (!get_pattern_args(" $r, $r, $r;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao SLL esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                     }; break;
            case SLR:{
                         if (!get_pattern_args(" $r, $r, $r;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao SLR esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                     }; break;
            case ORI:{
                         if (!get_pattern_args(" $r, $r, $c;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao ORI esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                     }; break;
            case XORI:{
                         if (!get_pattern_args(" $r, $r, $c;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao XORI esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                      }; break;
            case ADDI:{
                         if (!get_pattern_args(" $r, $r, $c;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao ADDI esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                      }; break;
            case DSP:{
                         if (!get_pattern_args(" $r;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao DSP esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                     }; break;
            case JMP:{
                         if (!get_pattern_args(" $l;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao JMP esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                     }; break;
            case JZ:{
                         if (!get_pattern_args(" $r, $r, $l;", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao JZ esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                    }; break;
            case HALT:{
                         if (!get_pattern_args(";", begin, end, insts[insts_sz].args, labels, labels_sz, insts_sz)){
                             fprintf(stderr, "Instrucao HALT esta com argumentos invalidos: %.*s\n", (int)(end - begin), begin);
                             return 1;
                         }
                      }; break;
        }
        insts_sz++;
        begin = end + 1;
        advance_empty_lines(&begin, &end);
    }

    for (int i = 0; i < insts_sz; i++){
        inst_t in = insts[i];
        printf("%x", in.op);
        switch (in.op){
            case NOP: case HALT:
                printf("0000000"); break;
            case ADD: case SUB: case AND: case OR: case XOR: case SLL: case SLR:
                printf("%x%x%x0000", in.args[0], in.args[1], in.args[2]); break;
            case NOT: 
                printf("%x0%x0000", in.args[0], in.args[1]); break;
            case ORI: case XORI: case ADDI: 
                printf("%x0%x%04x", in.args[0], in.args[1], in.args[2]); break;
            case DSP: 
                printf("%x000000", in.args[0]); break;
            case JMP: 
                printf("000%04x", in.args[0] & 0x0000ffff); break;
            case JZ:                                        // Para truncar numero negtivos
                printf("%x%x0%04x", in.args[0], in.args[1], in.args[2] & 0x0000ffff ); break;
        }
        printf("\n");
    }


    free(insts);
    free(buffer);

    return 0;
}
