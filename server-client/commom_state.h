#ifndef __COMMOM_STATE_
#define __COMMOM_STATE_

typedef
struct Fulano {
    int a;
    float b;
    char c;
} Fulano;

#define FULANO_CAPACITY 5

typedef
struct CommomState {
    Fulano fulano[FULANO_CAPACITY];
} CommomState;

#endif // __COMMOM_STATE_
