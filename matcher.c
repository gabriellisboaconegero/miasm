#include <stdio.h>
#include <string.h>

int is_num(char c){
    return '0' <= c && c <= '9';
}

int get_num_neg(char *str, size_t str_sz, size_t *i, int *num){
    int match = 0;
    int mult = 1;
    *num = 0;

    if (str[*i] == '-'){
        mult *= -1;
        (*i)++;
    }

    match = is_num(str[*i]);
    while(*i < str_sz && is_num(str[(*i)])){
        *num = *num * 10 + (str[(*i)] - '0');
        (*i)++;
    }

    (*i)--;
    
    return match;
}

int get_num(char *str, size_t str_sz, size_t *i, int *num){
    int match = 0;
    *num = 0;

    match = is_num(str[*i]);
    /* while(*i < str_sz && is_num(str[(*i)])){
        *num = *num * 10 + (str[(*i)] - '0');
        (*i)++;
    }

    (*i)--; */
    
    return match;
}

int match_group(const char *pttr, char *str, size_t pttr_sz, size_t str_sz, size_t *i, size_t *j);
int get_other_base_num(char *str, size_t str_sz, size_t *i){
    char *pttr = "0$[xb]";
    size_t j = 0;
    int match;
    char base_type;

    match = match_group(pttr, str, strlen(pttr), str_sz, i, &j);
    if (match){
        base_type = str[*i - 1];
        j = 0;

        switch (base_type){
            case 'x':{
                char *x_pttr = "$*$[$dabcdef]";
                match = match_group(x_pttr, str, strlen(x_pttr), str_sz, i, &j);
            }; break;
            case 'b':{
                char *x_pttr = "$*$[01]";
                match = match_group(x_pttr, str, strlen(x_pttr), str_sz, i, &j);
            }; break;
            default: printf("Nao era para chegar aqui\n");
        }
    }

    (*i)--;

    return match;
}

int match_one(const char *pttr, char *str, size_t pttr_sz, size_t str_sz, size_t *i, size_t *j){
    int match = 0;

    if (*j >= pttr_sz)
        return 1;

    if (*i >= str_sz)
        return 0;

    if (pttr[*j] == '$'){
        // advance '$'
        (*j)++;
        switch (pttr[*j]){
            case '.': match = 1; break;
            case 'd':{
                int n;
                match = get_num(str, str_sz, i, &n);
            }; break;
            case 'D':{
                int n;
                match = get_num_neg(str, str_sz, i, &n);
            }; break;
            case 'n':{
                match = get_other_base_num(str, str_sz, i);
            }; break;
            case '(':{
                // advance '('       
                (*j)++;
                size_t f = *j;
                int opened = 1;
                // get to the closing ')'
                while(f <= pttr_sz && opened != 0){
                    if (pttr[f] == '(')
                        opened++;
                    if(pttr[f] == ')')
                        opened--;
                    f++;
                }

                // we stop after ')', so
                f--;
                // check if got the clos ')'
                if (f > pttr_sz || opened != 0)
                    return 0;

                match = match_group(pttr, str, f, str_sz, i, j);

                // advance all group
                *j = f;
                // undo last match, becaus after every match (*i)++
                (*i)--;
            }; break;
            case '[':{
                // advance '['       
                (*j)++;
                size_t f = *j;
                int opened = 1;
                // get to the closing ']'
                while(f <= pttr_sz && opened != 0){
                    if (pttr[f] == '[')
                        opened++;
                    if(pttr[f] == ']')
                        opened--;
                    f++;
                }

                // we stop after ']', so
                f--;
                // check if got the clos ']'
                if (f > pttr_sz || opened != 0)
                    return 0;

                size_t tmp_i = *i;
                // equanto nao achar um match ou nao chegar no final da lista tenta dar match
                size_t tmp_j = *j;
                while (*j < f && !match){
                    tmp_i = *i;
                    tmp_j = *j;
                    match = match_one(pttr, str, pttr_sz, str_sz, &tmp_i, &tmp_j);
                    // advance the next char to not try matching it
                    *j = tmp_j;
                }
                (*j) = f;
                (*i) = tmp_i - 1;
            }; break;
            case '*':{
                // advance '*' 
                (*j)++;
                size_t tmp_j = *j;

                if (*j >= pttr_sz)
                    return 0;
                
                //printf("%s, %s\n", &pttr[*j], &str[*i]);
                match = match_one(pttr, str, pttr_sz, str_sz, i, &tmp_j);
                if (!match)
                    return 0;
                while (*i < str_sz && match){
                    tmp_j = *j;
                    match = match_one(pttr, str, pttr_sz, str_sz, i, &tmp_j);
                }
                // advance all reptition pattern, but dp tmp_j - 1 beacause stop at next patter and after every match (*j)++
                *j = tmp_j - 1;
                // undo last advance, becaus after every match (*i)++ and we stop after not match
                *i -= 2;
                
                match = 1;
            }; break;
        // if no matching char, match '$'
            default: match = str[*i] == pttr[--(*j)];
        }
    }else
        match = str[*i] == pttr[*j];

    (*i)++;
    (*j)++;

    return match;
}

