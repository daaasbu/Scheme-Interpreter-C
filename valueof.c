#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//a struct that will represent our Scheme objects. 
struct object {
  enum {
    type_null,
    type_pair,
    type_symbol,
    type_integer,
    type_void
  } type;
  
  union {
    struct pair *pair;
    const char *symbol;
    long integer;
  } value;
};


//Error Messages

typedef enum {
  error_valid = 0,
  error_invalid_int,
  error_mismatched_paren,
  error_invalid_app,
  error_undefined_var,
  error_invalid_syntax
} error;



//a struct that will represent a Pair.

struct pair {
  struct object objs[2];
};

typedef struct object object;


/*========Global Variables======*/

object symbol_table = { type_null };
//strings to use for lexxing
char *whitespace = " \t\n";
char *endtoken = "() \t\n";
char *parens = "()";


/*========Naming Conventions=============
  pair == pr
  object == o
  car == a
  cdr == d

*/
/*=========MACROS========================*/

#define car(pr) ((pr).value.pair->objs[0])
#define cdr(pr) ((pr).value.pair->objs[1])

#define nl() (printf("\n"));
/*======PROTOTYPES==================*/

object make_pair(object a, object d);
object make_sym(const char *s);
object make_int(long x);
object make_void();

int lex_input(char *s, char *start, char *end);
	


//functions to parse user input
int parse_input(char *s, char *end, object *o);
int parse_int(char *s, object *o, int pos, char *end);
int parse_sym(char *s, object *o, char *end);
int parse_list(char *s, char *start, object *o, char *end, int count);
int parse_exp(char *s, object *o);

//prints an object
void print_expr(object o);

//prints error message
void print_error(int err);

//the actual interpreter
object value_of(object sym, object env);

//environment functions
object extend_env(object sym, object val, object env);
object empty_env();
object apply_env(object env, object sym);

/*=========OBJECT PREDICATES ============ */
#define nullp(o) ((o).type == type_null)
#define integerp(o) ((o).type == type_integer)
#define symbolp(o) ((o).type == type_symbol)
#define pairp(o) ((o).type == type_pair)

int listp(object expr) {
  if (nullp(expr)) {
    return 1;
  }
  else if (!pairp(expr)) {
    return 0;
  }
  else {
    listp(cdr(expr));
  }
}

#define intp(c) ((c) >= 48) && ((c) <= 57)



/*=======Objects========*/
static const object null = { type_null };





/*========MAIN==========*/

int main() {
  char str[1000];
  object o;
  int err;
  while(strcmp(str, "exit") != 0) {
    printf("> ");
    gets(str);
    err = parse_input(str, str+strlen(str), &o);
    if (!err) {
    print_expr(o);
    printf("\n");
    }
    else {
      print_error(err); 
    }
  } 
  return 0;
}


/*=======Object Constructors==========*/

//Implementing cons. Takes two objects, the first being any object and the second being a pair and returns a newly allocated pair.

object make_pair(object a, object d) {
  object o;
  o.type = type_pair;
  o.value.pair = malloc(sizeof(struct pair));
  car(o) = a;
  cdr(o) = d;
  return o;
}

//makes an int object, sets the type and the value.
object make_int(long x) {
  object o;
  o.type = type_integer;
  o.value.integer = x;
  return o;
}

//makes a symbol object, takes a string, and duplicates it and sets it as the value.
object make_sym(const char *s) {
  object o;
  o.type = type_symbol;
  o.value.symbol = strdup(s);
  return o;
}

//returns the void object
object make_void() {
  object o;
  o.type = type_void;
  return o;
}

