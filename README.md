## Synopsis

Logcus offers a similar functionalities as syslog. Logcus appends a timestamp and the pid of message sender to the log message, which is desired to be written in a log file. With logcus, the log writing should be easy. There are only three different functions for user to use.

## Installation & How to use

Simply do the following:
- Include "logcus.h" to the .c-file where you want to use logcus.
- Add the logcus.h and logcus.c to the project directory.
- Link logcus with the project by adding it to the makefile's sources.
------------------

I have included a main.c and test.c which demonstrates the use and behaviour of logcus. To test, first you need to make the executables. This can be done simply by writing "make" to bash in projects root (or src). Now you should have main and test executables. Test executable offers some tests, wherease main is very simple program demostrating the use of logcus in its simplest form. Notice that main stops only with sigint signal (Ctrl ^C).

Notice that makefile includes also other useful functionalities. 'Make clean' deletes the executables and clears the log.txt


## Example function calls:
Simply run the main executable in src folder. You should give file path(s) to code file(s) that you want to be cleaned. It is also possible to give some option parameters, but currently the program simply skips these.

open_logcus();  
logcus("Example message1\n");  
logcus("I can be formatted in similar way as %s or %s\n", "fprintf", "syslog");  
logcus("I take variable amount of arguments\n");  
logcus("My weakness is that I currently write only to one log file\n");  
logcus("REMEMBER ALWAYS TO CLOSE LOGCUS AFTER USE!\n");  
close_logcus();  
