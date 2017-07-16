%{
#include "legex.h"

typedef struct Segex Segex;
typedef struct Symbol Symbol;
typedef struct Block Block;
typedef struct Condition Condition;
typedef struct Statement Statement;
typedef struct Arg Arg;
typedef struct Command Command;

enum {
	CMD, BLOCK
};

struct Segex
{
	int type;	// GRAPH, SEGMENT, '&'
	char *graph;
	Segment seg;
	Segex *left, *right;
};

struct Symbol
{
	char *name;
	Symbol *next;	// in symlist; tmp?
	Symbol *down;	// push down
	// values:
	Block *block;
	Language *lang;
	int keyw;
};

struct Block
{
	char *name;
	Statement *stmts;
};

typedef struct Langlist Langlist;
struct Langlist
{
	Language *lang;
	Langlist *next;
};

struct Condition
{
	int neg;
	Langlist *list;
};

struct Statement
{
	int type;
	Statement *next;	// in block

	Command *cmd;
	Condition *cond;
	Statement *s1, *s2;
	Symbol *sym;
	Block *block;
};

struct Arg
{
	int type;
	union {
		char *str;
		Symbol *sym;
		Segex *segex;
	};
};

typedef struct Arglist Arglist;
struct Arglist
{
	Arg *arg;
	Arglist *next;
};

struct Command
{
	Symbol *cmd;
	Arglist *args;
};


%}

%pure-parser
%lex-param {void *scanner}
%parse-param {void *scanner}
%define api.prefix {sc_}

%union {
	Statement *stmt;
	Command *cmd;
	Symbol *sym;
	Block *blk;
	Langlist *langl;
	Condition *cond;
	Arg *arg;
	Arglist *arglist;
	char *str;
	Segex *segex;
};
%token <str> STRING GRAPH
%token <sym> SYMBOL LANGUAGE
%token <segex> SEGMENT
%token DEF IF ELSE
%type <blk> block
%type <stmt> statement statements ifstm
%type <cmd> command
%type <langl> langlist
%type <cond> condition
%type <arg> argument
%type <arglist> arglist
%type <segex> segatm segexp
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%{
int yylex(YYSTYPE *yylval, void *scanner);
void yyerror(void *scanner, char *s);

Block *mkblock(char *name, Statement *stmts){
	Block *b;
	b = malloc(sizeof(Block));
	b->name = name;
	b->stmts = stmts;
	return b;
}
Statement *mkstmt(int t){
	Statement *stm;
	stm = malloc(sizeof(Statement));
	stm->type = t;
	stm->next = nil;
	return stm;
}
Statement *mkcmdstm(Command *c){
	Statement *stm;
	stm = mkstmt(CMD);
	stm->cmd = c;
	return stm;
}
Statement *mkblockstm(Block *b){
	Statement *stm;
	stm = mkstmt(BLOCK);
	stm->block = b;
	return stm;
}
Statement *mkdefstm(Symbol *s, Block *b){
	Statement *stm;
	stm = mkstmt(DEF);
	stm->sym = s;
	stm->block = b;
	return stm;
}
Langlist *mklanglist(Language *l, Langlist *next){
	Langlist *ll;
	ll = malloc(sizeof(Langlist));
	ll->lang = l;
	ll->next = next;
	return ll;
}
Condition *mkcond(int neg, Langlist *ll){
	Condition *cond;
	cond = malloc(sizeof(Condition));
	cond->neg = neg;
	cond->list = ll;
	return cond;
}
Statement *mkif(Condition *c, Statement *s1, Statement *s2){
	Statement *stm;
	stm = mkstmt(IF);
	stm->cond = c;
	stm->s1 = s1;
	stm->s2 = s2;
	return stm;
}
Arg *mkarg(int t, void *val){
	Arg *a;
	a = malloc(sizeof(Arg));
	a->type = t;
	switch(t){
	case GRAPH:
	case STRING: a->str = (char*)val; break;
	case SYMBOL: a->sym = (Symbol*)val; break;
	case SEGMENT: a->segex = (Segex*)val; break;
	}
	return a;
}
Arglist *mkarglist(Arg *arg, Arglist *next){
	Arglist *al;
	al = malloc(sizeof(Arglist));
	al->arg = arg;
	al->next = next;
	return al;
}
Command *mkcmd(Symbol *sym, Arglist *args){
	Command *cmd;
	cmd = malloc(sizeof(Command));
	cmd->cmd = sym;
	cmd->args = args;
	return cmd;
}
%}
%%
prog
	: globdef
	| prog globdef
	;
globdef
	: DEF SYMBOL block { $2->block = $3; }
	;