void print_expr(object o) {
  switch (o.type) {
  case type_null:
    printf("()");
    break;
  case type_pair:
    putchar('(');
    print_expr(car(o)); //prints the car, and sets the object to the cdr.
    o = cdr(o);
    while (!nullp(o)) {
      if (pairp(o)) {
        putchar(' ');
        print_expr(car(o));
        o = cdr(o);
      }
      else {
        printf(" . ");
        print_expr(o);
        break;
      }
    }
    putchar(')');
    break;
  case type_symbol:
    printf("%s",o.value.symbol);
    break;
  case type_integer:
    printf("%ld",o.value.integer);
    break;
  default:
    printf("Error: not a proper object\n");
  }
}

 void print_error(int err) {
   switch (err) {
   case error_invalid_int :
     printf("Invalid integer. \n");
     break;
   case error_mismatched_paren :
     printf("Mismatched parenthesis. \n");
     break;
   case error_invalid_app :
     printf("Invalid function application. \n");
     break;
   case error_undefined_var :
     printf("Undefined variable. \n");
     break;
   case error_invalid_syntax :
     printf("Invalid syntax. \n");
     break;
   }
 }


//finds the starting and ending place in s of the first token
int lex_input(char *s, char *start, char *end) {
  s += strspn(s,whitespace); //skips pointless whitespace

  if (s[0] == '\0') {//if null char, then we know the user entered onlywhitespace
    end = NULL;
    start = NULL;
    return error_invalid_syntax;
  }
  start = s; //else we know something besides whitespace was found
  if (strchr(parens,s[0]) != NULL) { //if the first character is either a left/right paren
    end = s+1; //then the end should be the vary next space. 
  }
  else {
    end = s + strcspn(s, endtoken); //if its not a paren, then its a symbol, and we will scan
    //until we hit whitespace, or a paren
  }
  return error_valid; //if we get here, then we know there aren't any errors. 
}



//this will parse user input in the repl, and either throw an error
//or return an object. 
int parse_input(char *s, char *end, object *o) {
  char first = s[0];
  char second = s[1];
  if (intp(first)) {//first char is a number
    return parse_int(s, o, 1, end);			
  }
  else if (first == '-' && intp(second)) {
    s[0] = '0';
    return parse_int(s, o, 0, end);
  }
  else if (first == 39 && second == '(' ) { //39 is ascii for quote, so this is a list 
    char *e;
    return parse_list(s,s+1,o,e,1);
  }
  else if (first == 39) { //quote without a paren, means symbol
    return parse_sym(s, o, end);
  }
  else if (first == '(') { //function application
    //    return parse_exp(s, o);
  }
  else { //error
    return error_invalid_syntax;
  }
}

int parse_int(char *s, object *o, int pos, char *end) {
  int i = 0;
  long num = 0;
  while (s[i] != '\0' || s == end) { //either we hit the null or we hit end
    if (intp(s[i])) {
      num = (10 * (num + (s[i] - '0'))); //converts char to int, and then accumulates it.
    }
    else { //input error
      return error_invalid_int;
    }
    i++;
  }
  if (pos) {
    *o = make_int(num/10);
  }
  else {
    *o = make_int( -1 * (num/10));
  }
  return error_valid;
}


int parse_sym(char *s, object *o, char *end) {
  char cpy[end-s];
  memcpy(cpy, s, end-s);
  cpy[end-s] = '\0';
  *o = make_sym(cpy);
  return error_valid;
}

//s is the whole string, start will mark the beginning of the specific list, end will find its matching right paren
//count is to keep track of the parens in the entire list. 
int parse_list(char *s, char *start, object *o, char *end, int count) {
  int nbal = 1; //a local count of the parens
  if (nbal) {
    
  }

  else { //if 0, then we know we have processed this list

  }
  
}


int parse_exp(char *s, object *o) {
  return error_valid;
}



//============Env functions==================
object empty_env() {
  return null;
}
//adds a symbol and its corresponding value to the front of the env.
object extend_env(object sym, object val, object env) {
  object a = make_pair(sym,val);
  env = make_pair(a,env);
  return env;
}

//looks up a symbol and returns its corresponding value in the environment
object apply_env(object sym, object env) {
  
  if (nullp(env)){
    printf("Error: Unbound variable %s\n",sym.value.symbol);
  }
    
  else {
    object a = car(env);
    if (strcmp(car(a).value.symbol, sym.value.symbol) == 0) {
      return cdr(a);
    }
    else {
      return apply_env(sym,cdr(env));
    }
  }
}
   
    
