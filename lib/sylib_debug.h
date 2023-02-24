#ifndef __SYLIB_DEBUG_H_
#define __SYLIB_DEBUG_H_

/* Input & output functions */
int getint(), getch(), getarray(int a[]);
float getfloat();
int getfarray(float a[]);
void putint(int a), putch(int a), putarray(int n, int a[]);
void putfloat(float a);
void putfarray(int n, float a[]);
void putf(char a[], ...);

#endif
