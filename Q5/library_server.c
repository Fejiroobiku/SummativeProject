/**
 * Digital Library Reservation Platform - Server
 * 
 * Features:
 * - TCP socket server supporting up to 5 concurrent clients
 * - Thread-based concurrency for handling multiple clients
 * - Mutex-protected shared resources (books, active users)
 * - Authentication via library IDs
 * - Book reservation with race condition prevention
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

/* ============================================================
 * Constants
 * ============================================================ */
#define PORT                8080
#define MAX_CLIENTS         5
#define BUFFER_SIZE         1024
#define MAX_BOOKS           10
#define MAX_USERS           10

/* ============================================================
 * Book Structure
 * ============================================================ */
typedef struct {
    int id;
    char title[100];
    int reserved;           // 0 = available, 1 = reserved
    int reserved_by;        // Library ID of user who reserved
} Book;

/* ============================================================
 * Active User Structure
 * ============================================================ */
typedef struct {
    int lib_id;
    int socket_fd;
    int authenticated;
    char ip_addr[INET_ADDRSTRLEN];
} ClientSession;

/* ============================================================
 * Shared Data Structures
 * ============================================================ */
Book books[MAX_BOOKS];
ClientSession active_users[MAX_CLIENTS];
int active_user_count = 0;

pthread_mutex_t books_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Valid library IDs (predefined) */
int valid_ids[] = {1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010};
int valid_id_count = 10;

/* ============================================================
 * Function: init_books()
 * Initializes the book catalog
 * ============================================================ */
void init_books() {
    char *titles[] = {
        "The C Programming Language",
        "Operating Systems: Three Easy Pieces",
        "Computer Networks",
        "Database Systems",
        "Artificial Intelligence",
        "Python Programming",
        "Data Structures and Algorithms",
        "Linux Kernel Development",
        "Design Patterns",
        "Clean Code"
    };
    
    for (int i = 0; i < MAX_BOOKS; i++) {
        books[i].id = i + 1;
        strcpy(books[i].title, titles[i]);
        books[i].reserved = 0;
        books[i].reserved_by = -1;
    }
}

/* ============================================================
 * Function: is_valid_library_id()
 * Checks if given ID exists in valid IDs list
 * ============================================================ */
int is_valid_library_id(int lib_id) {
    for (int i = 0; i < valid_id_count; i++) {
        if (valid_ids[i] == lib_id) {
            return 1;
        }
    }
    return 0;
}

/* ============================================================
 * Function: add_active_user()
 * Adds user to active users list
 * Returns: 1 on success, 0 on failure (max clients reached)
 * ============================================================ */
int add_active_user(int lib_id, int socket_fd, char *ip_addr) {
    pthread_mutex_lock(&users_mutex);
    
    if (active_user_count >= MAX_CLIENTS) {
        pthread_mutex_unlock(&users_mutex);
        return 0;
    }
    
    active_users[active_user_count].lib_id = lib_id;
    active_users[active_user_count].socket_fd = socket_fd;
    active_users[active_user_count].authenticated = 1;
    strcpy(active_users[active_user_count].ip_addr, ip_addr);
    active_user_count++;
    
    pthread_mutex_unlock(&users_mutex);
    return 1;
}

/* ============================================================
 * Function: remove_active_user()
 * Removes user from active users list by socket
 * ============================================================ */
void remove_active_user(int socket_fd) {
    pthread_mutex_lock(&users_mutex);
    
    for (int i = 0; i < active_user_count; i++) {
        if (active_users[i].socket_fd == socket_fd) {
            // Shift remaining users
            for (int j = i; j < active_user_count - 1; j++) {
                active_users[j] = active_users[j + 1];
            }
            active_user_count--;
            break;
        }
    }
    
    pthread_mutex_unlock(&users_mutex);
}

/* ============================================================
 * Function: display_active_users()
 * Prints list of currently authenticated users (server console)
 * ============================================================ */
void display_active_users() {
    pthread_mutex_lock(&users_mutex);
    
    printf("\n[ACTIVE USERS] Total: %d\n", active_user_count);
    for (int i = 0; i < active_user_count; i++) {
        printf("  - Library ID: %d | IP: %s\n", 
               active_users[i].lib_id, active_users[i].ip_addr);
    }
    
    pthread_mutex_unlock(&users_mutex);
}

/* ============================================================
 * Function: display_book_status()
 * Prints current book status (server console)
 * ============================================================ */
void display_book_status() {
    pthread_mutex_lock(&books_mutex);
    
    printf("\n[BOOK STATUS]\n");
    for (int i = 0; i < MAX_BOOKS; i++) {
        printf("  [%d] %s - %s", 
               books[i].id, 
               books[i].title,
               books[i].reserved ? "RESERVED" : "AVAILABLE");
        if (books[i].reserved) {
            printf(" (by ID: %d)", books[i].reserved_by);
        }
        printf("\n");
    }
    
    pthread_mutex_unlock(&books_mutex);
}

/* ============================================================
 * Function: send_book_list()
 * Sends available books list to client
 * ============================================================ */
