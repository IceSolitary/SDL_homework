#include<Windows.h>

int lib_func(char *msg) {
	MessageBox(NULL, msg, "nya", MB_OK);
	return 0;
}