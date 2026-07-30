/* =============================================================================
 * Project 5: Client Application - Laboratory Equipment Booking System
 * =============================================================================
 * File: client.c
 * Description: TCP client that connects to the equipment booking server.
 *              Provides command-line interface for user authentication,
 *              viewing equipment, and making reservations.
 *
 * Compilation:
 *   gcc -o client client.c
 *   ./client [server_ip] [port]
 *   (default: 127.0.0.1:8080)
 *
 * Communication Protocol (text-based, newline-terminated):
 *   Client -> Server: LOGIN:<user_id> | LIST | RESERVE:<equipment> | QUIT
 *   Server -> Client: AUTH_OK | AUTH_FAIL | EQUIPMENT:<list> |
 *                     RESERVED:<eq> | UNAVAILABLE:<eq> | ERROR:<msg> | BYE
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 2048

/* ===========================================================================
 * Read Server Response
 * ===========================================================================
 * Reads a newline-terminated response from the server socket.
 * Returns the number of bytes read, or -1 on error/connection closed.
 * ===========================================================================
 */
static int read_response(int sock_fd, char *buffer, size_t size)
{
    size_t total = 0;

    if (size == 0)
    {
        return -1;
    }

    /* Read until a full line is received (handles large/split TCP messages) */
    while (total < size - 1)
    {
        ssize_t n = recv(sock_fd, buffer + total, size - 1 - total, 0);
        if (n <= 0)
        {
            return -1;
        }

        total += (size_t)n;
        buffer[total] = '\0';

        if (strchr(buffer, '\n') != NULL)
        {
            break;
        }
    }

    /* Remove trailing newline or carriage return */
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
    {
        buffer[len - 1] = '\0';
        len--;
    }

    return (int)total;
}

/* ===========================================================================
 * Send Command to Server
 * ===========================================================================
 * Sends a command string (automatically appends newline) to the server.
 * Returns 0 on success, -1 on error.
 * ===========================================================================
 */
static int send_command(int sock_fd, const char *command)
{
    char buffer[BUFFER_SIZE];
    int len = snprintf(buffer, sizeof(buffer), "%s\n", command);

    if (send(sock_fd, buffer, len, 0) < 0)
    {
        perror("[CLIENT] send() failed");
        return -1;
    }

    return 0;
}

/* ===========================================================================
 * Display Equipment List
 * ===========================================================================
 * Parses and displays the EQUIPMENT: response from the server.
 * Format: EQUIPMENT:item1 (status1)\titem2 (status2)\t...
 * ===========================================================================
 */
static void display_equipment(const char *response)
{
    const char *prefix = "EQUIPMENT:";
    if (strncmp(response, prefix, strlen(prefix)) != 0)
    {
        printf("Unexpected response: %s\n", response);
        return;
    }

    const char *list = response + strlen(prefix);
    printf("\n=== Available Equipment ===\n");

    /* Parse tab-separated equipment items */
    char items[BUFFER_SIZE];
    strncpy(items, list, sizeof(items) - 1);

    int count = 0;
    char *token = strtok(items, "\t");
    while (token != NULL)
    {
        count++;
        printf("  %d. %s\n", count, token);
        token = strtok(NULL, "\t");
    }

    if (count == 0)
    {
        printf("  (No equipment available)\n");
    }
    printf("==========================\n");
}

/* ===========================================================================
 * Main Client Entry Point
 * ===========================================================================
 * Connects to the server and provides an interactive session for:
 *   1. User authentication
 *   2. Viewing available equipment
 *   3. Reserving equipment
 *   4. Session termination
 * ===========================================================================
 */