int match_group(const char *pttr, char *str, size_t pttr_sz, size_t str_sz, size_t *i, size_t *j){
    int match = 1;
    
    while (match &&
           (*i < str_sz) &&
           (*j < pttr_sz))
    {
        match = match_one(pttr, str, pttr_sz, str_sz, i, j);
    }
    
    return match && *j == pttr_sz;
}

int matcher(const char *pttr, char *str){
    size_t i = 0;
    size_t j = 0;
    return match_group(pttr, str, strlen(pttr), strlen(str), &i, &j);
}

void print_test(int match, int expect){
    printf("correct: %d\n", match == expect);
}

int main(){
    //TODO: fazer ter o $+ para 1 ou mais matches
    //TODO: fazer p $* para 0 ou ais matches, fazendo match ate encontra o proximo matche do patter ou n ter match
    //TODO: fazer ${1-3} para dar match entre 1 ate 3 repticoes, num arbitrario
    //TODO: sistema de match e salvar em alguma memoria, para usar o match
    //TODO: aceitar que tenha um func add_get, que adiciona um get e adiciona na lista de $get_name para usar nos matches
    /*print_test(matcher("-$-", "-$-"), 1); 
    print_test(matcher("-$-", "-.-"), 0);
    print_test(matcher("-$.-", "-9-"), 1);
    print_test(matcher("-$[abc]-", "-d-"), 0);
    print_test(matcher("-$[]-", "-d-"), 0);
    print_test(matcher("-$d-", "-4-"), 1);
    print_test(matcher("-$d-", "-g-"), 0);
    print_test(matcher("-$d-", "-234-"), 0);
    print_test(matcher("-$d-", "--6-"), 0);
    print_test(matcher("-$D-", "-624-"), 1);
    print_test(matcher("-$D-", "--624-"), 1);
    print_test(matcher("-$[$d$D]-", "--624-"), 1);
    print_test(matcher("-$[$d$D]-", "---"), 0);
    print_test(matcher("-$[$dabcdef]-", "-1-"), 1);
    print_test(matcher("-$[$dabcdef]-", "-c-"), 1);
    print_test(matcher("-$[$dabcdef]-", "-13-"), 0);
    print_test(matcher("-$[ab$[cd]ef]-", "-e-"), 1);
    print_test(matcher("[$[$d$(-$d)]]", "[-1]"), 1);
    print_test(matcher("[$[$d$(-$d)]]", "[6]"), 1);
    print_test(matcher("0$[xb]$[01]$[01]$[01]$[01]", "0x1001"), 1);
    print_test(matcher("$n", "0xfffff"), 1);
    print_test(matcher("$*$[01]f", "0f"), 1);
    print_test(matcher("$[$(-$*$d)$*$d]", "-131"), 1);
    print_test(matcher("0$[$(x$*$[$dabcdef])$(b$*$[01])]", "0b0110"), 1); */
    print_test(matcher("$* pedro", "            pedro"), 1);
    print_test(matcher("$[$($.$.$.$.$.:)$(  $.$.$.$.$.$* ,$* r$d$* ,$* r$d;)]", "  pedro , r2 , r5;"), 1);
    
    return 0;
}
