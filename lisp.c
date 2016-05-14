#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

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
enum{LVAL_NUM,LVAL_ERR,LVAL_SYM,LVAL_SXPR};
enum{LERR_DIV_ZERO,LERR_BAD_OP,LERR_BAD_NUM};
//lval eval(mpc_ast_t *);
//lval eval_op(lval,char *,lval);
lval* lval_num(long x);
lval* lval_err(char* x);
lval* lval_sym(char* s);
lval* lval_sxpr(void);
lval* lval_del(lval* v);
lval* lval_read(mpc_ast_t* t);
lval* lval_read_nums(mpc_ast_t* t);
lval* lval_add(lval* v,lval* x);
void lval_print(lval* v);
void lval_println(lval* v);


 void lval_expr_print(lval* v,char open,char close)
   {
     putchar(open);

     for(int i=0;i<v->count;i++){
       printf("inside loop");
       lval_print(v->cell[i]);

       if (i!=(v->count-1)){
	 putchar(' ');
       }
     }
     putchar(close);

   }

int main(int argc,char *argv[])
{
  //Creating parsers
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Sxpr = mpc_new("sxpr");  
  mpc_parser_t* Lisp = mpc_new("lisp");

  //Defining the language
  mpca_lang(MPCA_LANG_DEFAULT,
		"number : /-?[0-9]+/;                                     \
                 operator :  '+' | '-' | '*' | '/' | '%' | \"max\";  \
                 expr : <number> | '('  <operator> <expr>+ ')';		  \
                 sxpr : '(' <expr>* ')';				\
                 lisp : /^/  <operator> <expr>+ /$/;  			  \
                ",
	    Number , Operator ,  Sxpr , Expr , Lisp );

		
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
      lval* result=lval_read(r.output);
      lval_println(result);
      lval_del(result);
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
  mpc_cleanup(4,Number,Operator,Sxpr,Expr,Lisp);   
  return 0;
}
/*    
lval eval(mpc_ast_t *t)
{
  //If number return a number
  if (strstr(t->tag,"number")){
    errno=0;
    long x=strtol(t->contents,NULL,10);
    return errno!=ERANGE ? lval_num(x):lval_err("Bad Number");
  }
  //The second child is a operator   
  char *op=t->children[1]->contents;

  //eval the third child
  lval x=eval(t->children[2]);
  
  // Check other children
  int i=3;
  while(strstr(t->children[i]->tag,"expr")){
    x=eval_op(x,op,eval(t->children[i]));
    i++;
  }
  return x;
}

lval eval_op(lval x,char* op,lval y){
  //Check if x or y have errors
  if (x.type==LVAL_ERR) return x;
  if (y.type==LVAL_ERR) return y;

  //Perform opertions )
  if(strcmp("+",op)==0) {return lval_num((x->num)+(y->num));}
  if(strcmp("*",op)==0) {return lval_num((x->num)*(y->num));}
  if(strcmp("/",op)==0) {return y->num==0 ? lval_err("Dvision by zero"):lval_num(x->num/y->num);}
  if(strcmp("-",op)==0) {return lval_num(x->num-y->num);}
  if(strcmp("%",op)==0) {return lval_num((x->num)%(y->num));}
  if(strcmp("max",op)==0) {
    if (x->num>y->num)
      return lval_num(x->num);
    else
      return lval_num(y->num);
  }
  return lval_err(lerr_bad_op);
}
*/
  // create a new number lval 
 lval* lval_num(long x)
 {
   lval* v=malloc(sizeof(lval));
    v->type=LVAL_NUM;
    v->num=x;
    return v;
 }

  // create a new error type lval
 lval* lval_err(char *m)
 {
   lval* v=malloc(sizeof(lval));
    v->type=LVAL_ERR;
    v->err=malloc(sizeof(m)+1);
    strcpy(v->err,m);
    return v;
 }

// create a struct for symbol
lval* lval_sym(char* s)
{
  lval* v=malloc(sizeof(lval));
  v->sym=malloc(sizeof(strlen(s)+1));
  v->type=LVAL_SYM;
  strcpy(v->sym,s);
  return v;
}

//Create a struct for Sxpr
lval* lval_sxpr(void)
{
  lval* v =malloc(sizeof(lval));
  v->type=LVAL_SXPR;
  v->count=0;
  v->cell=NULL;
  return v;
}


// Freeing all the malloc calls
lval* lval_del(lval* v)
{
  switch (v->type){
  case LVAL_NUM:
    break;
  case LVAL_ERR:
    free(v->err);
    break;
  case LVAL_SYM:
    free(v->sym);
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
  errno=0;
  long x = strtol(t->contents,NULL,10);
  return errno!=ERANGE ? lval_num(x):lval_err("Invalid Number");
}

lval* lval_add(lval* v,lval* x)
{
  v->count++;
  v->cell=realloc(v->cell,sizeof(lval* )* v->num);
  v->cell[v->count-1]=x;
  return v;
}
  

lval* lval_read(mpc_ast_t* t)
{
  //For number and symbols 
  if (strstr(t->tag,"number")) { return lval_read_nums(t);}
  if (strstr(t->tag,"symbol")) { return lval_sym(t->contents);}
  
  //If it contains S Expressions
  lval* x=NULL;
  if (strstr(t->tag,"sxpr")) { return lval_sxpr();}
  if (strstr(t->tag,">")) { return lval_sxpr();}
  
  for(int i = 0;i < t->children_num;i++){
    if (strcmp(t->children[i]->contents,"(")==0) {  continue; }
    if (strcmp(t->children[i]->contents,")")==0) {  continue; }
    if (strcmp(t->children[i]->contents,"{")==0) {  continue; }
    if (strcmp(t->children[i]->contents,"}")==0) {  continue; }
    if (strcmp(t->tag,"regex")==0) {  continue; }
    x = lval_add(x,lval_read(t->children[i]));
  }
  return x;
}

  
void lval_print(lval* v)
{
  switch (v->type){
    // Print Number
  case LVAL_NUM:
    printf("number");
    printf("%li\n",v->num);
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
    
  }
}

 void lval_println(lval* v)
 {
   lval_print(v);
   putchar('\n');
 }
 