void send_book_list(int client_fd) {
    char buffer[BUFFER_SIZE];
    
    pthread_mutex_lock(&books_mutex);
    
    strcpy(buffer, "\n========== Available Books ==========\n");
    send(client_fd, buffer, strlen(buffer), 0);
    
    for (int i = 0; i < MAX_BOOKS; i++) {
        if (!books[i].reserved) {
            snprintf(buffer, BUFFER_SIZE, "[%d] %s\n", books[i].id, books[i].title);
            send(client_fd, buffer, strlen(buffer), 0);
        }
    }
    
    strcpy(buffer, "=====================================\n");
    send(client_fd, buffer, strlen(buffer), 0);
    
    strcpy(buffer, "Enter book ID to reserve (or 0 to exit): ");
    send(client_fd, buffer, strlen(buffer), 0);
    
    pthread_mutex_unlock(&books_mutex);
}

/* ============================================================
 * Function: reserve_book()
 * Attempts to reserve a book for a user
 * Returns: 1 on success, 0 on failure (already reserved)
 * ============================================================ */
int reserve_book(int book_id, int lib_id) {
    int result = 0;
    
    pthread_mutex_lock(&books_mutex);
    
    if (book_id >= 1 && book_id <= MAX_BOOKS) {
        if (!books[book_id - 1].reserved) {
            books[book_id - 1].reserved = 1;
            books[book_id - 1].reserved_by = lib_id;
            result = 1;
            printf("[RESERVATION] User %d reserved book: %s\n", 
                   lib_id, books[book_id - 1].title);
        }
    }
    
    pthread_mutex_unlock(&books_mutex);
    
    display_book_status();  // Update server console
    display_active_users();
    
    return result;
}

/* ============================================================
 * Function: handle_client()
 * Thread function to handle individual client connection
 * ============================================================ */
void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    
    char buffer[BUFFER_SIZE];
    int bytes_read;
    int lib_id = -1;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char client_ip[INET_ADDRSTRLEN];
    
    // Get client IP
    getpeername(client_fd, (struct sockaddr *)&client_addr, &addr_len);
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    
    printf("[NEW CONNECTION] Client connected from %s\n", client_ip);
    
    // Authentication phase
    send(client_fd, "Enter Library ID: ", 18, 0);
    
    bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read <= 0) {
        printf("[ERROR] Authentication failed - connection closed\n");
        close(client_fd);
        pthread_exit(NULL);
    }
    
    buffer[bytes_read] = '\0';
    lib_id = atoi(buffer);
    
    // Validate library ID
    if (!is_valid_library_id(lib_id)) {
        send(client_fd, "AUTH_FAILED: Invalid Library ID\n", 33, 0);
        printf("[AUTH FAILED] Invalid ID %d from %s\n", lib_id, client_ip);
        close(client_fd);
        pthread_exit(NULL);
    }
    
    // Add to active users
    if (!add_active_user(lib_id, client_fd, client_ip)) {
        send(client_fd, "AUTH_FAILED: Server at capacity\n", 33, 0);
        printf("[AUTH FAILED] Max clients reached - rejected %d\n", lib_id);
        close(client_fd);
        pthread_exit(NULL);
    }
    
    send(client_fd, "AUTH_SUCCESS\n", 13, 0);
    printf("[AUTH SUCCESS] User %d authenticated from %s\n", lib_id, client_ip);
    display_active_users();
    
    // Main client interaction loop
    while (1) {
        send_book_list(client_fd);
        
        bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read <= 0) {
            printf("[DISCONNECT] User %d disconnected\n", lib_id);
            break;
        }
        
        buffer[bytes_read] = '\0';
        int book_id = atoi(buffer);
        
        if (book_id == 0) {
            send(client_fd, "GOODBYE\n", 8, 0);
            printf("[EXIT] User %d logged out\n", lib_id);
            break;
        }
        
        // Attempt reservation
        if (reserve_book(book_id, lib_id)) {
            snprintf(buffer, BUFFER_SIZE, 
                     "SUCCESS: Book %d reserved successfully!\n", book_id);
            send(client_fd, buffer, strlen(buffer), 0);
        } else {
            snprintf(buffer, BUFFER_SIZE, 
                     "FAILED: Book %d is already reserved.\n", book_id);
            send(client_fd, buffer, strlen(buffer), 0);
        }
    }
    
    // Cleanup
    remove_active_user(client_fd);
    close(client_fd);
    display_active_users();
    
    pthread_exit(NULL);
}

/* ============================================================
 * Function: signal_handler()
 * Handles Ctrl+C to exit gracefully
 * ============================================================ */
void signal_handler(int sig) {
    printf("\n[SHUTDOWN] Server shutting down...\n");
    display_book_status();
    display_active_users();
    exit(0);
}

/* ============================================================
 * Function: main()
 * Server entry point - creates listening socket and accepts clients
 * ============================================================ */
int main() {
    int server_fd;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    // Initialize data
    init_books();
    signal(SIGINT, signal_handler);
    
    printf("\n========================================\n");
    printf("  DIGITAL LIBRARY RESERVATION PLATFORM\n");
    printf("========================================\n");
    printf("Server starting on port %d...\n", PORT);
    printf("Max concurrent clients: %d\n", MAX_CLIENTS);
    printf("========================================\n\n");
    
    display_book_status();
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(1);
    }
    
    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    // Listen for connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(1);
    }
    
    printf("[SERVER] Listening for connections...\n\n");
    
    // Accept clients
    while (1) {
        int *client_fd = malloc(sizeof(int));
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        
        if (*client_fd < 0) {
            perror("Accept failed");
            free(client_fd);
            continue;
        }
        
        // Create thread for client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client_fd) != 0) {
            perror("Thread creation failed");
            close(*client_fd);
            free(client_fd);
        } else {
            pthread_detach(thread);  // Auto-cleanup when thread exits
        }
    }
    
    close(server_fd);
    return 0;
}