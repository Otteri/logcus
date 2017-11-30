int open_logcus(void);
void closeall(void);
int create_daemon(void);
void errexit(const char *str);
void errreport(const char *str);
void process(void);
unsigned * create_shared_variable(const char *name);
unsigned * open_shared_variable(const char *name);
int close_logcus(void);
