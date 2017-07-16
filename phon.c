#include "legex.h"

Feature features[] = {
	{ "cons",	Cons,	0 },
	{ "son",	Son,	0 },
	{ "appr",	Appr,	0 },
	{ "syll",	Syll,	0 },

	{ "voi",	Voi,	0 },
	{ "SG",		Sg,	0 },
	{ "CG",		Cg,	0 },

	{ "cont",	Cont,	0 },
	{ "nas",	Nas,	0 },
	{ "lat",	Lat,	0 },
	{ "strid",	Strid,	0 },
	{ "drel",	DRel,	0 },

	{ "lab",	Lab,	1 },
	{ "rnd",	Rnd,	0 },

	{ "cor",	Cor,	1 },
	{ "ant",	Ant,	0 },
	{ "dist",	Dist,	0 },

	{ "dor",	Dor,	1 },
	{ "high",	High,	0 },
	{ "low",	Low,	0 },
	{ "back",	Back,	0 },
	{ "tense",	Tense,	0 },

	{ "phar",	Phar,	1 },

	{ "long",	Long,	0 },
};

void
verror(int e, char *fmt, va_list ap)
{
	fprintf(stderr, e ? "error: " : "warning: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
}

void
error(int e, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verror(e, fmt, ap);
	va_end(ap);
}

Feature*
findfeat(char *name)
{
	Feature *f;
	for(f = features; f < &features[nelem(features)]; f++)
		if(strcmp(f->name, name) == 0)
			return f;
	return nil;
}

Segment
segment(u32 feat, u32 nofeat)
{
	Segment s;
	s = (Segment){ feat & ~nofeat, feat | nofeat };
	if((s.spec & s.feat & Lab) == 0)
		s.spec &= ~labMask;
	if((s.spec & s.feat & Cor) == 0)
		s.spec &= ~corMask;
	if((s.spec & s.feat & Dor) == 0)
		s.spec &= ~dorMask;
	s.feat &= s.spec;
	return s;
}

char*
segtostr(char *s, Segment seg, int pad)
{
	char *t;
	unsigned int i;
	int j;
	int bit;

	t = s;
	*t++ = '{';
	*t++ = ' ';
	for(i = 0; i < nelem(features); i++){
		bit = 1<<i;
		if(seg.spec & bit){
			*t++ = seg.feat & bit ? '+' : '-';
			t += sprintf(t, "%s ", features[i].name);
		}else if(pad)
			for(j = strlen(features[i].name)+2; j; j--)
				*t++ = ' ';
	}
	*t++ = '}';
	*t++ = '\0';
	return s;
}

Segment
strtoseg(char *s)
{
	char *p, buf[128];
	Feature *fp;
	u32 feat, nofeat;

	feat = 0;
	nofeat = 0;
	while(isspace(*s)) s++;
	while(*s){
		p = buf;
		while(*s && !isspace(*s)) *p++ = *s++;
		*p = '\0';
		while(isspace(*s)) s++;
		if(*buf != '+' && *buf != '-')
			continue;
		fp = findfeat(buf+1);
		if(fp == nil)
			continue;
		if(*buf == '+')
			feat |= fp->bit;
		else
			nofeat |= fp->bit;
	}
	return segment(feat, nofeat);
}

Segment
segmod(Segment s, u32 add, u32 rem)
{
	u32 feat, spec;
	feat = s.spec & s.feat & ~rem | add;
	spec = s.spec | add | rem;
	if(add & High)
		feat &= ~Low;
	if(add & Low)
		feat &= ~High;
	return segment(feat, ~feat & spec);
}

Segment
segmodseg(Segment s, Segment mod)
{
	return segmod(s, mod.feat&mod.spec, ~mod.feat&mod.spec);
}

Alphabet*
newalphabet(void)
{
	Alphabet *a;
	a = malloc(sizeof(Alphabet));
	a->len = 0;
	a->cap = 1;
	a->letters = malloc(a->cap*sizeof(struct Letter));
	return a;
}

void
freealphabet(Alphabet *a)
{
	alphclear(a);
	free(a->letters);
	free(a);
}

void
alphdump(Alphabet *a)
{
	char namebuf[256];
	struct Letter *l;
	for(l = a->letters; l < &a->letters[a->len]; l++)
		printf("%s %d %s\n", l->graph, l->len, segtostr(namebuf, l->segm, 1));
}

/* Add a grapheme to the alphabet, keep sorted by length for lookup */
void
alphadd(Alphabet *a, char *gr, Segment seg)
{
	int i, j, l;

	l = strlen(gr);
	for(i = 0; i < a->len; i++){
		if(a->letters[i].len > l)
			continue;
		if(a->letters[i].len < l)
			break;
		if(strcmp(a->letters[i].graph, gr) == 0){
			a->letters[i].segm = seg;
			return;
		}
	}

	if(a->cap <= a->len){
		a->cap *= 2;
		a->letters = realloc(a->letters, a->cap*sizeof(struct Letter));
	}

	for(j = a->len; j > i; j--)
		a->letters[j] = a->letters[j-1];
	a->len++;
	a->letters[i].graph = strdup(gr);
	a->letters[i].len = l;
	a->letters[i].segm = seg;
}

/* Remove one grapheme from the alphabet */
void
alphrem(Alphabet *a, char *gr)
{
	int i;
	for(i = 0; i < a->len; i++)
		if(strcmp(a->letters[i].graph, gr) == 0){
			free(a->letters[i].graph);
			for(; i < a->len-1; i++)
				a->letters[i] = a->letters[i+1];
			a->len--;
			return;
		}
}

/* Remove all graphemes from the alphabet */
void
alphclear(Alphabet *a)
{
	int i;
	for(i = 0; i < a->len; i++)
		free(a->letters[i].graph);
	a->len = 0;
}

/* Look up a segment by grapheme */
Segment
alphgetseg(Alphabet *a, char *gr)
{
	struct Letter *l;
	Segment s = { 0, 0 };
	for(l = a->letters; l < &a->letters[a->len]; l++)
		if(strcmp(l->graph, gr) == 0)
			return l->segm;
	return s;
}

/* Find longest segment at beginning of a string, return length or 0 */
int
alphgetsegfirst(Alphabet *a, char *gr, Segment *seg)
{
	struct Letter *l;
	for(l = a->letters; l < &a->letters[a->len]; l++)
		if(strncmp(l->graph, gr, l->len) == 0){
			*seg = l->segm;
			return l->len;
		}
	return 0;
}

int
alphgetgraph(Alphabet *a, Segment s, char *gr)
{
	struct Letter *l;
	int len;
	len = 0;
	for(l = a->letters; l < &a->letters[a->len]; l++)
		if((l->segm.feat&l->segm.spec) == (s.feat&s.spec)){
			strcpy(gr, l->graph);
			return l->len;
		}else if((l->segm.feat&l->segm.spec) == (s.feat&s.spec&~Long)){
			strcpy(gr, l->graph);
			len = l->len;
			gr[len++] = ':';
			gr[len] = '\0';
		}
	return len;
}

Segment*
makeword(Alphabet *a, char *s, Segment *w)
{
	int n;
	Segment seg, *ret;
	ret = w;
	while(*s != '\0'){
		n = alphgetsegfirst(a, s, &seg);
		if(n > 0){
			*w++ = seg;
			s += n;
		}else{
			if(*s == ':')
				w[-1] = segmod(w[-1], Long, 0);
			s++;
		}
	}
	w->spec = Eof;
	return ret;
}

Segment*
makeword_a(Alphabet *a, char *s)
{
	Segment buf[WORDLEN];
	makeword(a, s, buf);
	return worddup(buf);
}

int
wordlen(Segment *w)
{
	int i;
	i = 0;
	while(w[i].spec != Eof)
		i++;
	return i;
}

Segment*
wordcpy(Segment *dst, Segment *src)
{
	while(src->spec != Eof)
		*dst++ = *src++;
	dst->spec = Eof;
	return dst;
}

Segment*
worddup(Segment *w)
{
	Segment *dst;
	int len;
	len = wordlen(w)+1;
	dst = malloc(len*sizeof(Segment));
	memcpy(dst, w, len*sizeof(Segment));
	return dst;
}

Segment*
subword(Segment *dst, Segment *src, int s, int e)
{
	int i;
	for(i = s; i < e; i++)
		*dst++ = src[i];
	dst->spec = Eof;
	return dst;
}

void
worddump(Segment *w)
{
	int i;
	char namebuf[256];
	for(i = 0; w[i].spec != Eof; i++){
		segtostr(namebuf, w[i], 1);
		printf("%s\n", namebuf);
	}
}

void
makestring(Alphabet *a, Segment *w, char *buf)
{
	char grph[20];
	char *p;
	int len;

	p = buf;
	*p = '\0';
	for(; w[0].spec != Eof; w++){
		len = alphgetgraph(a, *w, grph);
		if(len > 0){
			strcpy(p, grph);
			p += len;
		}else
			*p++ = '*';
	}
	*p = '\0';
}

