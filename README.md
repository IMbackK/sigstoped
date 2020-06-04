# sigstoped
a user deamon that stops processes associated with background windows
uses _NET_ACTIVE_WINDOW _NET_WM_PID WM_CLIENT_MACHINE and a blacklist to determine whitch processes to stop via SIGSTOP upon window swtich

usage:

1. build
2. create ~/.config/sigstoped/blacklist
  1. one proces name per line, nothing else
3. run

