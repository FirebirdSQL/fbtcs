#ifndef TCS_PROTO_H
#define TCS_PROTO_H

// External functions declarations
char* handle_sys_env(char*);

char* handle_where_gdb(char*);
char* handle_symbol(char*);

void print_error(char*, char*, char*, char*);

char** split_tokens(char*, const char*);
ULONG get_elapsed_time(struct tm);
char* read_file(FILE* pFile);

#endif // TCS_PROTO_H

