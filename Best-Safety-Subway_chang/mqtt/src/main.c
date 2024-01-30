/*
--------------------------The MIT License---------------------------
Copyright (c) 2013 Qi Wang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <signal.h>
#include "mqtts.h"
#include "event.h"

void clearup(int signum)
{
   printf("Caught signal %d\n",signum);
   exit(signum);
}

int main(int argc, char *argv[])
{

    int s;
    int sfd;
    int efd;

    struct addrinfo *res;
    struct epoll_event event;
    struct epoll_event* events;
    mqtts_t* mqtts;

    if (argc != 2)
    {
        fprintf (stderr, "Usage: %s [port]\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    if (sfd == -1)
        abort ();

    mqtts = init_mqtts(mqtts);

    sfd = init_net(res, argv[1]);
    efd = init_event(sfd, &event, events);
    events = calloc (MQTTS_MAXEVENTS, sizeof (event));

    mqtts->efd = efd;
    mqtts->sfd = sfd;
    mqtts->events = events;
    mqtts->event = &event;

    loop(mqtts);
}
