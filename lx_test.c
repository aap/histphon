#include "legex.h"

#define MOD(gr, base, feat, nofeat) alphadd(alph, gr, segmod(base, feat, nofeat));
#define GR(s) alphgetseg(alph, s)

int
runtest(Alphabet *alph, char *slegex, char *sword, int res, int *submatches)
{
	Segment word[WORDLEN];
	Legex *l;
	int ret;

	makeword(alph, sword, word);
	l = lxparse(alph, slegex);
	ret = lxmatch(alph, l, word, submatches, nil);
	lxfree(l);
	return ret != res;
}

int
runtests(void)
{
	Alphabet *alph;
	Segment schwa, con, son, tmp;
	int submatches[MAXSUB*2];
	char *slegex, *sword;

	alph = newalphabet();
	schwa = segment(Son|Syll|Lab|Dor|Phar|Voi|Cont,
	                    Cons|Rnd|Cor|High|Low|Back|Tense|Sg|Cg|Strid|Lat|DRel|Nas|Long);
	MOD("a", schwa, Low, 0);
	MOD("ā", GR("a"), Long, 0);
	MOD("e", schwa, Tense, 0);
	MOD("ē", GR("e"), Long, 0);
	MOD("o", schwa, Tense|Back|Rnd, 0);
	MOD("ō", GR("o"), Long, 0);
	MOD("i", schwa, High|Tense, 0);
	MOD("ī", GR("i"), Long, 0);
	MOD("u", schwa, High|Tense|Back|Rnd, 0);
	MOD("ū", GR("u"), Long, 0);

	con = segment(Cons, Son|Syll|Long);
	MOD("p", con, Lab, Rnd|Cor|Dor|Phar|Voi|Sg|Cg|Cont|Strid|Lat|DRel|Nas);
	MOD("t", con, Cor|Ant, Lab|Dist|Dor|Phar|Voi|Sg|Cg|Cont|Strid|Lat|DRel|Nas);
	MOD("c", con, Dor|High|Back, Lab|Cor|Low|Tense|Phar|Voi|Sg|Cg|Cont|Strid|Lat|DRel|Nas);
	MOD("b", GR("p"), Voi, 0);
	MOD("d", GR("t"), Voi, 0);
	MOD("g", GR("c"), Voi, 0);
	MOD("f", GR("p"), Cont|Strid, 0);
	MOD("s", GR("t"), Cont|Strid, 0);
	tmp = segment(Voi|Sg|Cont, Cons|Son|Syll|Lab|Cor|Dor|Phar|Voi|Cg|Strid|Lat|DRel|Nas|Long);
	alphadd(alph, "h", tmp);

	son = segment(Cons|Son|Voi, Syll|Phar|Sg|Cg|Long);
	MOD("r", son, Cor|Ant|Cont, Lab|Dor|Dist|Strid|Lat|DRel|Nas);
	MOD("l", son, Cor|Ant|Cont|Lat, Lab|Dor|Dist|Strid|DRel|Nas);
	MOD("m", son, Lab|Nas, Rnd|Cor|Dor|Cont|Strid|Lat|DRel);
	MOD("n", son, Cor|Ant|Nas, Lab|Dist|Cor|Dor|Cont|Strid|Lat|DRel);

#define TEST(_legex, _word, _ret)\
	slegex = _legex;\
	sword = _word;\
	if(runtest(alph, _legex, _word, _ret, submatches)){ \
		fprintf(stderr, "test failed: <%s> <%s> %d\n", _legex, _word, _ret); \
		return 1; \
	}
#define SUB(_n, _s, _e)\
	if(submatches[_n*2] != _s || submatches[_n*2+1] != _e){ \
		fprintf(stderr, "submatch failed: %s %s %d %d\n", slegex, sword, submatches[_n*2], submatches[_n*2+1]); \
	}

	TEST("aab", "aab", 1);
	SUB(0, 0, 3); SUB(1, 0, 3);
	TEST("aab", "faabf", 1);
	TEST("#^aab", "aaabf", 0);
	TEST("#aab", "aaabf", 0);
	TEST("aab#$", "faabb", 0);
	TEST("aab#", "faabb", 0);
	TEST("aab#$", "fafaab", 1);
	TEST("aab#", "fafaab", 1);
	TEST("#^aab", "aabfaf", 1);
	TEST("#aab", "aabfaf", 1);
	TEST("aab|eef", "uuuaabuuu", 1);
	TEST("aab|eef", "uuueefuuu", 1);
	TEST("a(ab|ee)f", "hhaabfhh", 1);
	SUB(0, 2, 6); SUB(1, 2, 6);
	TEST("a(ab|ee)f", "hhaeefhh", 1);

	TEST("aab dde", "aabdde", 1);
	SUB(1, 0, 3); SUB(2, 3, 6);

	TEST("ff[abcde]hh", "ffahh", 1);
	TEST("ff[abcde]hh", "ffchh", 1);
	TEST("ff[abcde]hh", "ffehh", 1);
	TEST("ff[abcde]hh", "ffphh", 0);
	TEST("ff[abcde]hh", "fflhh", 0);
	TEST("ff[abcde]hh", "ffmhh", 0);
	TEST("ff[!abcde]hh", "ffahh", 0);
	TEST("ff[!abcde]hh", "ffchh", 0);
	TEST("ff[!abcde]hh", "ffehh", 0);
	TEST("ff[!abcde]hh", "ffphh", 1);
	TEST("ff[!abcde]hh", "fflhh", 1);
	TEST("ff[!abcde]hh", "ffmhh", 1);

	TEST("[p{+son +syll}]", "p", 1);
	TEST("[p{+son +syll}]", "a", 1);
	TEST("[p{+son +syll}]", "o", 1);

	TEST("a*b", "b", 1);
	SUB(1, 0, 1);
	TEST("a*b", "ab", 1);
	SUB(1, 0, 2);
	TEST("a*b", "aaaaaab", 1);
	SUB(1, 0, 7);

	TEST("a+b", "b", 0);
	TEST("a+b", "ab", 1);
	SUB(1, 0, 2);
	TEST("a+b", "aaaaaab", 1);
	SUB(1, 0, 7);

	TEST("a?b", "b", 1);
	SUB(1, 0, 1);
	TEST("a?b", "ab", 1);
	SUB(1, 0, 2);
	TEST("#a?b", "aaaaaab", 0);

	freealphabet(alph);

	return 0;
}

