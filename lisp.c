#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"
#define LASSERT(args,cond,err)\
  if (!(cond)) {lval_del(args);return lval_err(err);}
// For windows
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  //Char pointer alocation for string
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  //Escape sequence
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

typedef struct lval{
  int type;
  long num;
  char *err;
  char *sym;
  int count;
  struct lval** cell;
}lval;
enum{LVAL_NUM,LVAL_ERR,LVAL_SYM,LVAL_SXPR,LVAL_QEXPR};
enum{LERR_DIV_ZERO,LERR_BAD_OP,LERR_BAD_NUM};

lval* builtin_op(lval* a,char* op);
lval* lval_eval_sxpr(lval *v);

lval* lval_qexpr(void)
{
  // Create a new struct for q expressions
  lval* v = malloc(sizeof(v));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_num(int i)
{
  //Create a new struct for num
  lval* v=malloc(sizeof(lval));
  v->type=LVAL_NUM;
  v->num = i;
  return v;
}

lval* lval_err(char *m)
    // create a new error type lval
 {
   lval* v=malloc(sizeof(lval));
    v->type=LVAL_ERR;
    v->err=malloc(sizeof(m)+1);
    strcpy(v->err,m);
    return v;
 }


lval* lval_sym(char* s)
{
  // create a struct for symbol
  lval* v=malloc(sizeof(lval));
  v->sym=malloc(sizeof(strlen(s)+1));
  v->type=LVAL_SYM;
  strcpy(v->sym,s);
  return v;
}


lval* lval_sxpr(void)
{
  //Create a struct for Sxpr
  lval* v =malloc(sizeof(lval));
  v->type=LVAL_SXPR;
  v->count=0;
  v->cell=NULL;
  return v;
}


void lval_del(lval* v)
{
  // Freeing all the malloc calls
  switch (v->type){
  case LVAL_NUM:
    break;
  case LVAL_ERR:
    free(v->err);
    break;

  case LVAL_SYM:
    free(v->sym);
    break;
  case LVAL_QEXPR:    
  case LVAL_SXPR:
    for (int i=0;i<v->count;i++){
      lval_del(v->cell[i]);
    }
    free(v->cell);
    break;
  }
  free(v);
}

    
lval* lval_read_nums(mpc_ast_t* t)
{
  // Convert to long
  errno=0;
  long x = strtol(t->contents,NULL,10);
  return errno!=ERANGE ? lval_num(x):lval_err("Invalid Number");
}

lval* lval_add(lval* v,lval* x)
{
  //Add to list
  v->count++;
  v->cell=realloc(v->cell,sizeof(lval* )* v->count);
  v->cell[v->count-1]=x;
  return v;
}
  

lval* lval_read(mpc_ast_t* t)
{
  //For number and symbols 
  if (strstr(t->tag,"number")) { return lval_read_nums(t);}
  if (strstr(t->tag,"operator")) { return lval_sym(t->contents);}
  
  //If it contains S Expressions
  lval* x=NULL;
  if (strstr(t->tag,"sxpr")) {  x=lval_sxpr();}
  if (strstr(t->tag,"qexpr")) {  x=lval_qexpr();}  
  if (strcmp(t->tag,">")==0) {  x=lval_sxpr();}
  
  for(int i = 0;i < t->children_num;i++){
    if (strcmp(t->children[i]->contents,"(")==0) {  continue; }
    if (strcmp(t->children[i]->contents,")")==0) {  continue; }
    if (strcmp(t->children[i]->contents,"{")==0) {  continue; }
    if (strcmp(t->children[i]->contents,"}")==0) {  continue; }
    if (strcmp(t->children[i]->tag,"regex")==0) {  continue; }
    x = lval_add(x,lval_read(t->children[i]));
  }
  return x;
}

void lval_print(lval* v);

 void lval_expr_print(lval* v,char open,char close)
 {
   //Print Expr
     putchar(open);

     for(int i=0;i<v->count;i++){
       lval_print(v->cell[i]);

       if (i!=(v->count-1)){
	 putchar(' ');
       }
     }
     putchar(close);

   }

  
void lval_print(lval* v)
{
  switch (v->type){
    // Print Number
  case LVAL_NUM:
    printf("%li",v->num);
    break;
  case LVAL_ERR:
    printf ("ERROR :\t%s",v->err);
    break;
  case LVAL_SYM:
    printf ("%s",v->sym);
    break;
  case LVAL_SXPR:
    lval_expr_print(v,'(',')');
    break;
  case LVAL_QEXPR:
    lval_expr_print(v,'{','}');
    break;
    
  }
}

 void lval_println(lval* v)
 {
   lval_print(v);
   putchar('\n');
 }


lval* lval_pop(lval* v, int i)
{
  /* Pops the ith element and shifts the list */
  lval* x = v->cell[i];
  memmove(&v->cell[i],&v->cell[i+1],
	  sizeof(lval *)*(v->count-i-1));
  v->count--;
  v->cell=realloc(v->cell,sizeof(lval *) * v->count);
  return x;
}


lval* lval_take(lval* v , int i)
{
  /* Pops the ith element and delets the list */
  lval* x=lval_pop(v,i);
  lval_del(v);
  return x;
}
  

lval* lval_eval(lval* v)
{
  /* Checks if a struct is a S expression */
  if (v->type == LVAL_SXPR){
    return lval_eval_sxpr(v);
  }
  return v;
}
    
lval* builtin(lval* a,char* func);
lval* lval_eval_sxpr(lval *v)
{
  /*Evalutating S expressions*/
  
  for (int i=0 ; i<v->count;i++){
    v->cell[i]=lval_eval(v->cell[i]);
  }

  //If any error then pop the first element and delete the struct 
  for (int i=0 ; i<v->count;i++){
    if (v->cell[i]->type==LVAL_ERR){return lval_take(v,i);}
  }

  // If the count is one pop first element and delete struct    
  if (v->count==0) {return v;}
  if (v->count==1) {return lval_take(v,0); }

  // Ensure that the first element is a symbol
  lval* x= lval_pop(v,0);
  if(x->type != LVAL_SYM){
    lval_del(x);
    lval_del(v);
    return lval_err("Not a symbol"); 
  }

  lval* result = builtin(v,x->sym);
  lval_del(x);
  return result;
}

lval* builtin_list(lval* a)
{
  //Convert S expressions to Q expressions
  a->type=LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lval* a)
{
  // Evaluate a Q expression
  LASSERT (a,a->count != 1,"Too many arguments");
  LASSERT (a,a->cell[0]->type != LVAL_QEXPR,"Wrong type");
  lval* v = lval_take(a,0);
  v->type = LVAL_SXPR;
  return lval_eval(v);
}

lval* builtin_head(lval* a)
{
  // Pops the first element and deletes the rest
  LASSERT (a,a->count != 1,"Too many arguments");
  LASSERT (a,a->cell[0]->type != LVAL_QEXPR,"Wrong type");
  LASSERT (a,a->cell[0]->count == 0,"head alredy passed");
  lval* v = lval_take(a,0);
 // Pops the cell at 1st index and delets it.
  while (v->count >1 ){lval_del(lval_pop(v,1));}
  return v;

}

lval* builtin_tail(lval* a)
{
  // Returns Qexpr with the first element removed
  LASSERT (a,a->count != 1,"Too many arguments");
  LASSERT (a,a->cell[0]->type != LVAL_QEXPR,"Wrong type");
  LASSERT (a,a->cell[0]->count == 0,"head alredy passed");
  lval* v = lval_pop(a,0);
  lval_del(lval_pop(v,0));
  return v;
  
}

lval* builtin_op(lval* a,char* op)
{

  /* Evaluate expressions */
  for (int i=0;i<a->count;i++){
    if (a->cell[i]->type != LVAL_NUM){
      lval_del(a);
      return lval_err("Cannot operate on a number");
    }
  }

  //Pop the first element 
  lval* x = lval_pop(a,0);

  //Check if the number is negative
  if ((strcmp(op,"-")==0)&&a->count==0) { x->num = -x->num;}

  while (a->count > 0 ){
    lval* y =lval_pop(a,0);
    if (strcmp(op,"+")==0) { x->num += y->num;}
    if (strcmp(op,"-")==0) { x->num -= y->num;}
    if (strcmp(op,"*")==0) { x->num *= y->num;}
    if (strcmp(op,"/")==0) {
      if (y->num == 0){
	lval_del(x);
	lval_del(y);
	x= lval_err("Division by zero");
	break;
      }
      x->num /= y->num;
    }

    lval_del(y);   
  }

  lval_del(a);
  return x;
}

lval* lval_join(lval* a,lval* v)
{
  // Add each v to a
  while (v->count){
    lval_add(a,lval_pop(v,0));
  }

  lval_del(v);
  return a;
}

lval* builtin_join(lval* a)
{
  // Join multiple Q expressions.
  for(int i=0;i < a->count ;i++){
    LASSERT(a,a->type=LVAL_QEXPR,"wrong type");
  }

  lval* x = lval_pop(a,0);

  while(a->count){
    x = lval_join(x,lval_pop(a,0));
  }

  lval_del(a);
  return x;
}


lval* builtin(lval* a,char* func)
{
  //Evaluate Q expressions
  if (strcmp("list",func)==0) {return builtin_list(a);}
  if (strcmp("head",func)==0) {return builtin_head(a);}
  if (strcmp("tail",func)==0) {return builtin_tail(a);}
  if (strcmp("join",func)==0) {return builtin_join(a);}
  if (strcmp("eval",func)==0) {return builtin_eval(a);}
  if (strstr("+-/*",func)==0) {return builtin_op(a,func);}
  lval_del(a);
  return lval_err("Unknown Function");

}



int main(int argc,char *argv[])
{
  //Creating parsers
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Qexpr = mpc_new("qexpr");    
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Sxpr = mpc_new("sxpr");  
  mpc_parser_t* Lisp = mpc_new("lisp");


  //Defining the language
  mpca_lang(MPCA_LANG_DEFAULT,
		"number : /-?[0-9]+/;                                     \
                 operator :  '+' | '-' | '*' | '/' | '%' | \"max\" | \"list \" | \"head \" | \"tail \" | \"join \" | \"eval\";  \
                 expr : <number> |  <operator> |  <sxpr> | <qexpr>;		  \
                 sxpr : '(' <expr>* ')';				\
                 qexpr : '{' <expr>* '}';				\
                 lisp : /^/  <expr>+ /$/;  			  \
                ",
	    Number , Operator ,  Sxpr , Expr ,Qexpr , Lisp );
  puts("Lisp version 1");
  puts("Press CTL-C to exit");
  //Infinite Loop
  while(1){
    
    // Pointer to input string

    char *buffer = readline("lisp> ");
    
    // Add to history 
    add_history(buffer);
    
    //Parse the input
    mpc_result_t r;
    if (mpc_parse("<stdin>",buffer,Lisp,&r)){
      //Call eval
      lval* result=lval_eval(lval_read(r.output));
      lval_println(result);
      lval_del(result);
      mpc_ast_delete(r.output);
    }
    else{
      //If any error  
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
    //Free the pointer
    free(buffer);

    //Delete the parser

  }
  mpc_cleanup(4,Number,Operator,Sxpr,Expr,Qexpr,Lisp);   
  return 0;
}
