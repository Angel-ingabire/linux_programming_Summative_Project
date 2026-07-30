/* =============================================================================
 * Project 5: Building a Concurrent TCP Client-Server Monitoring System
 * =============================================================================
 * File: server.c
 * Description: Central server for a university laboratory equipment booking
 *              system. Handles multiple simultaneous client connections using
 *              POSIX threads for concurrent processing.
 *
 * Features:
 *   - User authentication against registered user IDs
 *   - Equipment listing and reservation management
 *   - Concurrent client handling with thread-safe shared resource access
 *   - Graceful handling of client disconnections
 *
 * Compilation:
 *   gcc -pthread -o server server.c
 *   ./server
 *
 * Communication Protocol:
 *   Text-based messages exchanged over TCP sockets.
 *   Messages are newline-terminated strings.
 *
 *   Server -> Client Messages:
 *     "AUTH_OK"                    - Authentication successful
 *     "AUTH_FAIL"                  - Authentication failed
 *     "EQUIPMENT:<list>"           - Equipment list (tab-separated)
 *     "RESERVED:<equipment>"       - Reservation confirmed
 *     "UNAVAILABLE:<equipment>"    - Equipment already reserved
 *     "ERROR:<message>"            - Error notification
 *     "BYE"                        - Session termination
 *
 *   Client -> Server Messages:
 *     "LOGIN:<user_id>"            - Authentication request
 *     "LIST"                        - Request equipment list
 *     "RESERVE:<equipment>"         - Reservation request
 *     "QUIT"                        - Session termination request
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* ===========================================================================
 * Constants and Configuration
 * ===========================================================================
 */
#define PORT 8080                 /* Server listening port */
#define MAX_CLIENTS 10            /* Maximum simultaneous clients */
#define BUFFER_SIZE 4096          /* Message buffer size */
#define MAX_USER_ID_LEN 32        /* Maximum user ID length */
#define MAX_EQUIPMENT_NAME_LEN 64 /* Maximum equipment name length */
#define MAX_EQUIPMENT 20          /* Maximum equipment items */

/* ===========================================================================
 * Data Structures
 * ===========================================================================
 */

/* Equipment item with availability status */
typedef struct
{
    char name[MAX_EQUIPMENT_NAME_LEN]; /* Equipment name */
    int available;                     /* 1 = available, 0 = reserved */
    char reserved_by[MAX_USER_ID_LEN]; /* User ID who reserved it */
} Equipment;

/* Client session context */
typedef struct
{
    int socket_fd;                 /* Client socket descriptor */
    int authenticated;             /* 1 if authenticated */
    char user_id[MAX_USER_ID_LEN]; /* Authenticated user ID */
} ClientSession;

/* ===========================================================================
 * Global State (Protected by Mutex)
 * ===========================================================================
 */

/* Registered users (simulated database) */
static const char *valid_users[] = {
    "student01", "student02", "student03",
    "researcher01", "researcher02",
    "professor01",
    NULL /* Sentinel */
};

/* Equipment inventory */
static Equipment equipment[MAX_EQUIPMENT];
static int equipment_count = 0;

/* Active client tracking */
static int active_clients = 0;
static char active_users[MAX_CLIENTS][MAX_USER_ID_LEN];

/* Synchronization */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* ===========================================================================
 * Equipment Database Initialization
 * ===========================================================================
 */
static void init_equipment(void)
{
    const char *names[] = {
        "Spectrophotometer",
        "Centrifuge",
        "Microscope_A",
        "Microscope_B",
        "PCR_Thermocycler",
        "Electrophoresis_Unit",
        "pH_Meter",
        "Balance_Analytical",
        "Incubator_A",
        "Incubator_B",
        "Autoclave",
        "Fume_Hood",
        "Shaker_Incubator",
        "Water_Bath",
        "Vortex_Mixer",
        "Magnetic_Stirrer",
        "Spectrofluorometer",
        "HPLC_System",
        "Thermal_Cycler",
        "Glove_Box",
        NULL};

    equipment_count = 0;
    for (int i = 0; names[i] != NULL && i < MAX_EQUIPMENT; i++)
    {
        strncpy(equipment[i].name, names[i], MAX_EQUIPMENT_NAME_LEN - 1);
        equipment[i].available = 1;         /* Initially available */
        equipment[i].reserved_by[0] = '\0'; /* Not reserved */
        equipment_count++;
    }
}

