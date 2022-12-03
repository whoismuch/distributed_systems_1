

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "ipc.h"
#include "pa1.h"
#include "common.h"


int send(void *self, local_id dst, const Message *msg) {
    if (msg == NULL) return -1;
    write(((int *) self)[dst], msg, sizeof(MessageHeader) + msg->s_header.s_payload_len);
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

    read(((int *) self)[from], &(msg->s_header), sizeof(MessageHeader));
    read(((int *) self)[from], msg->s_payload, msg->s_header.s_payload_len);

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

int close_not_my_pipes(int process_number, int pipes_to_read[process_number + 2][process_number + 2],
                       int pipes_to_write[process_number + 2][process_number + 2], local_id pid) {
    for (int i = 0; i <= process_number; i++) {
        if (i != pid) {
            for (int j = 0; j <= process_number; j++) {
                if (i != j) {
                    if (close(pipes_to_read[i][j]) != 0) return -1;
                    if (close(pipes_to_write[i][j]) != 0) return -1;
                    pipes_to_read[i][j] = -999;
                    pipes_to_write[i][j] = -999;
                }
            }
        }
    }
    return 0;
}

int send_msg(int process_number, int pipes_to_write[process_number + 2][process_number + 2], int pid, const char *msg,
             MessageType messageType) {

    MessageHeader message_header = {
            .s_magic = MESSAGE_MAGIC,
            .s_payload_len = strlen(msg),
            .s_type = messageType,
            .s_local_time = time(NULL)
    };

    Message message = {
            .s_header = message_header,
            .s_payload = {0}
    };

    strcpy(message.s_payload, msg);

    return send_multicast(pipes_to_write[pid], &message);

}

int receive_msg(int process_number, int pipes_to_read[process_number + 2][process_number + 2], int pid, FILE *p_log) {
    for (int j = 1; j <= process_number; j++) {
        if (pid != j) {
            Message message;
            memset(&message, 0, sizeof (Message));
            receive(pipes_to_read[pid], j, &message);
            fprintf(p_log, "I read message from %d process channels descriptor message %s\n", j,
                    message.s_payload);

        }
    }
    return 0;
}

int main(int argc, char *argv[]) {

    if (0 != strcmp(argv[1], "-p")) {
        return 1;
    }

    int process_number = atoi(argv[2]);

    FILE *p_log;
    p_log = fopen(pipes_log, "wa+");
    if (p_log == NULL) { return 1; }

    FILE *e_log;
    e_log = fopen(events_log, "wa+");
    if (e_log == NULL) { return 1; }


    int pipes_to_read[process_number + 2][process_number + 2];
    int pipes_to_write[process_number + 2][process_number + 2];

    for (int i = 0; i <= process_number; i++) {
        for (int j = 0; j <= process_number; j++) {
            int fdPipes[2];
            if (i != j) {
                pipe(fdPipes);
                pipes_to_read[j][i] = fdPipes[0];
                pipes_to_write[i][j] = fdPipes[1];
            } else {
                pipes_to_read[j][i] = -999;
                pipes_to_write[i][j] = -999;
            }
        }
        pipes_to_read[i][process_number + 1] = -1;
        pipes_to_write[i][process_number + 1] = -1;
        pipes_to_read[process_number + 1][i] = -1;
        pipes_to_write[process_number + 1][i] = -1;
    }


    for (int i = 1; i <= process_number; i++) // loop will run n times (n=5)
    {
        pid_t child_pid = fork();
        if (child_pid == 0) {
            if (close_not_my_pipes(process_number, pipes_to_read, pipes_to_write, i) != 0) return -1;

            printf(log_started_fmt, i, getpid(), getppid());

            fprintf(e_log, log_started_fmt, i, getpid(), getppid());

            char *message_content = (char *) malloc(100 * sizeof(char));
            sprintf(message_content, log_started_fmt, i, getpid(), getppid());

            send_msg(process_number, pipes_to_write, i, message_content, STARTED);

            fprintf(p_log, "I wrote in all channels descriptor\n");

            receive_msg(process_number, pipes_to_read, i, p_log);

            printf(log_received_all_started_fmt, i);
            fprintf(e_log, log_received_all_started_fmt, i);

            printf(log_done_fmt, i);
            fprintf(e_log, log_done_fmt, i);

            char *message_content2 = (char *) malloc(100 * sizeof(char));
            sprintf(message_content2, log_done_fmt, i);

            send_msg(process_number, pipes_to_write, i, message_content2, DONE);

            fprintf(p_log, "I wrote in all channels descriptors\n");

            receive_msg(process_number, pipes_to_read, i, p_log);

            printf(log_received_all_done_fmt, i);
            fprintf(e_log, log_received_all_done_fmt, i);


            free(message_content);
            free(message_content2);
            exit(0);
        }

    }

    close_not_my_pipes(process_number, pipes_to_read, pipes_to_write, 0);
    receive_msg(process_number, pipes_to_read, 0, p_log);
    receive_msg(process_number, pipes_to_read, 0, p_log);


    for (int i = 1; i <= process_number; i++) // loop will run n times (n=5)
        wait(NULL);

}

