%{
#include "legex.h"

typedef struct Node Node;
struct Node
{
	int type;
	Node *left, *right;
	Segment seg;
	SegSet *set;
};
static Node *makenode(int type, Node *left, Node *right);
enum {
	// node types and opcodes
	Bol, Eol, Bound, Dot, Empty,
	Group,
	Segm, Class, Set, NotSet,
	Star, Plus, Quest,
	Conc, Alt,

	// only opcodes
	Branch, Jump, Save, End
	
};

static char *types[] = {
	"Bol", "Eol", "Bound", "Dot", "Empty",
	"Group",
	"Segm", "Class", "Set", "NotSet",
	"Star", "Plus", "Quest",
	"Conc", "Alt",
	"Branch", "Jump", "Save", "End"
};

Node *result;
%}


%pure-parser
%lex-param {void *scanner}
%parse-param {void *scanner}
%define api.prefix {lx_}

%union {
	Node *node;
}
%token <node> ATOM BOUND BOUNDL BOUNDR
%type <node> legex atom qatom conc qconc grp expr
%{
int yylex(YYSTYPE *yylval, void *scanner);
void yyerror(void *scanner, char *s);
%}
%%
legex:	  expr { result = $1; }
	;
atom:	  ATOM { $$ = $1; }
	| '_' { $$ = makenode(Empty, nil, nil); }
	| '.' { $$ = makenode(Dot, nil, nil); }
	| BOUNDL { $$ = makenode(Bol, nil, nil); }
	| BOUNDR { $$ = makenode(Eol, nil, nil); }
	| BOUND { $$ = makenode(Bound, nil, nil); }
	| '(' grp ')' { $$ = $2; }
	;
qatom:	  atom { $$ = $1; }
	| atom '*' { $$ = makenode(Star, $1, nil); }
	| atom '+' { $$ = makenode(Plus, $1, nil); }
	| atom '?' { $$ = makenode(Quest, $1, nil); }
	;
conc:	  qatom { $$ = $1; }
	| conc qatom { $$ = makenode(Conc, $1, $2); }
	;
qconc:	  conc { $$ = $1; }
	| '*' conc { $$ = makenode(Star, $2, nil); }
	| '+' conc { $$ = makenode(Plus, $2, nil); }
	| '?' conc { $$ = makenode(Quest, $2, nil); }
	;
grp:	  qconc { $$ = $1; }
	| grp '|' qconc { $$ = makenode(Alt, $1, $3); }
	;
expr:	  grp { $$ = makenode(Group, $1, nil); }
	| expr ' ' grp { $$ = makenode(Conc, $1, makenode(Group, $3, nil)); }
%%

typedef union Inst Inst;
union Inst
{
	int op;
	int n;
	Segment seg;
	SegSet *set;
};

void
parseerror(Scanner *scn, char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	verror(1, s, ap);
	va_end(ap);
	longjmp(scn->err, 1);
}

void
yyerror(void *scanner, char *s)
{
	parseerror((Scanner*)scanner, s);
}

static Node*
makenode(int type, Node *left, Node *right)
{
	Node *n;
	n = malloc(sizeof(Node));
	n->type = type;
	n->left = left;
	n->right = right;
	return n;
}

static Node*
makeseg(Segment seg)
{
	Node *n;
	n = malloc(sizeof(Node));
	n->type = Segm;
	n->seg = seg;
	return n;
}

static Node*
makeset(int type, int num, int *types, Segment *segs)
{
	Node *n;
	n = malloc(sizeof(Node));
	n->type = type;
	n->set = makesegset(num, types, segs);
	return n;
}

Segment
parsebraces(Scanner *scn)
{
	char c;
	char *p, buf[256];

	p = buf;
	while(c = *scn->lexstr++, c && c != '}')
		*p++ = c;
	*p++ = '\0';
	return strtoseg(buf);
}

