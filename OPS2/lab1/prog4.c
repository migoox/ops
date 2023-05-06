#include <asm-generic/errno-base.h>
#include <linux/limits.h>
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MSG_SIZE 16
#define MAX_POINTS 5
#define MAX_CARDS 10
#define MAX_PLAYERS 5

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s N, M\n", name);
	fprintf(stderr, "2=<N<=5 - number of players\n");
	fprintf(stderr, "5=<M<=10 - number of cards\n");
	exit(EXIT_FAILURE);
}

int set_handler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

void create_players_and_pipes(int players_count, int* pipes_out, int* pipes_in, int cards_count);
void player_work(int pipe_out, int pipe_in, int cards_count);
void server_work(int players_count, int* pipes_out, int* pipes_in, int rounds_count);
void wait_children_with_suspend();
// not used
void wait_children();

int main(int argc, char **argv)
{
    // input arguments
    if (3 != argc)
        usage(argv[0]);
    int players_count = atoi(argv[1]), cards_count = atoi(argv[2]);
    if (!(players_count >= 2 && players_count <= 5)
        || !(cards_count >= 5 && cards_count <= 10)) {
            usage(argv[0]);
    }

    if (set_handler(SIG_IGN, SIGPIPE) < 0)
        ERR("setting signal handler failed");

    int pipes_out[MAX_PLAYERS];
    int pipes_in[MAX_PLAYERS];

    create_players_and_pipes(players_count, pipes_out, pipes_in, cards_count);
    server_work(players_count, pipes_out, pipes_in, cards_count);

    // close descriptors
    for (int i = 0; i < players_count; ++i) {
        if (TEMP_FAILURE_RETRY(close(pipes_in[i]))
            || TEMP_FAILURE_RETRY(close(pipes_out[i]))) {
                ERR("(parent) close failed");
        }
    }

    // wait for children
    wait_children_with_suspend();

    return EXIT_SUCCESS;
}

void create_players_and_pipes(int players_count, int* pipes_out, int* pipes_in, int cards_count)
{
    for (int i = 0; i < players_count; ++i) {
        // player to server
        int pts_fds[2];
        if (pipe(pts_fds) < 0)
            ERR("pipe creation failed");
        pipes_out[i] = pts_fds[0];

        // server to player
        int stp_fds[2];
        if (pipe(stp_fds) < 0)
            ERR("pipe creation failed");
        pipes_in[i] = stp_fds[1];

        // fork
        pid_t child_id = fork();
        if (child_id == 0) {
            // clear all of the fds previously opened by the parent   
            for (int j = 0; j <= i; ++j) {
                if (TEMP_FAILURE_RETRY(close(pipes_in[j]))
                    || TEMP_FAILURE_RETRY(close(pipes_out[j]))) {
                        ERR("(child) close failed");
                }
            }

            // save pipes
            int pipe_out = stp_fds[0], pipe_in = pts_fds[1];

            // start work
            player_work(pipe_out, pipe_in, cards_count);

            // close used pipes
            if (TEMP_FAILURE_RETRY(close(pipe_out))
                || TEMP_FAILURE_RETRY(close(pipe_in))) {
                    ERR("(child) close failed");
            }

            // exit 
            exit(EXIT_SUCCESS);
        } else if (child_id < 0) {
            ERR("fork failed");
        }

        // clear fds given to the child
        if (TEMP_FAILURE_RETRY(close(pts_fds[1]))
            || TEMP_FAILURE_RETRY(close(stp_fds[0]))) {
            ERR("(parent) close failed");
        }
    }
}

