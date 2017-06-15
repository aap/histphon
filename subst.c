#include "legex.h"

/* Modify all segments by mod */
static Segment*
submod(Segment *dst, Segment *src, int s, int e, Segment mod)
{
	int i;
	for(i = s; i < e; i++)
		*dst++ = segmodseg(src[i], mod);
	dst->spec = Eof;
	return dst;
}

/* Substitute segments that matched a class by its counterpart in a subsitution set.
 * This mean either literal replacement or modification as above. */
static Segment*
subset(Segment *dst, Segment *src, int s, int e, SegSet *set, int *indices)
{
	int i, x;
	for(i = s; i < e; i++){
		x = indices[i+1];
		if(x >= 0 && x < set->n){
			if(set->types[x] == SxLit)
				*dst++ = set->segs[x];
			else if(set->types[x] == SxMod)
				*dst++ = segmodseg(src[i], set->segs[x]);
			else
				assert(0);
		}else{
			fprintf(stderr, "Warning: set index out of range: %d\n", x);
			*dst++ = src[i];
		}
	}
	dst->spec = Eof;
	return dst;
}

SegSet*
makesegset(int n, int *types, Segment *segs)
{
	SegSet *s;
	// TODO: check alignment
	s = malloc(sizeof(SegSet)+n*(sizeof(Segment)+sizeof(int)));
	s->n = n;
	s->segs = (Segment*)(s+1);
	s->types = (int*)(s->segs+n);
	if(types)
		memcpy(s->types, types, n*sizeof(int));
	if(segs)
		memcpy(s->segs, segs, n*sizeof(Segment));
	return s;
}

static char *sxtypes[] = {
	"SxDel", "SxKeep", "SxLit", "SxMod", "SxSet"
};

static void
sxparse1(Scanner *scn, Subst *sub)
{
	long n;
	char c, *s;
	Segment word[WORDLEN], *w;
	Segment seg;
	Segment setbuf[100];
	int types[100];

	sub->type = SxKeep;
	s = scn->lexstr;
	c = *scn->lexstr++;

	if(c == '_'){
		if((c = *scn->lexstr))
			error(0, "ignoring unexpected '%c' after '_'", c);
		goto out;
	}
	if(c == '@'){
		if((c = *scn->lexstr))
			error(0, "ignoring unexpected '%c' after '@'", c);
		sub->type = SxDel;
		goto out;
	}

	if(isdigit(c)){
		scn->lexstr--;
		n = strtol(scn->lexstr, &scn->lexstr, 10);
		sub->n = n;
		c = *scn->lexstr++;
		if(c != '/')
			error(1, "expected '/'");
		else
			c = *scn->lexstr++;
	}

	if(c == '['){
		n = 0;
		while(*scn->lexstr && *scn->lexstr != ']'){
			while(isspace(c = *scn->lexstr++));
			if(c == '{'){
				types[n] = SxMod;
				setbuf[n++] = parsebraces(scn);
			}else if(c == '%' && *scn->lexstr == '{'){
				scn->lexstr++;
				types[n] = SxLit;
				setbuf[n++] = parsebraces(scn);
			}else if(parsegraph(scn, &seg)){
				types[n] = SxLit;
				setbuf[n++] = seg;
			}else
				parseerror(scn, "no graph %c", c);
		}
		sub->type = SxSet;
		sub->u.set = makesegset(n, types, setbuf);
		if((c = *scn->lexstr))
			error(0, "ignoring unexpected '%c' after set", c);
		goto out;
	}

	if(c == '{'){
		sub->type = SxMod;
		sub->u.mod = parsebraces(scn);
		if((c = *scn->lexstr))
			error(0, "ignoring unexpected '%c' after modifier", c);
		goto out;
	}

	scn->lexstr--;
	w = word;
	while((c = *scn->lexstr++)){
		if(c == '%' && *scn->lexstr == '{')
			*w = parsebraces(scn);
		else if(!parsegraph(scn, w))
			parseerror(scn, "no graph %c", c);
		w++;
		sub->type = SxLit;
	}
	w->spec = Eof;
	sub->u.lit = worddup(word);
	//worddump(word);

out:
	printf("%d %s <%s>\n", sub->n, sxtypes[sub->type], s);
}

void
sxparse(Alphabet *a, char *str, Subst *subst)
{
	int i;
	char *s, *e, tmp;
	Scanner scn;

	for(i = 0; i < MAXSUB; i++){
		subst[i].type = SxKeep;
		subst[i].n = i;
	}

	i = 1;
	s = str = strdup(str);
	while(isspace(*s)) s++;
	while(*s && i < MAXSUB){
		e = s;
		while(*e && !isspace(*e)) e++;
		tmp = *e;
		*e = '\0';
		scn.lexstr = s;
		scn.lexalph = a;
		if(setjmp(scn.err))
			fprintf(stderr, "PARSE ERROR\n");
		else
			sxparse1(&scn, &subst[i]);
		*e = tmp;
		s = e;
		while(isspace(*s)) s++;
		i++;
	}
	free(str);
}

void
sxfree(Subst *subst)
{
	int i;
	for(i = 0; i < MAXSUB; i++)
		if(subst[i].type == SxLit)
			free(subst[i].u.lit);
		else if(subst[i].type == SxSet)
			free(subst[i].u.set);
}

void
substitute(Segment *out, Segment *in, int *submatches, int *index, Subst *subst)
{
	int i, n;
	int s, e;

	subword(out, in, 0, submatches[0]);
	out += submatches[0];
	for(i = 1; ; i++){
		if(submatches[i*2] < 0)
			break;
		n = subst[i].n;
		if(n < 0 || submatches[n*2] < 0)
			n = i;
		s = submatches[n*2];
		e = submatches[n*2+1];
		switch(subst[i].type){
		case SxDel:
			break;
		case SxKeep:
			subword(out, in, s, e);
			out += e - s;
			break;
		case SxLit:
			out = wordcpy(out, subst[i].u.lit);
			break;
		case SxMod:
			out = submod(out, in, s, e, subst[i].u.mod);
			break;
		case SxSet:
			out = subset(out, in, s, e, subst[i].u.set, index);
			break;
		}
	}
	subword(out, in, submatches[1], wordlen(in));
}

int
printmatches(Alphabet *a, Segment *word, int *submatches)
{
	Segment segbuf[256];
	char buf[256];
	int i;

	makestring(a, word, buf);
	printf("%s\n", buf);

	subword(segbuf, word, 0, submatches[0]);
	makestring(a, segbuf, buf);
	printf("%s\n", buf);
	for(i = 1; ; i++){
		if(submatches[i*2] < 0)
			break;
		subword(segbuf, word, submatches[i*2], submatches[i*2+1]);
		makestring(a, segbuf, buf);
		printf("%s\n", buf);
	}
	subword(segbuf, word, submatches[1], wordlen(word));
	makestring(a, segbuf, buf);
	printf("%s\n", buf);

	return 0;
}
