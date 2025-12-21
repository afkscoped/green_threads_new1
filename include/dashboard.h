#ifndef DASHBOARD_H
#define DASHBOARD_H

/* Starts the dashboard server on the specified port in a new green thread.
   Returns the thread handle or NULL on failure. */
void dashboard_start(int port);

#endif