extern int dotrace;

int
main()
{
	Alphabet *alph;
	Segment schwa, con, son, tmp;
	Segment word[WORDLEN], segbuf[WORDLEN];
	int index[WORDLEN];
	Subst subst[MAXSUB];
	char buf[WORDLEN];
	Legex *l;
	int submatches[MAXSUB*2];
	int i;
	int ret;

	alph = newalphabet();
	schwa = segment(Son|Syll|Lab|Dor|Phar|Voi|Cont,
	                    Cons|Rnd|Cor|High|Low|Back|Tense|Sg|Cg|Strid|Lat|DRel|Nas|Long);
	MOD("a", schwa, Low, 0);
	MOD("ā", GR("a"), Long, 0);
	MOD("e", schwa, Tense, 0);
	MOD("ē", GR("e"), Long, 0);
	MOD("o", schwa, Tense|Back|Rnd, 0);
	MOD("ō", GR("o"), Long, 0);
	MOD("i", schwa, High|Tense, 0);
	MOD("ī", GR("i"), Long, 0);
	MOD("u", schwa, High|Tense|Back|Rnd, 0);
	MOD("ū", GR("u"), Long, 0);

	con = segment(Cons, Son|Syll|Long);
	MOD("p", con, Lab, Rnd|Cor|Dor|Phar|Voi|Sg|Cg|Cont|Strid|Lat|DRel|Nas);
	MOD("t", con, Cor|Ant, Lab|Dist|Dor|Phar|Voi|Sg|Cg|Cont|Strid|Lat|DRel|Nas);
	MOD("c", con, Dor|High|Back, Lab|Cor|Low|Tense|Phar|Voi|Sg|Cg|Cont|Strid|Lat|DRel|Nas);
	MOD("b", GR("p"), Voi, 0);
	MOD("d", GR("t"), Voi, 0);
	MOD("g", GR("c"), Voi, 0);
	MOD("f", GR("p"), Cont|Strid, 0);
	MOD("s", GR("t"), Cont|Strid, 0);
	tmp = segment(Voi|Sg|Cont, Cons|Son|Syll|Lab|Cor|Dor|Phar|Voi|Cg|Strid|Lat|DRel|Nas|Long);
	alphadd(alph, "h", tmp);

	son = segment(Cons|Son|Voi, Syll|Phar|Sg|Cg|Long);
	MOD("r", son, Cor|Ant|Cont, Lab|Dor|Dist|Strid|Lat|DRel|Nas);
	MOD("l", son, Cor|Ant|Cont|Lat, Lab|Dor|Dist|Strid|DRel|Nas);
	MOD("m", son, Lab|Nas, Rnd|Cor|Dor|Cont|Strid|Lat|DRel);
	MOD("n", son, Cor|Ant|Nas, Lab|Dist|Cor|Dor|Cont|Strid|Lat|DRel);

//	alphdump(alph);

	if(runtests()){
		fprintf(stderr, "Tests failed\n");
		return 1;
	}

	dotrace = 1;

	for(i = 0; i < MAXSUB; i++){
		subst[i].type = SxKeep;
		subst[i].n = -1;
	}
/*
	makeword(alph, "ffaabeehh", word);
	l = legexparse(alph, "aa _ bee");
	subst[1].type = SxDel;
	subst[2].type = SxLit;
	subst[2].u.lit = makeword_a(alph, "sss");
	subst[3].n = 1;
*/
/*
	makeword(alph, "aeo", word);
	l = legexparse(alph, "[aeo] . [aeo]");
//	l = legexparse(alph, "{+son +syll} {+son +syll} {+son +syll}");
	subst[1].type = SxMod;
	subst[1].u.mod = segment(Dor|Tense, 0);
	subst[3].type = SxSet;
	sxset = makesegset(3, nil, nil);
	sxset->types[0] = SxLit;
	sxset->segs[0] = GR("p");
	sxset->types[1] = SxLit;
	sxset->segs[1] = GR("c");
	sxset->types[2] = SxMod;
	sxset->segs[2] = segment(Long, 0);
	subst[3].u.set = sxset;
*/

	makeword(alph, "aeo", word);
	l = lxparse(alph, "[aeo] _ [aeo] .");
	sxparse(alph, "c 3/[ptc] _ 1/{+long}", subst);

	ret = lxmatch(alph, l, word, submatches, index);
	printf("-> %d\n", ret);
	for(i = 0; i < MAXSUB; i++)
		printf("%d %d\n", submatches[i*2], submatches[i*2+1]);
	if(ret){
		printmatches(alph, word, submatches);
		for(i = 0; i < index[0]-1; i++)
			printf("%d ", index[i+1]);
		printf("\n");
		substitute(segbuf, word, submatches, index, subst);
		makestring(alph, segbuf, buf);
		printf("%s\n", buf);
	}
	lxfree(l);
	sxfree(subst);

	freealphabet(alph);

	return 0;
}