int
parsegraph(Scanner *scn, Segment *seg)
{
	int n;
	n = alphgetsegfirst(scn->lexalph, --scn->lexstr, seg);
	if(n > 0){
		scn->lexstr += n;
		if(*scn->lexstr == ':'){
			scn->lexstr++;
			*seg = segmod(*seg, Long, 0);
		}
		return 1;
	}
	return 0;
}

int
yylex(YYSTYPE *yylval, void *scanner)
{
	char c;
	int n;
	int type;
	Segment seg;
	Segment setbuf[100];
	int types[100];
	Scanner *scn;

	scn = scanner;
	c = *scn->lexstr++;
	if(c == '\0')
		return 0;
	if(isspace(c)){
		while(isspace(*scn->lexstr)) scn->lexstr++;
		return ' ';
	}
	if(strchr("()*+?|!._", c))
		return c;
	if(c == '#'){
		if(*scn->lexstr == '^'){
			scn->lexstr++;
			return BOUNDL;
		}
		if(*scn->lexstr == '$'){
			scn->lexstr++;
			return BOUNDR;
		}
		return BOUND;
	}

	if(c == '['){
		type = Set;
		if(*scn->lexstr == '!'){
			scn->lexstr++;
			type = NotSet;
		}
		n = 0;
		while(*scn->lexstr && *scn->lexstr != ']'){
			while(isspace(c = *scn->lexstr++));
			if(c == '{'){
				types[n] = Class;
				setbuf[n++] = parsebraces(scn);
			}else if(c == '%' && *scn->lexstr == '{'){
				scn->lexstr++;
				types[n] = Segm;
				setbuf[n++] = parsebraces(scn);
			}else if(parsegraph(scn, &seg)){
				types[n] = Segm;
				setbuf[n++] = seg;
			}else
				parseerror(scn, "no graph %c", c);
		}
		if(*scn->lexstr == ']') scn->lexstr++;
		yylval->node = makeset(type, n, types, setbuf);
		return ATOM;
	}

	if(c == '{'){
		yylval->node = makeseg(parsebraces(scn));
		yylval->node->type = Class;
		return ATOM;
	}
	if(c == '%' && *scn->lexstr == '{'){
		scn->lexstr++;
		yylval->node = makeseg(parsebraces(scn));
		return ATOM;
	}

	if(!parsegraph(scn, &seg))
		parseerror(scn, "no graph %c", c);
	yylval->node = makeseg(seg);
	return ATOM;
}

static void
printnode(Node *n, Alphabet *a)
{
	char buf[256];
	int i;

	printf("(%s", types[n->type]);
	switch(n->type){
	case Segm:
	case Class:
		if(alphgetgraph(a, n->seg, buf) == 0)
			segtostr(buf, n->seg, 0);
		printf(" '%s'", buf);
		break;
	case Set:
	case NotSet:
		// TODO: print correctly
		for(i = 0; i < n->set->n; i++){
			if(alphgetgraph(a, n->set->segs[i], buf) == 0)
				segtostr(buf, n->set->segs[i], 0);
			printf(" '%s'", buf);
		}
		break;
	case Star:
	case Plus:
	case Quest:
	case Group:
		printf(" ");
		printnode(n->left, a);
		break;
	case Conc:
	case Alt:
		printf(" ");
		printnode(n->left, a);
		printf(" ");
		printnode(n->right, a);
		break;
	}
	printf(")");
}

typedef struct Legex Legex;
struct Legex
{
	Inst *code;
	int nop;
	int ninst;
	int nsub;
};

#define JUMP(_pc, _l1)\
	l->code[(_pc)].op = Jump;\
	l->code[(_pc)+1].n = (_l1);

#define SAVE(_n)\
	l->code[l->ninst++].op = Save;\
	l->code[l->ninst++].n = (_n);

#define BRANCH(_pc, _l1, _l2)\
	l->code[(_pc)].op = Branch;\
	l->code[(_pc)+1].n = (_l1);\
	l->code[(_pc)+2].n = (_l2);

