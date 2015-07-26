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

long max(long x, long y) {
  if (x > y) {
    return x;
  } else { return y; }
}

long min(long x, long y) {
  if (x > y) {
    return y;
  } else { return x; }
}

long eval_op(long x, char* op, long y) {

  if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) { return x + y; }
  if (strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) { return x - y; }
  if (strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) { return x * y; }
  if (strcmp(op, "/") == 0 || strcmp(op, "div") == 0) { return x / y; }
  if (strcmp(op, "%") == 0 || strcmp(op, "mod") == 0) { return x % y; }
  if (strcmp(op, ">") == 0 || strcmp(op, "max") == 0) { return max(x, y); }
  if (strcmp(op, "<") == 0 || strcmp(op, "min") == 0) { return min(x, y); }
  return 0;
}
int num_count = 0;

long eval(mpc_ast_t*t) {

  /* if tagged as number return it directly, otherwise expression. */
  if (strstr(t->tag, "number")) {
    num_count += 1;
    return atoi(t->contents);
  }
  
  /* the operator is always the second child */
  char* op = t->children[1]->contents;
  
  /* store the 3rd child as x */
  long x = eval(t->children[2]);
  
  /* iterate through remaining children, combining using operator */
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }
  
  /* when there is only one argument with a subtraction operator, negate it */
  if (num_count == 1 && *op == '-') {
    return -x;
  } else { return x; }
}

int main(int argc, char** argv) {

  /* Create parsers */
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lissp    = mpc_new("lissp");
  
  /* Define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                                                                                             \
      number   : /-?[0-9]+\\.?[0-9]*/ ;                                                                                           \
      operator : '+' | \"add\" | '-' | \"sub\" | '%' | \"mod\" | '*' | \"mul\" | '/' | \"div\" | '>' | \"max\" | '<' | \"min\" ;  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;                                                                          \
      lissp    : /^/ <operator> <expr>+ /$/ ;                                                                                     \
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

      /* on success, print the evaluated output */
      long result = eval(r.output);
      printf("%li\n", result);
      mpc_ast_delete(r.output);

    } else {
      /* Otherwise, print the error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    free(input);
    num_count = 0;
    }
  
  /* Undefine and delete parsers */
  mpc_cleanup(4, Number, Operator, Expr, Lissp);
  
  return 0;
}