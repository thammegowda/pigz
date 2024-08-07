#ifndef PIGZ_HEADER_H
#define PIGZ_HEADER_H

// https://stackoverflow.com/a/12066342/1506477
// https://stackoverflow.com/q/1041866/1506477
#ifdef __cplusplus
extern "C"{
#endif

int pigz_call(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif // PIGZ_HEADER_H