block
	: '(' ')' { $$ = mkblock(nil, nil); }
	| '(' statements ')' { $$ = mkblock(nil, $2); }
	| STRING '(' ')' { $$ = mkblock($1, nil); }
	| STRING '(' statements ')' { $$ = mkblock($1, $3); }
	;
statements
	: statement
	| statement statements { $1->next = $2; $$ = $1; }
	;
statement
	: command { $$ = mkcmdstm($1); }
	| block { $$ = mkblockstm($1); }
	| ifstm
	| DEF SYMBOL block { $$ = mkdefstm($2, $3); }
	;
langlist
	: LANGUAGE { $$ = mklanglist($1->lang, nil); }
	| LANGUAGE langlist { $$ = mklanglist($1->lang, $2); }
	;
condition
	: langlist { $$ = mkcond(0, $1); }
	| '!' langlist { $$ = mkcond(1, $2); }
	;
ifstm
	: IF '(' condition ')' statement %prec LOWER_THAN_ELSE
		{ $$ = mkif($3, $5, nil); }
	| IF '(' condition ')' statement ELSE statement
		{ $$ = mkif($3, $5, $7); }
	;
segatm
	: SEGMENT
	| GRAPH
	{
		Segex *segex;
		segex = malloc(sizeof(Segex));
		segex->type = GRAPH;
		segex->graph = $1;
		$$ = segex;
	}
	;
segexp:
	segatm
	| segexp '&' segatm
	{
		Segex *segex;
		segex = malloc(sizeof(Segex));
		segex->type = '&';
		segex->left = $1;
		segex->right = $3;
		$$ = segex;
	}
	;
argument
	: STRING { $$ = mkarg(STRING, strdup($1)); }
//	| GRAPH { $$ = mkarg(GRAPH, strdup($1)); }
	| SYMBOL { $$ = mkarg(SYMBOL, $1); }
	| LANGUAGE { $$ = mkarg(SYMBOL, $1); }
	| segexp { $$ = mkarg(SEGMENT, $1); }
	;
arglist
	: argument { $$ = mkarglist($1, nil); }
	| argument ',' arglist { $$ = mkarglist($1, $3); }
	;
command
	: SYMBOL ';' { $$ = mkcmd($1, nil); }
	| SYMBOL arglist ';' { $$ = mkcmd($1, $2); }
	;
%%

typedef struct FileScanner FileScanner;
struct FileScanner
{
	FILE *f;
	int peekc;
	int line;

	Script *sc;
};

void
parseerror_(char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
	exit(1);
}

void
yyerror(void *scanner, char *s)
{
	FileScanner *sc;
	sc = (FileScanner*)scanner;
	parseerror_("error: %s: %d\n", s, sc->line);
}

static int
nextsp(FileScanner *scn)
{
	int c;
	if(scn->peekc){
		c = scn->peekc;
		scn->peekc = 0;
		return c;
	}
	c = getc(scn->f);
	if(c == '\n')
		scn->line++;
	if(isspace(c))
		c = ' ';
	
	return c;
}

static int
next(FileScanner *scn)
{
	int c;
	while(c = nextsp(scn), c == ' ');
	return c;
}

static void
skipline(FileScanner *scn)
{
	int c;
	scn->peekc = 0;
	while(c = getc(scn->f), c != EOF && c != '\n');
	scn->line++;
}

static Symbol*
lookup(Symbol **symlist, char *name)
{
	Symbol *s;
	for(s = *symlist; s; s = s->next)
		if(strcmp(s->name, name) == 0)
			return s;
	s = malloc(sizeof(Symbol));
	s->name = strdup(name);
	s->keyw = -1;
	s->lang = nil;
	s->next = nil;
	s->down = nil;
	s->block = nil;
	s->next = *symlist;
	*symlist = s;
	return s;
}

int
yylex(YYSTYPE *yylval, void *scanner)
{
	int c;
	FileScanner *scn;
	Symbol *sym;
	Segex *segex;
	char *s, buf[256];

	scn = scanner;
loop:
	c = next(scn);
	if(c == EOF)
		return 0;
	if(c == '{'){
		s = buf;
		while(c = getc(scn->f), c != EOF && c != '}')
			*s++ = c;
		*s++ = '\0';
		segex = malloc(sizeof(Segex));
		segex->type = SEGMENT;
		segex->seg = strtoseg(buf);
		yylval->segex = segex;
		return SEGMENT;
	}
	if(c == '\''){
		s = buf;
		while(c = getc(scn->f), c != EOF && c != '\'')
			*s++ = c;
		*s++ = '\0';
		yylval->str = strdup(buf);
		return GRAPH;
	}
	if(c == '"'){
		s = buf;
		while(c = getc(scn->f), c != '"')
			*s++ = c;
		*s++ = '\0';
		yylval->str = strdup(buf);
		return STRING;
	}
	if(isalpha(c) || c == '_'){
		s = buf;
		*s++ = c;
		while(c = getc(scn->f), isalpha(c) || isdigit(c) || c == '_')
			*s++ = c;
		*s++ = '\0';
		scn->peekc = c;
		sym = lookup(&scn->sc->symlist, buf);
		yylval->sym = sym;
		return sym->keyw >= 0 ? sym->keyw :
		       sym->lang ? LANGUAGE :
		       SYMBOL;
	}
	if(c == '/'){
		if(c = getc(scn->f), c == '/'){
			skipline(scn);
			goto loop;
		}
		scn->peekc = c;
		return '/';
	}
	return c;
}

