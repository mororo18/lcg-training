#ifndef __COMMOM_STATE_
#define __COMMOM_STATE_

typedef
struct Fulano {
    int a;
    float b;
    char c;
} Fulano;

typedef
struct CommomState {
    Fulano fulano[10];
} CommomState;

#endif // __COMMOM_STATE_
