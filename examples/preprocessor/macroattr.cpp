#define WRAP(rettype, name, body) rettype name();

WRAP(void, glBegin, {return;})