/* Compile a node and free its memory */
static void
compnode(Legex *l, Node *n)
{
	int l1, l2;
	int k;

	switch(n->type){
	case Bol:
	case Eol:
	case Bound:
	case Dot:
	case Empty:
		l->nop++;
		l->code[l->ninst++].op = n->type;
		break;
	case Class:
	case Segm:
		l->nop++;
		l->code[l->ninst++].op = n->type;
		l->code[l->ninst++].seg = n->seg;
		break;
	case Set:
	case NotSet:
		l->nop++;
		l->code[l->ninst++].op = n->type;
		l->code[l->ninst++].set = n->set;
		break;

	case Group:
		l->nop += 2;
		k = l->nsub++;
		SAVE(2*k);
		compnode(l, n->left);
		SAVE(2*k+1);
		break;

	case Conc:
		compnode(l, n->left);
		compnode(l, n->right);
		break;
	case Alt:
		l->nop += 2;
		l->ninst += 3;	// branch l1, l2
		l1 = l->ninst;
		compnode(l, n->left);
		l->ninst += 2;	// jump l3
		l2 = l->ninst;
		compnode(l, n->right);
		BRANCH(l1-3, l1, l2);
		JUMP(l2-2, l->ninst);
		break;
	case Star:
		l->nop += 2;
		l1 = l->ninst;
		l->ninst += 3;	// branch l2, l3
		l2 = l->ninst;
		compnode(l, n->left);
		JUMP(l->ninst, l1);
		l->ninst += 2;
		BRANCH(l1, l2, l->ninst);
		break;
	case Plus:
		l->nop++;
		l1 = l->ninst;
		compnode(l, n->left);
		BRANCH(l->ninst, l1, l->ninst + 3);
		l->ninst += 3;
		break;
	case Quest:
		l->nop++;
		l->ninst += 3;
		l1 = l->ninst;
		compnode(l, n->left);
		BRANCH(l1-3, l1, l->ninst);
		break;
	default:
		printf("unknown type %d\n", n->type);
		break;
	}
	free(n);
}

static Legex*
compile(Node *n)
{
	Legex *l;
	Inst *code, codebuf[1024];

	l = malloc(sizeof(Legex));
	l->code = codebuf;
	l->nop = 0;
	l->ninst = 0;
	l->nsub = 1;

	/* hardcoded non-greedy .* */
	l->nop += 3;
	BRANCH(l->ninst, 6, 3);
	l->ninst += 3;
	l->code[l->ninst++].op = Dot;
	JUMP(l->ninst, 0);
	l->ninst += 2;

	l->nop += 3;
	SAVE(0);
	compnode(l, n);
	SAVE(1);
	l->code[l->ninst++].op = End;

	code = malloc(l->ninst*sizeof(Inst));
	memcpy(code, l->code, l->ninst*sizeof(Inst));
	l->code = code;
	return l;
}

static void
disasm(Alphabet *a, Legex *l)
{
	char buf[256];
	Inst *i;
	int j;
	int pc;

	pc = 0;
	for(pc = 0; pc < l->ninst; pc++){
		i = &l->code[pc];
		printf("%02x: %s ", pc, types[i->op]);
		switch(i->op){
		case Bol:
		case Eol:
		case Bound:
		case Dot:
		case Empty:
		case End:
			printf("\n");
			break;
		case Segm:
		case Class:
			pc++; i++;
			if(alphgetgraph(a, i->seg, buf) == 0)
				segtostr(buf, i->seg, 0);
			printf("'%s'\n", buf);
			break;
		case NotSet:
		case Set:
			pc++; i++;
			for(j = 0; j < i->set->n; j++){
				if(alphgetgraph(a, i->set->segs[j], buf) == 0)
					segtostr(buf, i->set->segs[j], 0);
				if(*buf == '{' && i->set->types[j] == Segm)
					printf(" '%%%s'", buf);
				else
					printf(" '%s'", buf);
			}
			printf("\n");
			break;
		case Save:
			pc++; i++;
			printf("%d\n", i->n);
			break;
		case Jump:
			pc++; i++;
			printf("0x%x\n", i->n);
			break;
		case Branch:
			pc++; i++;
			printf("0x%x", i->n);
			pc++; i++;
			printf(", 0x%x\n", i->n);
			break;
		}
	}
	printf("nop: %d\n", l->nop);
}