/* ===========================================================================
 * User Authentication
 * ===========================================================================
 * Checks if the provided user ID exists in the registered users list.
 * Returns 1 if valid, 0 if invalid.
 *
 * Thread safety: This function only reads from a const array, so no
 * mutex is needed.
 * ===========================================================================
 */
static int is_valid_user(const char *user_id)
{
    for (int i = 0; valid_users[i] != NULL; i++)
    {
        if (strcmp(valid_users[i], user_id) == 0)
        {
            return 1; /* User found */
        }
    }
    return 0; /* User not found */
}

/* ===========================================================================
 * Build Equipment List String
 * ===========================================================================
 * Constructs a tab-separated string of all equipment with their status.
 * Format: "EQUIPMENT:Spectrophotometer(available)\tCentrifuge(reserved)\t..."
 *
 * Thread safety: Caller must hold mutex when accessing equipment array.
 * ===========================================================================
 */
static void build_equipment_list(char *buffer, size_t buffer_size)
{
    size_t offset = 0;
    int written;

    if (buffer_size == 0)
    {
        return;
    }

    written = snprintf(buffer, buffer_size, "EQUIPMENT:");
    if (written < 0 || (size_t)written >= buffer_size)
    {
        return;
    }
    offset = (size_t)written;

    for (int i = 0; i < equipment_count; i++)
    {
        if (equipment[i].available)
        {
            written = snprintf(buffer + offset, buffer_size - offset,
                               "%s%s (available)",
                               (i > 0) ? "\t" : "",
                               equipment[i].name);
        }
        else
        {
            written = snprintf(buffer + offset, buffer_size - offset,
                               "%s%s (reserved by %s)",
                               (i > 0) ? "\t" : "",
                               equipment[i].name,
                               equipment[i].reserved_by);
        }

        if (written < 0 || (size_t)written >= buffer_size - offset)
        {
            break;
        }
        offset += (size_t)written;
    }
}

/* ===========================================================================
 * Send Message to Client (with error handling)
 * ===========================================================================
 * Wraps send() with error checking and logging.
 * Returns number of bytes sent, or -1 on error.
 * ===========================================================================
 */
static int send_message(int client_fd, const char *message)
{
    ssize_t sent = send(client_fd, message, strlen(message), 0);
    if (sent < 0)
    {
        perror("[SERVER] send() failed");
        return -1;
    }
    /* Send newline terminator separately */
    char newline = '\n';
    send(client_fd, &newline, 1, 0);
    return (int)sent;
}

/* ===========================================================================
 * Handle Client Session
 * ===========================================================================
 * This is the per-client thread function. It manages the complete lifecycle
 * of a client connection: authentication, equipment browsing, reservation,
 * and disconnection.
 *
 * Thread Safety:
 *   - Shared resources (equipment, active_users) accessed only under mutex
 *   - Each client has its own stack and socket descriptor (no sharing)
 * ===========================================================================
 */
