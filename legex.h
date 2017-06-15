#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <ctype.h>
#include <assert.h>


#define nil NULL
#define nelem(a) (sizeof(a)/sizeof(a[0]))

typedef uint32_t u32;

typedef struct Legex Legex;
enum { MAXSUB = 10, WORDLEN = 256 };

typedef struct Feature Feature;
struct Feature
{
	char *name;
	u32 bit;
	int priv;	// not used (yet?)
};

typedef struct Segment Segment;
struct Segment
{
	u32 feat;
	u32 spec;
};

#define SAMESEG(s1, s2) (((s1).feat & (s1).spec) == ((s2).feat & (s2).spec))
/* check whether s1 has all features s2 has and has none of the features s2 doesn't */
#define MATCHCLASS(s1, s2) (((s1).feat & (s1).spec & (s2).spec) == ((s2).feat & (s2).spec) && \
	(~(s1).feat & (s1).spec & (s2).spec) == (~(s2).feat & (s2).spec))

typedef struct Alphabet Alphabet;
struct Alphabet
{
	struct Letter {
		char *graph;
		int len;
		Segment segm;
	} *letters;
	int len, cap;
};

enum
{
	/* major class */
	Cons	= 1<<0,
	Son	= 1<<1,
	Appr	= 1<<2,
	Syll	= 1<<3,
	/* laryngeal */
	Voi	= 1<<4,
	Sg	= 1<<5,
	Cg	= 1<<6,
	/* manner */
	Cont	= 1<<7,
	Nas	= 1<<8,
	Lat	= 1<<9,
	Strid	= 1<<10,
	DRel	= 1<<11,
	/* place */
	Lab	= 1<<12,
	 Rnd	= 1<<13,
	Cor	= 1<<14,
	 Ant	= 1<<15,
	 Dist	= 1<<16,
	Dor	= 1<<17,
	 High	= 1<<18,
	 Low	= 1<<19,
	 Back	= 1<<20,
	 Tense	= 1<<21,
	Phar	= 1<<22,
	Long	= 1<<23,

	Eof	= 1U<<31,	// '\0' marker

	majorMask = Cons | Son | Appr | Syll,
	laryMask = Voi | Sg | Cg,
	mannerMask = Cont | Nas | Lat | Strid | DRel,
	labMask = Rnd,
	corMask = Ant | Dist,
	dorMask = High | Low | Back | Tense,
	defMask = majorMask | laryMask | mannerMask,
};

/* Used for both legex lexing and subst parsing */
typedef struct Scanner Scanner;
struct Scanner
{
	char *lexstr;
	Alphabet *lexalph;
	jmp_buf err;
};

/* A set of segments of different types. Used for matching to match
 * Segments and Classes, for subsitution for replacement and modification. */
typedef struct SegSet SegSet;
struct SegSet
{
	int n;
	int *types;
	Segment *segs;
};

/* TODO: this is temporary, ideally we want more sophisticated substitutions */
enum SubstType
{
	SxDel,	/* delete group */
	SxKeep,	/* keep group */
	SxLit,	/* replace with literal word */
	SxMod,	/* modify features */
	SxSet,	/* replace from set */
};

typedef struct Subst Subst;
struct Subst
{
	int type;
	int n;	/* -1: current group; else: group n */
	union {
		Segment *lit;
		Segment mod;
		SegSet *set;
	} u;
};

void error(int e, char *fmt, ...);
void verror(int e, char *fmt, va_list ap);

Feature *findfeat(char *name);

/* Segment */
Segment segment(u32 feat, u32 nofeat);
char *segtostr(char *s, Segment seg, int pad);
Segment strtoseg(char *s);
Segment segmod(Segment s, u32 add, u32 rem);
Segment segmodseg(Segment s, Segment mod);

/* Alphabet */
Alphabet *newalphabet(void);
void freealphabet(Alphabet *a);
void alphdump(Alphabet *a);
void alphadd(Alphabet *a, char *gr, Segment seg);
void alphrem(Alphabet *a, char *gr);
void alphclear(Alphabet *a);
Segment alphgetseg(Alphabet *a, char *gr);
int alphgetsegfirst(Alphabet *a, char *gr, Segment *seg);
int alphgetgraph(Alphabet *a, Segment s, char *gr);

/* Words and strings */
Segment *makeword(Alphabet *a, char *s, Segment *w);
int wordlen(Segment *w);
Segment *wordcpy(Segment *dst, Segment *src);
Segment *worddup(Segment *w);
Segment *subword(Segment *dst, Segment *src, int s, int e);
void worddump(Segment *w);

void makestring(Alphabet *a, Segment *w, char *buf);

/* Parser */
void parseerror(Scanner *scn, char *s, ...);
Segment parsebraces(Scanner *scn);
int parsegraph(Scanner *scn, Segment *seg);
SegSet *makesegset(int n, int *types, Segment *segs);

/* Legex */
Legex *lxparse(Alphabet *a, char *s);
int lxmatch(Alphabet *a, Legex *l, Segment *s, int *submatches, int *index);
void lxfree(Legex *l);
int printmatches(Alphabet *a, Segment *word, int *submatches);

/* Subst */
void sxparse(Alphabet *a, char *s, Subst *subst);
void sxfree(Subst *subst);
void substitute(Segment *out, Segment *in, int *submatches, int *index, Subst *subst);

int runtests(void);