int dotrace;

Legex*
lxparse(Alphabet *a, char *s)
{
	Scanner scn;
	Legex *l;
	scn.lexstr = s;
	scn.lexalph = a;
	if(setjmp(scn.err)){
		printf("Error\n");
		return nil;
	}
	yyparse(&scn);
	if(dotrace){
		printnode(result, a);
		printf("\n");
	}
	l = compile(result);
	if(dotrace)
		disasm(a, l);
	return l;
}

/*
 * VM implementation
 */

static int
setindex(SegSet *set, Segment s)
{
	int i;
	for(i = 0; i < set->n; i++){
		if(set->types[i] == Segm && SAMESEG(s, set->segs[i]))
			return i;
		if(set->types[i] == Class && MATCHCLASS(s, set->segs[i]))
			return i;
	}
	return -1;
}

/* Threads are manually allocated by threadlist and legexmatch.
 * index has len(word) elements to track matched set indices */
typedef struct Thread Thread;
struct Thread
{
	Inst *pc;
	int saved[MAXSUB*2];
	int *index;
};


static void
threadcopy(Thread *new, Thread *old, Inst *pc)
{
	new->pc = pc;
	memcpy(new->saved, old->saved, sizeof(int[MAXSUB*2]));
	memcpy(new->index, old->index, old->index[0]*sizeof(int));
}

typedef struct Threadlist Threadlist;
struct Threadlist
{
	int n;
	Thread *list;
};

/* Create a list of n threads to execute on words len segments long */
static Threadlist*
threadlist(int n, int len)
{
	Threadlist *tl;
	int *indices;
	int i;

	tl = malloc(sizeof(Threadlist) + n*sizeof(Thread) + n*(len+1)*sizeof(int));
	tl->n = 0;
	tl->list = (Thread*)(tl+1);
	indices = (int*)(tl->list+n);
	for(i = 0; i < n; i++)
		tl->list[i].index = &indices[(len+1)*i];
	return tl;
};

/* Add a new thread and follow jumps etc.
 * Save changes to the thread on the stack to avoid
 * allocating more threads. */
static void
addthread(Threadlist *tl, Legex *l, Thread *th, int pos)
{
	int i;
	Inst *savedpc;
	int savedpos;

	switch(th->pc->op){
	case Branch:
		savedpc = th->pc;
		th->pc = l->code+savedpc[1].n;
		addthread(tl, l, th, pos);
		th->pc = l->code+savedpc[2].n;
		addthread(tl, l, th, pos);
		th->pc = savedpc;
		return;
	case Jump:
		savedpc = th->pc;
		th->pc = l->code+savedpc[1].n;
		addthread(tl, l, th, pos);
		th->pc = savedpc;
		return;
	case Save:
		savedpc = th->pc;
		savedpos = th->saved[savedpc[1].n];
		th->saved[savedpc[1].n] = pos;
		th->pc = savedpc+2;
		addthread(tl, l, th, pos);
		th->saved[savedpc[1].n] = savedpos;
		th->pc = savedpc;
		return;
	}
	for(i = 0; i < tl->n; i++)
		if(tl->list[i].pc == th->pc)
			return;
	threadcopy(&tl->list[tl->n++], th, th->pc);
	if(dotrace)
		printf("-> 0x%x %s\n", (int)(th->pc-l->code), types[th->pc->op]);
}

