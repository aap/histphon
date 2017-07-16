#include "legex.h"

int
main()
{
	FILE *f;
	Language *langtree;
	Script *sc;

	f = fopen("treetest", "r");
	assert(f);
	langtree = readlangtree(f, nil);
	printlangtree(langtree, 0);

	sc = scriptparse(stdin, langtree);
	printdefs(sc);

	legscrcompile(sc, "idg", "att");

	return 0;
}