static void printind(int i) { while(i--) printf("	"); }

static void printblk(Block *b, int ind);

static void
printcond(Condition *cond)
{
	Langlist *ll;
	if(cond->neg)
		printf("! ");
	for(ll = cond->list; ll; ll = ll->next){
		if(ll != cond->list)
			printf(" ");
		printf("%s", ll->lang->name);
	}
}

static void
printsegex(Segex *segex)
{
	char buf[256];
	switch(segex->type){
	case SEGMENT:
		segtostr(buf, segex->seg, 0);
		printf(" %s", buf);
		break;
	case GRAPH:
		printf(" '%s'", segex->graph);
		break;
	case '&':
		printsegex(segex->left);
		printf(" &");
		printsegex(segex->right);
		break;
	}
}

static void
printstmt(Statement *stmt, int ind)
{
	Arglist *al;
	if(stmt->type == CMD){
		printind(ind);
		printf("%s", stmt->cmd->cmd->name);
		for(al = stmt->cmd->args; al; al = al->next){
			if(al != stmt->cmd->args)
				printf(",");
			switch(al->arg->type){
			case STRING:
				printf(" \"%s\"", al->arg->str);
				break;
			case GRAPH:
				printf(" '%s'", al->arg->str);
				break;
			case SYMBOL:
				printf(" %s", al->arg->sym->name);
				break;
			case SEGMENT:
				printsegex(al->arg->segex);
				break;
			}
		}
		printf(";\n");
	}else if(stmt->type == IF){
		printind(ind);
		printf("if(");
		printcond(stmt->cond);
		printf(")\n");
		if(stmt->s1->type == BLOCK)
			printstmt(stmt->s1, ind);
		else
			printstmt(stmt->s1, ind+1);
		if(stmt->s2){
			printind(ind);
			printf("else\n");
			if(stmt->s1->type == BLOCK)
				printstmt(stmt->s2, ind);
			else
				printstmt(stmt->s2, ind+1);
		}
	}else if(stmt->type == BLOCK){
		printind(ind);
		printblk(stmt->block, ind);
	}else if(stmt->type == DEF){
		printind(ind);
		printf("def %s ", stmt->sym->name);
		printblk(stmt->block, ind);
	}
}

static void
printblk(Block *b, int ind)
{
	Statement *stmt;
	if(b->name)
		printf("\"%s\" ", b->name);
	printf("{\n");
	for(stmt = b->stmts; stmt; stmt = stmt->next)
		printstmt(stmt, ind+1);
	printind(ind);
	printf("}\n");
}

void
printdefs(Script *sc)
{
	Symbol *s;
	for(s = sc->symlist; s; s = s->next)
		if(s->block){
			printf("def %s ", s->name);
			printblk(s->block, 0);
		}
}

static void
deflangtree(Script *sc, Language *root)
{
	Language *child;
	
	lookup(&sc->symlist, root->name)->lang = root;
	for(child = root->child; child; child = child->next)
		deflangtree(sc, child);
}

void*
legscrcompile(Script *sc, char *from, char *to)
{
	Language *lfrom, *lto;
	Language *l;

	lfrom = lookup(&sc->symlist, from)->lang;
	lto = lookup(&sc->symlist, to)->lang;
	if(lfrom == nil || lto == nil){
		fprintf(stderr, "language not defined\n");
		return nil;
	}
	for(l = lto; l; l = l->parent)
		if(l == lfrom)
			goto found;
	fprintf(stderr, "No evolution\n");
	return nil;
found:
	printf("yay\n");
	return (void*)1;
}

Script*
scriptparse(FILE *f, Language *tree)
{
	FileScanner scn;
	Script *sc;

	sc = malloc(sizeof(Script));
	sc->symlist = nil;
	sc->langtree = tree;

	lookup(&sc->symlist, "def")->keyw = DEF;
	lookup(&sc->symlist, "if")->keyw = IF;
	lookup(&sc->symlist, "else")->keyw = ELSE;

	deflangtree(sc, sc->langtree);

	scn.f = f;
	scn.peekc = 0;
	scn.line = 1;
	scn.sc = sc;
	yyparse(&scn);
	return sc;
}
