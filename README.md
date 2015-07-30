Building a Lisp derivative through C following a book by Daniel Holden: http://buildyourownlisp.com/

# Compiling on Windows
cc -std=c99 -Wall lissp.c mpc.c -o lissp

# Compiling on linux
cc -std=c99 -Wall lissp.c mpc.c -ledit -lm -o lissp

Editline library required for linux.
