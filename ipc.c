#include "ipc.h"
#include <unistd.h>

int send(void *self, local_id dst, const Message *msg) {
    if (msg == NULL) return -1;
    if (write(((int *) self)[dst], msg, sizeof(*msg)) != 0) return -1;
    return 0;
}

int send_multicast(void *self, const Message *msg) {
    local_id i = 0;
    while (((int *) self)[i] != -1) {
        send(self, i, msg);
        i++;
    }
    return 0;
}

int receive(void *self, local_id from, Message *msg) {
    if (msg == NULL) return -1;
    read(((int *) self)[from], msg, sizeof(*msg));
    return 0;
}

int receive_any(void *self, Message *msg) {
    local_id i = 0;
    while (((int *) self)[i] != -1) {
        if (receive(self, i, msg) != 0) return -1;
        i++;
    }
    return 0;
}
