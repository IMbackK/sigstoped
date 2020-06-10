# sigstoped
A user session deamon that stops processes associated with background X11 windows.
Uses _NET_ACTIVE_WINDOW _NET_WM_PID WM_CLIENT_MACHINE and a blacklist to determine whitch processes to stop via SIGSTOP upon window swtich.

Sigstoped uses no background cpu time and only runs when the window manager is shuffling windows.

usage:

1. build
2. create ~/.config/sigstoped/blacklist
  1. one proces name per line, nothing else
3. run