void player_work(int pipe_out, int pipe_in, int cards_count)
{
    // printf("player with pid: %d\n", getpid());
    srand(getpid() * time(NULL));

    char buffer[MSG_SIZE];
    int cards_sent[MAX_CARDS];
    memset(cards_sent, 0, cards_count);

    for(;;) {
        // 5 % to fail
        if (rand() % 20 == 0) {
            printf("player with pid %d has disconnected\n", getpid());
            break;
        }

        int count = 0;
        if ((count = TEMP_FAILURE_RETRY(read(pipe_out, buffer, MSG_SIZE))) < 0) {
            ERR("(child) read failed");
        }

        if (0 == count) { // EOF
            break;
        }

        // send a random card
        int card = rand() % cards_count;
        while (cards_sent[card] == 1) {
            card = (card + 1) % cards_count;
        }
        cards_sent[card] = 1;
        printf("player [pid: %d] sends a card: %d\n", getpid(), card);

        *((int*)buffer) = card;
        memset(buffer + sizeof(int), 0, MSG_SIZE - sizeof(int));
        if ((count = TEMP_FAILURE_RETRY(write(pipe_in, buffer, MSG_SIZE))) < 0)
            ERR("(child) write failed");
    }
}

void server_work(int players_count, int* pipes_out, int* pipes_in, int rounds_count)
{
    // printf("server with pid: %d\n", getpid());
    srand(getpid() * time(NULL));

    char new_round_buffer[MSG_SIZE] = "new_round";
    char buffer[MSG_SIZE];

    // tracks points collected by players
    int players_points[MAX_PLAYERS];
    memset(players_points, 0, players_count * sizeof(int));

    int players_cards[MAX_PLAYERS];

    // tracks connected players
    int players_active[MAX_PLAYERS];
    memset(players_active, 1, players_count * sizeof(int));

    for (int i = 0; i < rounds_count; ++i) {
        printf("NEW ROUND\n");
        // send information about a new round to each player
        for (int j = 0; j < players_count; ++j) {
            if (players_active[j] == 0) continue;

            if (TEMP_FAILURE_RETRY(write(pipes_in[j], new_round_buffer, MSG_SIZE)) < 0) {
                if (errno != EPIPE)
                    ERR("(parent) write failed");
                players_active[j] = 0;
                players_cards[j] = 0;
            }
        }

        // get responses
        for (int j = 0; j < players_count; ++j) {
            if (players_active[j] == 0) continue;

            int count = 0;
            if ((count = TEMP_FAILURE_RETRY(read(pipes_out[j], buffer, MSG_SIZE))) < 0)
                ERR("(parent) read failed");
            
            if (0 == count) { // EOF
                players_active[j] = 0;
                players_cards[j] = 0;
                continue;
            }
            players_cards[j]= *((int*)buffer);

            printf("round: %d, server receives a card: %d\n", i, *((int*)buffer));
        }

        // find max card
        int max_card = -1;
        int occurences = 0;
        for (int j = 0; j < players_count; ++j) {
            if (max_card < players_cards[j]) {
                max_card = players_cards[j];
                occurences = 1;
            } else if (max_card == players_cards[j]) {
                ++occurences;
            }
        }

        // give points
        for (int j = 0; j < players_count; ++j) {
            if (players_cards[j] == max_card) {
                players_points[j] += (int)(MAX_POINTS / occurences);
            }
        }
    }

    printf("POINTS:\n");
    for (int j = 0; j < players_count; ++j) {
        printf("player %d: %d\n", j + 1, players_points[j]);
    }
}

void wait_children_with_suspend()
{
    for (;;) {
        pid_t chld_pid = wait(NULL);

        if (-1 == chld_pid && ECHILD == errno) { // no children to wait for
            break;
        } else if (-1 == chld_pid) { // error
            ERR("waiting failed"); 
        }
    }
}

void wait_children()
{
    // we use loop, since there might be more than one child to be waited
    for (;;) {
        // we use WNOHANG option, so the parent won't be suspended
        // if there are no children to wait for
        // every child inherited its process group from parent
        // so using 0 as pid will work
        pid_t chld_pid = waitpid(0, NULL, WNOHANG);

        if (chld_pid == 0) { // no children statuses available
            return;
        } else if (-1 == chld_pid && ECHILD == errno) { // no children exist
            return;
        } else if (-1 == chld_pid){ // error
            ERR("waiting failed"); 
        }
    }
}