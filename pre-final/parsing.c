#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

/* if we are compiling on windows compile these functions */
#ifdef _WIN32

#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* fake add_history function */
void add_history(char* unused) {}

/* otherwise, include the editline headers */
#else

#include <editline/readline.h>
#include <editline/history.h>

#endif

int main(int argc, char** argv) {

  /* Create parsers */
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lissp    = mpc_new("lissp");
  
  /* Define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                                                             \
      number   : /-?[0-9]+\\.?[0-9]*/ ;                                                                     \
      operator : '+' | \"add\" | '-' | \"sub\" | '%' | \"mod\" | '*' | \"mul\" | '/' | \"div\" ;  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;                                          \
      lissp    : /^/ <operator> <expr>+ /$/ ;                                                     \
    ",
    Number, Operator, Expr, Lissp);

  /* Print version and exit information */
  puts("Lissp Version 0.0.0.0.1");
  puts("Press CTRL+C to exit\n");
  
  /* create infinite loop */
  while (1) {
  
    /* now in either case readline will be correctly defined */
    char* input = readline("lissp> ");
    add_history(input);
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lissp, &r)) {
      /* on success, print the ast */
      mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    } else {
      /* Otherwise, print the error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    free(input);
    }
  
  /* Undefine and delete parsers */
  mpc_cleanup(4, Number, Operator, Expr, Lissp);
  
  return 0;
}