static void *handle_client(void *arg)
{
    ClientSession session = {.socket_fd = *(int *)arg, .authenticated = 0};
    free(arg); /* Free the heap-allocated socket descriptor */
    char buffer[BUFFER_SIZE];
    int n;

    printf("[SERVER] New client connected (fd: %d)\n", session.socket_fd);

    /* Process client requests until they disconnect or error occurs */
    while ((n = recv(session.socket_fd, buffer, BUFFER_SIZE - 1, 0)) > 0)
    {
        buffer[n] = '\0'; /* Null-terminate the received data */

        /* Remove trailing newline if present */
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
        {
            buffer[len - 1] = '\0';
        }

        printf("[SERVER] Received from fd %d: %s\n", session.socket_fd, buffer);

        /* ----- Parse and handle the command ----- */

        if (strncmp(buffer, "LOGIN:", 6) == 0)
        {
            /* Extract user ID from LOGIN command */
            char *user_id = buffer + 6;

            /* ----- CRITICAL SECTION (authenticate) ----- */
            pthread_mutex_lock(&mutex);

            if (!is_valid_user(user_id))
            {
                send_message(session.socket_fd, "AUTH_FAIL");
                printf("[SERVER] Authentication FAILED for '%s'\n", user_id);
            }
            else
            {
                /* Check if user already has an active session */
                int already_active = 0;
                for (int i = 0; i < active_clients; i++)
                {
                    if (strcmp(active_users[i], user_id) == 0)
                    {
                        already_active = 1;
                        break;
                    }
                }

                if (already_active)
                {
                    send_message(session.socket_fd, "AUTH_FAIL:already_logged_in");
                    printf("[SERVER] Authentication FAILED for '%s' (already logged in)\n", user_id);
                }
                else
                {
                    session.authenticated = 1;
                    strncpy(session.user_id, user_id, MAX_USER_ID_LEN - 1);
                    strncpy(active_users[active_clients], user_id, MAX_USER_ID_LEN - 1);
                    active_clients++;
                    send_message(session.socket_fd, "AUTH_OK");
                    printf("[SERVER] Authentication SUCCESS for '%s' (active users: %d)\n",
                           user_id, active_clients);
                }
            }
            pthread_mutex_unlock(&mutex);
            /* ----- END CRITICAL SECTION ----- */
        }
        else if (strcmp(buffer, "LIST") == 0)
        {
            if (!session.authenticated)
            {
                send_message(session.socket_fd, "ERROR:Please authenticate first");
                continue;
            }

            /* ----- CRITICAL SECTION (read equipment) ----- */
            pthread_mutex_lock(&mutex);
            char eq_list[BUFFER_SIZE];
            build_equipment_list(eq_list, sizeof(eq_list));
            send_message(session.socket_fd, eq_list);
            pthread_mutex_unlock(&mutex);
            /* ----- END CRITICAL SECTION ----- */
        }
        else if (strncmp(buffer, "RESERVE:", 8) == 0)
        {
            if (!session.authenticated)
            {
                send_message(session.socket_fd, "ERROR:Please authenticate first");
                continue;
            }

            char *equip_name = buffer + 8;
            int found = -1;

            /* ----- CRITICAL SECTION (check and reserve equipment) ----- */
            pthread_mutex_lock(&mutex);

            /* Find the equipment by name (case-sensitive) */
            for (int i = 0; i < equipment_count; i++)
            {
                if (strcmp(equipment[i].name, equip_name) == 0)
                {
                    found = i;
                    break;
                }
            }

            if (found == -1)
            {
                /* Equipment not found in inventory */
                char err_msg[BUFFER_SIZE];
                snprintf(err_msg, sizeof(err_msg), "ERROR:Unknown equipment '%s'", equip_name);
                send_message(session.socket_fd, err_msg);
            }
            else if (equipment[found].available)
            {
                /* Reserve the equipment */
                equipment[found].available = 0;
                strncpy(equipment[found].reserved_by, session.user_id, MAX_USER_ID_LEN - 1);
                char confirm[BUFFER_SIZE];
                snprintf(confirm, sizeof(confirm), "RESERVED:%s", equip_name);
                send_message(session.socket_fd, confirm);
                printf("[SERVER] '%s' reserved '%s' for '%s'\n",
                       session.user_id, equip_name, session.user_id);
            }
            else
            {
                /* Equipment already reserved by someone else */
                char reject[BUFFER_SIZE];
                snprintf(reject, sizeof(reject), "UNAVAILABLE:%s", equip_name);
                send_message(session.socket_fd, reject);
                printf("[SERVER] '%s' REJECTED for '%s' (reserved by %s)\n",
                       equip_name, session.user_id, equipment[found].reserved_by);
            }

            pthread_mutex_unlock(&mutex);
            /* ----- END CRITICAL SECTION ----- */
        }
        else if (strcmp(buffer, "QUIT") == 0)
        {
            printf("[SERVER] Client '%s' requested disconnection\n", session.user_id);
            break;
        }
        else
        {
            /* Unknown command */
            send_message(session.socket_fd, "ERROR:Unknown command");
        }
    }

    /* ----- Client disconnected or error occurred ----- */
    /* Clean up: remove from active users list */
    pthread_mutex_lock(&mutex);

    if (session.authenticated)
    {
        for (int i = 0; i < active_clients; i++)
        {
            if (strcmp(active_users[i], session.user_id) == 0)
            {
                /* Remove by shifting remaining entries */
                for (int j = i; j < active_clients - 1; j++)
                {
                    strncpy(active_users[j], active_users[j + 1], MAX_USER_ID_LEN - 1);
                }
                active_clients--;
                break;
            }
        }

        /* Release any equipment reserved by this user */
        for (int i = 0; i < equipment_count; i++)
        {
            if (!equipment[i].available &&
                strcmp(equipment[i].reserved_by, session.user_id) == 0)
            {
                equipment[i].available = 1;
                equipment[i].reserved_by[0] = '\0';
                printf("[SERVER] Released equipment '%s' reserved by '%s'\n",
                       equipment[i].name, session.user_id);
            }
        }

        printf("[SERVER] Client '%s' disconnected (active users: %d)\n",
               session.user_id, active_clients);
    }
    else
    {
        printf("[SERVER] Unauthenticated client disconnected (fd: %d)\n", session.socket_fd);
    }

    pthread_mutex_unlock(&mutex);

    send_message(session.socket_fd, "BYE");
    close(session.socket_fd);
    return NULL;
}

