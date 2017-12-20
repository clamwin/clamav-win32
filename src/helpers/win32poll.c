#include <osdeps.h>

#define RegisterWaitForSingleObject cw_helpers.k32.RegisterWaitForSingleObject
#define UnregisterWaitEx cw_helpers.k32.UnregisterWaitEx

/* from compat/net.c */

struct w32polldata {
    HANDLE setme;
    HANDLE event;
    HANDLE waiter;
    struct pollfd *polldata;
};

VOID CALLBACK poll_cb(PVOID param, BOOLEAN timedout) {
    WSANETWORKEVENTS evt;
    struct w32polldata *item = (struct w32polldata *)param;
    if(!timedout) {
	unsigned int i;
	WSAEnumNetworkEvents(item->polldata->fd, item->event, &evt);
	if(evt.lNetworkEvents & FD_ACCEPT) {
	    item->polldata->revents |= POLLIN;
	    if(evt.iErrorCode[FD_ACCEPT_BIT])
		item->polldata->revents = POLLERR;
	}
	if(evt.lNetworkEvents & FD_READ) {
	    item->polldata->revents |= POLLIN;
	    if(evt.iErrorCode[FD_READ_BIT])
		item->polldata->revents = POLLERR;
	}
	if(evt.lNetworkEvents & FD_CLOSE) {
	    item->polldata->revents |= POLLHUP;
	    if(evt.iErrorCode[FD_CLOSE_BIT])
		item->polldata->revents = POLLERR;
	}
	SetEvent(item->setme);
    }
}

int poll_with_event(struct pollfd *fds, int nfds, int timeout, HANDLE event) {
    HANDLE *setme, cankill;
    struct w32polldata *items;
    unsigned int i, ret = 0, reallywait = 1;

    if(timeout <0) timeout = INFINITE;
    if(!nfds) {
	if(event) {
	    if(WaitForSingleObject(event, timeout) == WAIT_OBJECT_0)
		return 1;
	} else
	    Sleep(timeout);
	return 0;
    }
    setme = malloc(2 * sizeof(HANDLE));
    if (setme == NULL) { /* oops, malloc() failed */
	fprintf(stderr, "warning: malloc() for variable 'setme' failed in function 'poll_with_event'...\n");
	return -1;
    }
    setme[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
    setme[1] = event;
    items = malloc(nfds * sizeof(struct w32polldata));
    if (items == NULL) { /* oops, malloc() failed */
	fprintf(stderr, "warning: malloc() for variable 'items' failed in function 'poll_with_event'...\n");
	return -1;
    }
    for(i=0; i<nfds; i++) {
	items[i].polldata = &fds[i];
	items[i].event = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(items[i].event) {
	    items[i].setme = setme[0];
	    if(WSAEventSelect(fds[i].fd, items[i].event, FD_ACCEPT|FD_READ|FD_CLOSE)) {
    		CloseHandle(items[i].event);
		items[i].event = NULL;
	    } else {
		char c; /* Ugly workaround to FD_CLOSE not being persistent
			   better win32 code is possible at the cost of a larger diff vs. the unix
			   netcode - for now it stays ugly...
			*/
		int n = recv(fds[i].fd, &c, 1, MSG_PEEK);
		if(!n)
		    items[i].polldata->revents = POLLHUP;
		if(n == 1)
		    items[i].polldata->revents = POLLIN;
		if(n >= 0 || !RegisterWaitForSingleObject(&items[i].waiter, items[i].event, poll_cb, &items[i], timeout, WT_EXECUTEONLYONCE)) {
		    WSAEventSelect(fds[i].fd, items[i].event, 0);
		    CloseHandle(items[i].event);
		    items[i].event = NULL;
		    reallywait = 0;
		}
	    }
	}
    }
    if(reallywait) {
	if(WaitForMultipleObjects(2 - (event == NULL), setme, FALSE, timeout) == WAIT_OBJECT_0 + 1)
	    ret = 1;
	 else
	    ret = 0;
    }
    cankill = CreateEvent(NULL, TRUE, FALSE, NULL);
    for(i=0; i<nfds; i++) {
	if(items[i].event) {
	    ResetEvent(cankill);
	    UnregisterWaitEx(items[i].waiter, cankill);
	    WSAEventSelect(fds[i].fd, items[i].event, 0);
	    WaitForSingleObject(cankill, INFINITE);
	    CloseHandle(items[i].event);
	}
	ret += (items[i].polldata->revents != 0);
    }
    CloseHandle(cankill);
    free(items);
    CloseHandle(setme[0]);
    free(setme);
    return ret;
}
