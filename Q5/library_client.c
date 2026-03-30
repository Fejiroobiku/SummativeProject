/**
 * Digital Library Reservation Platform - Client
 * 
 * Features:
 * - TCP connection to library server
 * - Library ID authentication
 * - Browse available books
 * - Reserve books
 * - Graceful session termination
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT                8080
#define BUFFER_SIZE         1024
#define SERVER_IP           "127.0.0.1"

/* ============================================================
 * Function: receive_response()
 * Receives and displays server response
 * ============================================================ */
void receive_response(int sock_fd) {
    char buffer[BUFFER_SIZE];
    int bytes_read;
    
    bytes_read = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
}

/* ============================================================
 * Function: authenticate()
 * Handles authentication with server
 * Returns: 1 on success, 0 on failure
 * ============================================================ */
int authenticate(int sock_fd) {
    char buffer[BUFFER_SIZE];
    int lib_id;
    
    // Receive authentication prompt
    receive_response(sock_fd);
    
    // Send library ID
    printf("Enter Library ID: ");
    scanf("%d", &lib_id);
    getchar();  // Consume newline
    
    snprintf(buffer, BUFFER_SIZE, "%d\n", lib_id);
    send(sock_fd, buffer, strlen(buffer), 0);
    
    // Receive authentication result
    receive_response(sock_fd);
    
    // Check if authenticated
    char auth_result[BUFFER_SIZE];
    int bytes = recv(sock_fd, auth_result, BUFFER_SIZE - 1, 0);
    if (bytes > 0) {
        auth_result[bytes] = '\0';
        printf("%s", auth_result);
        
        if (strstr(auth_result, "AUTH_SUCCESS")) {
            return lib_id;
        }
    }
    
    return 0;
}

/* ============================================================
 * Function: main()
 * Client entry point
 * ============================================================ */
int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int lib_id;
    
    printf("\n========================================\n");
    printf("  DIGITAL LIBRARY CLIENT\n");
    printf("========================================\n");
    
    // Create socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    // Convert IP address
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(1);
    }
    
    // Connect to server
    printf("Connecting to server at %s:%d...\n", SERVER_IP, PORT);
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }
    
    printf("Connected successfully!\n\n");
    
    // Authenticate
    lib_id = authenticate(sock_fd);
    if (!lib_id) {
        printf("\n[ERROR] Authentication failed. Exiting.\n");
        close(sock_fd);
        exit(1);
    }
    
    printf("\n[SUCCESS] Welcome, Library ID: %d\n", lib_id);
    
    // Main interaction loop
    while (1) {
        int choice;
        
        // Receive book list
        receive_response(sock_fd);
        
        // Get user choice
        if (scanf("%d", &choice) != 1) {
            break;
        }
        getchar();  // Consume newline
        
        // Send choice to server
        snprintf(buffer, BUFFER_SIZE, "%d\n", choice);
        send(sock_fd, buffer, strlen(buffer), 0);
        
        if (choice == 0) {
            // Exit
            receive_response(sock_fd);
            printf("\nSession closed. Goodbye, %d\n", lib_id);
            break;
        }
        
        // Receive reservation result
        receive_response(sock_fd);
        printf("\n");
    }
    
    close(sock_fd);
    return 0;
}