int main(int argc, char *argv[])
{
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];

    /* Parse command-line arguments (optional server IP and port) */
    const char *server_ip = "127.0.0.1";
    int port = 8080;

    if (argc > 1)
    {
        server_ip = argv[1];
    }
    if (argc > 2)
    {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535)
        {
            fprintf(stderr, "Invalid port number. Using default 8080.\n");
            port = 8080;
        }
    }

    printf("=== Laboratory Equipment Booking Client ===\n");
    printf("Connecting to %s:%d...\n", server_ip, port);

    /* Create TCP socket */
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        perror("[CLIENT] socket() failed");
        exit(1);
    }

    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    /* Convert IP address from text to binary */
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "[CLIENT] Invalid server address: %s\n", server_ip);
        close(sock_fd);
        exit(1);
    }

    /* Connect to the server */
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("[CLIENT] connect() failed");
        close(sock_fd);
        exit(1);
    }

    printf("Connected to server successfully!\n\n");

    /* ---- Main interaction loop ---- */
    int authenticated = 0;
    char user_id[32];
    size_t len;

    while (1)
    {
        if (!authenticated)
        {
            /* ----- Authentication Phase ----- */
            printf("Enter your User ID (or type 'quit' to exit): ");
            if (fgets(input, sizeof(input), stdin) == NULL)
                break;

            /* Remove trailing newline */
            len = strlen(input);
            if (len > 0 && input[len - 1] == '\n')
            {
                input[len - 1] = '\0';
            }

            if (strcmp(input, "quit") == 0)
            {
                break;
            }

            /* Send LOGIN command to server */
            char login_cmd[BUFFER_SIZE];
            snprintf(login_cmd, sizeof(login_cmd), "LOGIN:%s", input);
            send_command(sock_fd, login_cmd);

            /* Read server response */
            if (read_response(sock_fd, buffer, sizeof(buffer)) < 0)
            {
                printf("[CLIENT] Server connection lost.\n");
                break;
            }

            if (strcmp(buffer, "AUTH_OK") == 0)
            {
                authenticated = 1;
                strncpy(user_id, input, sizeof(user_id) - 1);
                printf("Authentication successful! Welcome, %s.\n\n", user_id);
            }
            else
            {
                printf("Authentication failed. Please check your User ID.\n\n");
            }
        }
        else
        {
            /* ----- Post-Authentication Menu ----- */
            printf("\n=== Main Menu ===\n");
            printf("1. List available equipment\n");
            printf("2. Reserve equipment\n");
            printf("3. Logout and exit\n");
            printf("Enter choice (1-3): ");

            if (fgets(input, sizeof(input), stdin) == NULL)
                break;
            int choice = atoi(input);

            switch (choice)
            {
            case 1:
                /* Request equipment list */
                send_command(sock_fd, "LIST");

                /* Read and display response */
                if (read_response(sock_fd, buffer, sizeof(buffer)) < 0)
                {
                    printf("[CLIENT] Server connection lost.\n");
                    goto cleanup;
                }

                if (strncmp(buffer, "EQUIPMENT:", 10) == 0)
                {
                    display_equipment(buffer);
                }
                else
                {
                    printf("Server response: %s\n", buffer);
                }
                break;

            case 2:
                /* First show available equipment */
                send_command(sock_fd, "LIST");
                if (read_response(sock_fd, buffer, sizeof(buffer)) < 0)
                {
                    printf("[CLIENT] Server connection lost.\n");
                    goto cleanup;
                }
                display_equipment(buffer);

                /* Ask user which equipment to reserve */
                printf("\nEnter the exact name of equipment to reserve: ");
                if (fgets(input, sizeof(input), stdin) == NULL)
                    break;

                len = strlen(input);
                if (len > 0 && input[len - 1] == '\n')
                {
                    input[len - 1] = '\0';
                }

                /* Send reservation request */
                char reserve_cmd[BUFFER_SIZE];
                snprintf(reserve_cmd, sizeof(reserve_cmd), "RESERVE:%s", input);
                send_command(sock_fd, reserve_cmd);

                /* Read and display response */
                if (read_response(sock_fd, buffer, sizeof(buffer)) < 0)
                {
                    printf("[CLIENT] Server connection lost.\n");
                    goto cleanup;
                }

                if (strncmp(buffer, "RESERVED:", 9) == 0)
                {
                    printf("\n*** Reservation confirmed! '%s' is now reserved for you. ***\n",
                           buffer + 9);
                }
                else if (strncmp(buffer, "UNAVAILABLE:", 12) == 0)
                {
                    printf("\n*** Sorry, '%s' is already reserved by another user. ***\n",
                           buffer + 12);
                }
                else if (strncmp(buffer, "ERROR:", 6) == 0)
                {
                    printf("\n*** Error: %s ***\n", buffer + 6);
                }
                else
                {
                    printf("Server response: %s\n", buffer);
                }
                break;

            case 3:
                /* Logout and disconnect */
                send_command(sock_fd, "QUIT");
                if (read_response(sock_fd, buffer, sizeof(buffer)) > 0)
                {
                    printf("Session closed. Goodbye, %s\n", user_id);
                }
                goto cleanup;

            default:
                printf("Invalid choice. Please enter 1, 2, or 3.\n");
                break;
            }
        }
    }

cleanup:
    close(sock_fd);
    printf("\nClient disconnected.\n");
    return 0;
}