/* ===========================================================================
 * Display Server Status
 * ===========================================================================
 * Prints current server state to stdout including connected users and
 * equipment reservation status.
 * ===========================================================================
 */
static void display_status(void)
{
    pthread_mutex_lock(&mutex);

    printf("\n=== SERVER STATUS ===\n");
    printf("Active clients: %d\n", active_clients);

    if (active_clients > 0)
    {
        printf("Connected users: ");
        for (int i = 0; i < active_clients; i++)
        {
            printf("%s", active_users[i]);
            if (i < active_clients - 1)
                printf(", ");
        }
        printf("\n");
    }

    printf("\nEquipment Status:\n");
    for (int i = 0; i < equipment_count; i++)
    {
        printf("  %s: ", equipment[i].name);
        if (equipment[i].available)
        {
            printf("Available\n");
        }
        else
        {
            printf("Reserved by %s\n", equipment[i].reserved_by);
        }
    }
    printf("====================\n\n");

    pthread_mutex_unlock(&mutex);
}

/* ===========================================================================
 * Main Server Entry Point
 * ===========================================================================
 * 1. Create TCP socket
 * 2. Bind to port
 * 3. Listen for connections
 * 4. Accept clients and spawn handler threads
 * ===========================================================================
 */
int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    /* Initialize equipment database */
    init_equipment();

    /* Create TCP socket
     * AF_INET  = IPv4
     * SOCK_STREAM = TCP
     * 0 = auto-select protocol (TCP)
     */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("[SERVER] socket() failed");
        exit(1);
    }

    /* Allow socket reuse to avoid "Address already in use" errors */
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("[SERVER] setsockopt() failed");
        close(server_fd);
        exit(1);
    }

    /* Configure server address structure */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;         /* IPv4 */
    server_addr.sin_addr.s_addr = INADDR_ANY; /* Listen on all interfaces */
    server_addr.sin_port = htons(PORT);       /* Port (network byte order) */

    /* Bind socket to the address/port */
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("[SERVER] bind() failed");
        close(server_fd);
        exit(1);
    }

    /* Listen for incoming connections (backlog = MAX_CLIENTS) */
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("[SERVER] listen() failed");
        close(server_fd);
        exit(1);
    }

    printf("=== University Laboratory Equipment Booking Server ===\n");
    printf("Listening on port %d\n", PORT);
    printf("Max simultaneous clients: %d\n", MAX_CLIENTS);
    printf("Registered users: ");
    for (int i = 0; valid_users[i] != NULL; i++)
    {
        printf("%s", valid_users[i]);
        if (valid_users[i + 1] != NULL)
            printf(", ");
    }
    printf("\n\n");

    /* Main accept loop - runs until process is terminated */
    while (1)
    {
        /* Accept a new client connection (blocking) */
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            perror("[SERVER] accept() failed");
            continue;
        }

        /* Check if we've reached max client limit */
        pthread_mutex_lock(&mutex);
        if (active_clients >= MAX_CLIENTS)
        {
            pthread_mutex_unlock(&mutex);
            printf("[SERVER] Rejecting connection - max clients reached\n");
            send_message(client_fd, "ERROR:Server full, try again later");
            close(client_fd);
            continue;
        }
        pthread_mutex_unlock(&mutex);

        /* Spawn a new thread to handle this client
         *
         * Thread-per-client design choices:
         *   - Each client gets an independent execution context
         *   - If one client blocks, others are unaffected
         *   - Thread creation overhead is minimal (vs. process-per-client)
         *   - Shared memory space simplifies access to equipment data
         */
        pthread_t thread;
        int *client_fd_ptr = malloc(sizeof(int));
        *client_fd_ptr = client_fd;

        if (pthread_create(&thread, NULL, handle_client, client_fd_ptr) != 0)
        {
            perror("[SERVER] pthread_create() failed");
            free(client_fd_ptr);
            send_message(client_fd, "ERROR:Internal server error");
            close(client_fd);
            continue;
        }

        /* Detach the thread so its resources are freed automatically on exit
         * We don't need to join every thread; detached threads clean up themselves. */
        pthread_detach(thread);

        /* Display updated server status */
        display_status();
    }

    /* Cleanup (never reached in normal operation) */
    close(server_fd);
    pthread_mutex_destroy(&mutex);
    return 0;
}
