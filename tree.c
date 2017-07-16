#include "legex.h"

static void printind(int i) { while(i--) printf("  "); }

Language*
makelang(char *name)
{
	Language *l;
	l = malloc(sizeof(Language));
	l->name = strdup(name);
	l->parent = nil;
	l->child = nil;
	l->next = nil;
	return l;
}

void
langsetparent(Language *par, Language *child)
{
	Language **p;
	assert(child->parent == nil);
	child->parent = par;
	for(p = &par->child; *p; p = &(*p)->next);
	*p = child;
}

void
printlangtree(Language *root, int indent)
{
	Language *child;
	printind(indent);
	printf("%s\n", root->name);
	for(child = root->child; child; child = child->next)
		printlangtree(child, indent+1);
}

int peekind = -1;
int iseof;

static int
getind(FILE *f)
{
	int c, n;
	n = 0;
	if(peekind >= 0){
		n = peekind;
		peekind = -1;
		return n;
	}
	while(c = getc(f), c == ' ')
		n++;
	if(c == EOF)
		return -1;
	ungetc(c, f);
	return n;
}

Language*
readlangtree(FILE *f, Language *parent)
{
	int c;
	char langbuf[100], *p;
	int level, ind;
	Language *lang;

	lang = nil;
	level = peekind = getind(f);
	while((ind = getind(f)) >= level){
		if(ind == level){
			p = langbuf;
			while(c = getc(f), c != EOF && c != ' ' && c != '\n')
				*p++ = c;
			ungetc(c, f);
			while(c = getc(f), c != EOF && c != '\n');
			*p = '\0';
			if(parent == nil && lang)
				return lang;
			lang = makelang(langbuf);
			if(parent)
				langsetparent(parent, lang);
		}else{
			assert(lang != nil);
			peekind = ind;
			readlangtree(f, lang);
		}
	}
	peekind = ind;
	return lang;
}
