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

/* ── Scenario header ───────────────────────────────────────────────── */
static void scenario(int num, const char *desc)
{
    printf(BOLD YEL "\n[%d] %s\n" R, num, desc);
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

/* ── Main ──────────────────────────────────────────────────────────── */
int main(void)
{
    init(5);

    scenario(1, "Initial election — P1 detects no coordinator");
    notice_coordinator_gone(1);   /* P5 should win */
    print_state();

    scenario(2, "Coordinator (P5) fails — P2 detects it");
    fail(5);
    notice_coordinator_gone(2);   /* P4 should win */
    print_state();

    scenario(3, "P4 and P3 fail — P1 detects it");
    fail(4);
    fail(3);
    notice_coordinator_gone(1);   /* P2 should win */
    print_state();

    scenario(4, "P5 recovers — bullies its way to the top");
    recover(5);                   /* P5 should win */
    print_state();

    scenario(5, "Everyone fails except P1 — P1 wins");
    fail(5);
    fail(4);
    fail(3);
    fail(2);
    notice_coordinator_gone(1);   /* P1 should win */
    print_state();

    printf(BOLD "=== done ===\n" R);
    return 0;
}
