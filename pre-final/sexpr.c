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

/* create enumeration of possible lval types */
typedef enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR } lval_type;


typedef struct lval {
  lval_type type;
  double num;
  
  /* error and symbol have string data */
  char* err;
  char* sym;
  
  /* count and pointer to list of lval* */
  int count;
  struct lval** cell;
} lval;

/* construct pointer to new number lval */
lval* lval_num(double x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* construct pointer to new error lval */
lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

/* construct pointer to new symbol lval */
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

/* pointer to an empty sexpr lval */
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lval_del(lval* v) {
  
  switch (v->type) {
    /* do nothing special for number type */
    case LVAL_NUM: break;
    
    /* for err and sym free string data */
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;
    
    /* if sexpr then delete all elements inside */
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }
      /* also free memory allocated to contain the pointers */
      free(v->cell);
    break;
  }
  
  /* finally, free memory allocated for the lval struct itself */
  free(v);
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

lval* lval_read_num(mpc_ast_t* t) {
  char *end;
  errno = 0;
  double x = strtod(t->contents, &end);
  return (*end || errno == EINVAL || errno == ERANGE) ? lval_err("Invalid number.") : lval_num(x);
}

lval* lval_read(mpc_ast_t* t) {

  /* if symbol or number return conversion to that type */
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
  
  /* if root (>) or sexpr then create empty list */
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
  
  /* fill this list with any valid expression contained within */
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }
  
  return x;
}

void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {
  
    /* print value contained within */
    lval_print(v->cell[i]);
    
    /* delete trailing space if last element */
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM:   printf("%g", v->num); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
  }
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

double max(double x, double y) {
  if (x > y) {
    return x;
  } else { return y; }
}

double min(double x, double y) {
  if (x > y) {
    return y;
  } else { return x; }
}

lval* lval_pop(lval* v, int i) {
  /* find the item at "i" */
  lval* x = v->cell[i];
  
  /* shift the memory following item at "i" over the top of it */
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
  
  /* decrease count of items in the list */
  v->count--;
  
  /* reallocate the memory used */
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* builtin_op(lval* a, char* op) {
  
  /* ensure all arguments are numbers */
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-numbers!");
    }
  }
  
  /* pop the first element */
  lval* x = lval_pop(a, 0);
  
  /* if no arguments and sub then perform unary negation */
  if ((strcmp(op, "-") == 0) && a->count == 0) { x->num = -x->num; }
  
  /* while there are elements remaining */
  while (a->count > 0) {
  
    /* pop next element */
    lval* y = lval_pop(a, 0);
    
    /* perform operation */
    if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) { x->num *= y->num; }
    if (strcmp(op, ">") == 0 || strcmp(op, "max") == 0) { x->num = max(x->num, y->num); }
    if (strcmp(op, "<") == 0 || strcmp(op, "min") == 0) { x->num = min(x->num, y->num); }
    if (strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
      if (y->num == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Division by zero!"); break;
      }
      x->num /= y->num;
    }
    if (strcmp(op, "%") == 0 || strcmp(op, "mod") == 0) {
      if (y->num == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Division by zero!"); break;
      }
      long tempx = x->num;
      long tempy = y->num;
      x->num = tempx % tempy;
    }
    
    /* delete element when finished with it */
    lval_del(y);
  }
  
  /* delete input expression and return result */
  lval_del(a);
  return x;
}

lval* lval_eval(lval* v);

lval* lval_eval_sexpr(lval* v) {

  /* evaluate children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }
  
  /* error checking */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }
  
  /* empty expression */
  if (v->count == 0) { return v; }
  
  /* single expression */
  if (v->count == 1) { return lval_take(v, 0); }
  
  /* ensure first element is symbol */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f); lval_del(v);
    return lval_err("S-Expression does not start with symbol.");
  }
  
  /* call builtin with operator */
  lval* result = builtin_op(v, f->sym);
  lval_del(f);
  return result;
}

lval* lval_eval(lval* v) {
  /* evaluate s-expressions */
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
  /* other lval types stay the same */
  return v;
}


int main(int argc, char** argv) {

  /* Create parsers */
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr    = mpc_new("sexpr");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lissp    = mpc_new("lissp");
  
  /* Define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                                                                                             \
      number   : /-?[0-9]+\\.?[0-9]*/ ;                                                                                           \
      symbol   : '+' | \"add\" | '-' | \"sub\" | '%' | \"mod\" | '*' | \"mul\" | '/' | \"div\" | '>' | \"max\" | '<' | \"min\" ;  \
      sexpr    : '(' <expr>* ')' ;                                                                                                \
      expr     : <number> | <symbol> | <sexpr> ;                                                                          \
      lissp    : /^/ <expr>* /$/ ;                                                                                     \
    ",
    Number, Symbol, Sexpr, Expr, Lissp);

  /* Print version and exit information */
  puts("Lissp Version 0.0.0.0.4");
  puts("Press CTRL+C to exit\n");
  
  /* create infinite loop */
  while (1) {
  
    /* now in either case readline will be correctly defined */
    char* input = readline("lissp> ");
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lissp, &r)) {

      /* on success, print the evaluated output */
      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);

    } else {
      /* Otherwise, print the error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    free(input);
    }
  
  /* Undefine and delete parsers */
  mpc_cleanup(3, Number, Symbol, Sexpr, Expr, Lissp);
  
  return 0;
}