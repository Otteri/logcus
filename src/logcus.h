char * concat(const char*, const char*);
char * get_timestamp(void);
char * to_string(int n);

void closeall(void);
int create_daemon(void);
void errexit(const char *str);
void errreport(const char *str);
void process(void);
unsigned * create_shared_variable(const char *name);
unsigned * open_shared_variable(const char *name);

int logcus(char *);
int open_logcus(void);
int close_logcus(void);
