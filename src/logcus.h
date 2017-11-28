int logcus(char *str);
int init_logcus();
int close_logcus();
int check_logcus();
void sigint_handler(int sig);
void sigterm_handler(int sig);
unsigned * open_shared();
unsigned * create_shared();