int
lxmatch(Alphabet *a, Legex *l, Segment *s, int *submatches, int *index)
{
	Threadlist *clist, *nlist, *tmp;
	Thread th, *tp;
	int i, j;
	int x;
	int len;
	int matched;
	char buf[20];

	len = wordlen(s);

	th.index = malloc((len+1)*sizeof(int));
	clist = threadlist(l->nop, len);
	nlist = threadlist(l->nop, len);

	for(i = 0; i < MAXSUB*2; i++){
		th.saved[i] = -1;
		submatches[i] = -1;
	}
	th.index[0] = len+1;
	for(i = 0; i < len; i++)
		th.index[i+1] = -1;
	th.pc = l->code;

	matched = 0;
	addthread(clist, l, &th, 0);
	for(i = 0; i < len+1 && clist->n > 0; i++){
		if(dotrace){
			if(s[i].spec == Eof)
				printf("---- Eof\n");
			else{
				alphgetgraph(a, s[i], buf);
				printf("---- %s\n", buf);
			}
		}

		for(j = 0; j < clist->n; j++){
			tp = &clist->list[j];
			switch(tp->pc->op){
			case Dot:
				threadcopy(&th, tp, tp->pc+1);
				addthread(nlist, l, &th, i+1);
				break;
			case Empty:
				threadcopy(&th, tp, tp->pc+1);
				addthread(clist, l, &th, i);
				break;
			case Segm:
				if(SAMESEG(s[i], tp->pc[1].seg)){
					if(dotrace){
						alphgetgraph(a, tp->pc[1].seg, buf);
						printf("match! %s\n", buf);
					}
					threadcopy(&th, tp, tp->pc+2);
					addthread(nlist, l, &th, i+1);
				}
				break;
			case Class:
				if(MATCHCLASS(s[i], tp->pc[1].seg)){
					threadcopy(&th, tp, tp->pc+2);
					addthread(nlist, l, &th, i+1);
				}
				break;
			case Set:
				if((x = setindex(tp->pc[1].set, s[i])) >= 0){
					threadcopy(&th, tp, tp->pc+2);
					th.index[i+1] = x;
					addthread(nlist, l, &th, i+1);
				}
				break;
			case NotSet:
				if((x = setindex(tp->pc[1].set, s[i])) < 0){
					threadcopy(&th, tp, tp->pc+2);
					th.index[i+1] = x;
					addthread(nlist, l, &th, i+1);
				}
				break;
			case Bol:
				if(i == 0){
					threadcopy(&th, tp, tp->pc+1);
					addthread(clist, l, &th, i);
				}
				break;
			case Eol:
				if(s[i].spec == Eof){
					threadcopy(&th, tp, tp->pc+1);
					addthread(clist, l, &th, i);
				}
				break;
			case Bound:
				if(i == 0 || s[i].spec == Eof){
					threadcopy(&th, tp, tp->pc+1);
					addthread(clist, l, &th, i);
				}
				break;
			case End:
				if(submatches)
					memcpy(submatches, tp->saved, sizeof(int[MAXSUB*2]));
				if(index)
					memcpy(index, tp->index, (len+1)*sizeof(int));
				matched = 1;
				clist->n = 0;
				break;
			default:
				assert(0);
			}
		}

		tmp = nlist;
		nlist = clist;
		clist = tmp;
		nlist->n = 0;
	}
	free(clist);
	free(nlist);
	free(th.index);
	return matched;
}

void
lxfree(Legex *l)
{
	int pc;

	pc = 0;
	for(pc = 0; pc < l->ninst; pc++)
		switch(l->code[pc].op){
		case Segm:
		case Class:
		case Save:
		case Jump:
			pc++;
			break;
		case Branch:
			pc += 2;
			break;
		case NotSet:
		case Set:
			pc++;
			free(l->code[pc].set);
			break;
		}
	free(l->code);
	free(l);
}
