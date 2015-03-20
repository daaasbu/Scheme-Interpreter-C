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

int lex_input(char *s, char **start, char **end);
	


//functions to parse user input
int parse_input(char *s, object *o);
int parse_int(char *s, object *o);
int parse_sym(char *s, object *o);
//object parse_list(char *s);
int parse_exp(char *s, object *o);

//prints an object
void print_expr(object o);

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
	while(strcmp(str, "exit") != 0) {
		printf("> ");
		gets(str);
		o = parse_input(str);
		print_expr(o);
		printf("\n");
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


//finds the starting and ending place in s of the first token
ints lex_input(char *s, char *start, char *end) {
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
object parse_input(char *s) {
	char first = s[0];
	char second = s[1];
	object o;
	if (intp(first)) {//first char is a number
		return parse_int(s);			
	}
	else if (first == 39 && second == '(' ) { //39 is ascii for quote, so this is a list 
		//return parse_list(s);
	}
	else if (first == 39) { //quote without a paren, means symbol
		return parse_sym(s);
	}
	else if (first == '(') { //function application
		return parse_exp(s);
	}
	else { //error
		printf("Not a valid input. \n");
		return make_void();
	}
}

object parse_int(char *s) {
	int i = 0;
	long num = 0;
	while (s[i] != '\0') {
		if (intp(s[i])) {
			num = (10 * (num + (s[i] - '0'))); //converts char to int, and then accumulates it.
		}
		else { //input error
			printf("Invalid Number. \n");
			return make_void();
		}
		i++;
	}
	return make_int(num/10);
}


object parse_sym(char *s) {
	return make_sym(s);
}

object parse_exp(char *s) {
	return make_void();
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
   
    
