/*
 * Bully Election Algorithm — single-file synchronous simulation
 *
 * Rules:
 *   - Processes have unique IDs 1..N. Higher ID = higher priority.
 *   - When a process notices the coordinator is gone, it starts an election:
 *       1. Send ELECTION to every process with a higher ID.
 *       2. If any higher process is alive it replies OK and takes over.
 *       3. If none reply, the initiator declares itself COORDINATOR
 *          and broadcasts to everyone.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define MAX  10

/* ── Colours ───────────────────────────────────────────────────────── */
#define R    "\033[0m"
#define BOLD "\033[1m"
#define RED  "\033[31m"
#define GRN  "\033[32m"
#define YEL  "\033[33m"
#define CYN  "\033[36m"
#define MAG  "\033[35m"

/* ── State ─────────────────────────────────────────────────────────── */
typedef enum { UP, DOWN } Status;

static Status procs[MAX + 1];   /* procs[id], 1-based; procs[0] unused */
static int    n;                /* number of processes                  */
static int    coordinator;      /* current coordinator id, -1 = none    */

/* ── Print cluster state ───────────────────────────────────────────── */
static void print_state(void)
{
    printf("\n  ");
    for (int id = 1; id <= n; id++) {
        if (procs[id] == UP)
            printf(GRN "P%d✓" R "  ", id);
        else
            printf(RED "P%d✗" R "  ", id);
    }
    if (coordinator == -1)
        printf(" coord: " YEL "none" R "\n\n");
    else
        printf(" coord: " MAG BOLD "P%d" R "\n\n", coordinator);
}

/* ── Election (returns new coordinator id) ─────────────────────────── */
static int elect(int initiator)
{
    printf(CYN "  P%d" R CYN " → ELECTION" R, initiator);

    /* Send to every higher process */
    for (int id = initiator + 1; id <= n; id++) {
        if (procs[id] == UP) {
            printf(CYN " → P%d" R, id);
            printf(GRN " (OK)" R "\n");
            /* Higher process takes over — recurse */
            return elect(id);
        } else {
            printf(" → P%d" RED " (no reply)" R, id);
        }
    }

    /* No higher process alive — this process wins */
    printf("\n");
    coordinator = initiator;
    printf(MAG BOLD "  ★ P%d wins — broadcasting COORDINATOR\n" R, initiator);
    return initiator;
}

/* ── Scenario helpers ──────────────────────────────────────────────── */
static void fail(int id)
{
    procs[id] = DOWN;
    if (id == coordinator) coordinator = -1;
    printf(RED "  P%d failed%s\n" R, id,
           id == coordinator ? " (was coordinator)" : "");
}

static void recover(int id)
{
    procs[id] = UP;
    printf(GRN "  P%d recovered → starts election\n" R, id);
    elect(id);
}

static void notice_coordinator_gone(int detector)
{
    printf(YEL "  P%d notices coordinator is gone → starts election\n" R, detector);
    elect(detector);
}

/* ── Init ──────────────────────────────────────────────────────────── */
static void init(int count)
{
    n = count;
    coordinator = -1;
    for (int id = 1; id <= n; id++) procs[id] = UP;
    printf(BOLD "=== Bully Election — synchronous simulation ===\n" R);
    printf("  %d processes (P1–P%d), all " GRN "UP" R "\n", n, n);
}

/* ── Help text ─────────────────────────────────────────────────────── */
static void print_help(void)
{
    printf(BOLD "  Commands:\n" R);
    printf("    fail    <id>          — bring a process down\n");
    printf("    recover <id>          — bring a process back up (triggers election)\n");
    printf("    detect  <detector>    — process notices coordinator is gone, starts election\n");
    printf("    elect   <initiator>   — manually kick off an election from a process\n");
    printf("    state                 — print current cluster state\n");
    printf("    reset                 — re-initialise with the same N\n");
    printf("    help                  — show this list\n");
    printf("    quit                  — exit\n\n");
}

/* ── Main ──────────────────────────────────────────────────────────── */
int main(void)
{
    /* ── Ask for number of nodes ──────────────────────────────────── */
    printf(BOLD "=== Bully Election — interactive simulation ===\n\n" R);
    printf("  Number of processes (2-%d): ", MAX);
    if (scanf("%d", &n) != 1 || n < 2 || n > MAX) {
        fprintf(stderr, RED "  Invalid count.\n" R);
        return 1;
    }
    init(n);
    print_state();
    print_help();

    /* ── REPL ─────────────────────────────────────────────────────── */
    char cmd[32];
    int  arg;

    for (;;) {
        printf(BOLD "> " R);
        fflush(stdout);

        if (scanf("%31s", cmd) != 1) break;   /* EOF */

        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0) {
            break;

        } else if (strcmp(cmd, "state") == 0) {
            print_state();

        } else if (strcmp(cmd, "reset") == 0) {
            init(n);
            print_state();

        } else if (strcmp(cmd, "help") == 0) {
            print_help();

        } else if (strcmp(cmd, "fail") == 0) {
            if (scanf("%d", &arg) != 1) { printf("  Usage: fail <id>\n"); continue; }
            if (arg < 1 || arg > n) { printf(RED "  ID out of range (1-%d)\n" R, n); continue; }
            if (procs[arg] == DOWN) { printf(YEL "  P%d is already down\n" R, arg); continue; }
            fail(arg);
            print_state();

        } else if (strcmp(cmd, "recover") == 0) {
            if (scanf("%d", &arg) != 1) { printf("  Usage: recover <id>\n"); continue; }
            if (arg < 1 || arg > n) { printf(RED "  ID out of range (1-%d)\n" R, n); continue; }
            if (procs[arg] == UP) { printf(YEL "  P%d is already up\n" R, arg); continue; }
            recover(arg);
            print_state();

        } else if (strcmp(cmd, "detect") == 0) {
            if (scanf("%d", &arg) != 1) { printf("  Usage: detect <id>\n"); continue; }
            if (arg < 1 || arg > n) { printf(RED "  ID out of range (1-%d)\n" R, n); continue; }
            if (procs[arg] == DOWN) { printf(RED "  P%d is down — can't detect anything\n" R, arg); continue; }
            if (coordinator != -1) { printf(YEL "  There is a coordinator (P%d). Use 'fail %d' first if you want to simulate it going away.\n" R, coordinator, coordinator); continue; }
            notice_coordinator_gone(arg);
            print_state();

        } else if (strcmp(cmd, "elect") == 0) {
            if (scanf("%d", &arg) != 1) { printf("  Usage: elect <id>\n"); continue; }
            if (arg < 1 || arg > n) { printf(RED "  ID out of range (1-%d)\n" R, n); continue; }
            if (procs[arg] == DOWN) { printf(RED "  P%d is down — can't start an election\n" R, arg); continue; }
            printf(YEL "  P%d manually starts an election\n" R, arg);
            elect(arg);
            print_state();

        } else {
            printf("  Unknown command '%s'. Type 'help' for options.\n", cmd);
        }
    }

    printf(BOLD "\n=== bye ===\n" R);
    return 0;
}
