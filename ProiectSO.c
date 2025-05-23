#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#define MAX_USERNAME_LEN 50
#define MAX_CLUE_LEN 256
#define MAX_PATH_LEN 512
#define TREASURE_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"

typedef struct {
    int id;
    char username[MAX_USERNAME_LEN];
    double latitude;
    double longitude;
    char clue[MAX_CLUE_LEN];
    int value;
} Treasure;


void add_treasure(const char *hunt_id);
void list_treasures(const char *hunt_id);
void view_treasure(const char *hunt_id, int treasure_id);
void remove_treasure(const char *hunt_id, int treasure_id);
void remove_hunt(const char *hunt_id);
void log_operation(const char *hunt_id, const char *operation);
void create_symlink(const char *hunt_id);
int get_next_id(const char *hunt_id);
int hunt_exists(const char *hunt_id);
void ensure_hunt_dir(const char *hunt_id);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s --<operation> <hunt_id> [treasure_id]\n", argv[0]);
        fprintf(stderr, "Operations: add, list, view, remove_treasure, remove_hunt\n");
        return EXIT_FAILURE;
    }

    const char *operation = argv[1];
    const char *hunt_id = argv[2];

    
    if (operation[0] != '-' || operation[1] != '-') {
        fprintf(stderr, "Operation should start with '--'\n");
        return EXIT_FAILURE;
    }
    
    
    operation += 2;

    if (strcmp(operation, "add") == 0) {
        add_treasure(hunt_id);
    } else if (strcmp(operation, "list") == 0) {
        list_treasures(hunt_id);
    } else if (strcmp(operation, "view") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Missing treasure ID for view operation\n");
            return EXIT_FAILURE;
        }
        view_treasure(hunt_id, atoi(argv[3]));
    } else if (strcmp(operation, "remove_treasure") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Missing treasure ID for remove_treasure operation\n");
            return EXIT_FAILURE;
        }
        remove_treasure(hunt_id, atoi(argv[3]));
    } else if (strcmp(operation, "remove_hunt") == 0) {
        remove_hunt(hunt_id);
    } else {
        fprintf(stderr, "Unknown operation: %s\n", operation);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


int hunt_exists(const char *hunt_id) {
    struct stat st;
    char path[MAX_PATH_LEN];
    snprintf(path, sizeof(path), "%s", hunt_id);
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}


void ensure_hunt_dir(const char *hunt_id) {
    if (!hunt_exists(hunt_id)) {
        if (mkdir(hunt_id, 0755) != 0) {
            fprintf(stderr, "Failed to create hunt directory: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        printf("Created new hunt: %s\n", hunt_id);
        
        
        char log_path[MAX_PATH_LEN];
        snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, LOG_FILE);
        int log_fd = open(log_path, O_WRONLY | O_CREAT, 0644);
        if (log_fd == -1) {
            fprintf(stderr, "Failed to create log file: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        close(log_fd);
        
       
        create_symlink(hunt_id);
    }
}


void create_symlink(const char *hunt_id) {
    char log_path[MAX_PATH_LEN];
    char symlink_path[MAX_PATH_LEN];
    
  
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, LOG_FILE);
    
    
    snprintf(symlink_path, sizeof(symlink_path), "%s-%s", LOG_FILE, hunt_id);
    
    
    unlink(symlink_path);
    
    
    if (symlink(log_path, symlink_path) != 0) {
        fprintf(stderr, "Failed to create symbolic link: %s\n", strerror(errno));
    }
}


void log_operation(const char *hunt_id, const char *operation) {
    char log_path[MAX_PATH_LEN];
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, LOG_FILE);
    
    int log_fd = open(log_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (log_fd == -1) {
        fprintf(stderr, "Failed to open log file: %s\n", strerror(errno));
        return;
    }
    
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[26];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
 
    char log_entry[512];
    snprintf(log_entry, sizeof(log_entry), "[%s] %s\n", timestamp, operation);
    
   
    if (write(log_fd, log_entry, strlen(log_entry)) == -1) {
        fprintf(stderr, "Failed to write to log file: %s\n", strerror(errno));
    }
    
    close(log_fd);
}


int get_next_id(const char *hunt_id) {
    char file_path[MAX_PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);
    
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        
        return 1;
    }
    
    
    int count = 0;
    Treasure treasure;
    
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        count++;
    }
    
    close(fd);
    return count + 1;
}


void add_treasure(const char *hunt_id) {
    
    ensure_hunt_dir(hunt_id);
    
    char file_path[MAX_PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);
    
    
    int fd = open(file_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        fprintf(stderr, "Failed to open treasure file: %s\n", strerror(errno));
        return;
    }
    
    
    Treasure treasure;
    treasure.id = get_next_id(hunt_id);
    
    printf("Enter username: ");
    fgets(treasure.username, MAX_USERNAME_LEN, stdin);
    treasure.username[strcspn(treasure.username, "\n")] = 0; // Remove newline
    
    printf("Enter latitude: ");
    scanf("%lf", &treasure.latitude);
    
    printf("Enter longitude: ");
    scanf("%lf", &treasure.longitude);
    
   
    getchar();
    
    printf("Enter clue: ");
    fgets(treasure.clue, MAX_CLUE_LEN, stdin);
    treasure.clue[strcspn(treasure.clue, "\n")] = 0; // Remove newline
    
    printf("Enter value: ");
    scanf("%d", &treasure.value);
    
    
    if (write(fd, &treasure, sizeof(Treasure)) != sizeof(Treasure)) {
        fprintf(stderr, "Failed to write treasure: %s\n", strerror(errno));
        close(fd);
        return;
    }
    
    close(fd);
    
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Added treasure %d by %s", treasure.id, treasure.username);
    log_operation(hunt_id, log_msg);
    
    printf("Treasure %d added successfully to hunt %s\n", treasure.id, hunt_id);
}


void list_treasures(const char *hunt_id) {
    if (!hunt_exists(hunt_id)) {
        fprintf(stderr, "Hunt does not exist: %s\n", hunt_id);
        return;
    }
    
    char file_path[MAX_PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);
    
    
    struct stat st;
    if (stat(file_path, &st) == -1) {
        fprintf(stderr, "Failed to get file information: %s\n", strerror(errno));
        return;
    }
    
   
    printf("Hunt: %s\n", hunt_id);
    printf("File Size: %ld bytes\n", st.st_size);
    
   
    char time_str[26];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
    printf("Last Modified: %s\n\n", time_str);
    

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        if (errno == ENOENT) {
            printf("No treasures found in hunt %s\n", hunt_id);
        } else {
            fprintf(stderr, "Failed to open treasure file: %s\n", strerror(errno));
        }
        return;
    }
    
  
    Treasure treasure;
    int count = 0;
    
    printf("Treasures:\n");
    printf("-------------------------------------------\n");
    
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d, User: %s, Value: %d\n", treasure.id, treasure.username, treasure.value);
        count++;
    }
    
    if (count == 0) {
        printf("No treasures found\n");
    } else {
        printf("-------------------------------------------\n");
        printf("Total treasures: %d\n", count);
    }
    
    close(fd);
    
   
    log_operation(hunt_id, "Listed all treasures");
}

void view_treasure(const char *hunt_id, int treasure_id) {
    if (!hunt_exists(hunt_id)) {
        fprintf(stderr, "Hunt does not exist: %s\n", hunt_id);
        return;
    }
    
    char file_path[MAX_PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);
    
  
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Failed to open treasure file: %s\n", strerror(errno));
        return;
    }
    
   
    Treasure treasure;
    int found = 0;
    
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        if (treasure.id == treasure_id) {
            found = 1;
            break;
        }
    }
    
    close(fd);
    
    if (found) {
        printf("Treasure Details:\n");
        printf("-------------------------------------------\n");
        printf("ID: %d\n", treasure.id);
        printf("User: %s\n", treasure.username);
        printf("Location: %.6f, %.6f\n", treasure.latitude, treasure.longitude);
        printf("Clue: %s\n", treasure.clue);
        printf("Value: %d\n", treasure.value);
        printf("-------------------------------------------\n");
        
        
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Viewed treasure %d", treasure_id);
        log_operation(hunt_id, log_msg);
    } else {
        fprintf(stderr, "Treasure with ID %d not found in hunt %s\n", treasure_id, hunt_id);
    }
}


void remove_treasure(const char *hunt_id, int treasure_id) {
    if (!hunt_exists(hunt_id)) {
        fprintf(stderr, "Hunt does not exist: %s\n", hunt_id);
        return;
    }
    
    char file_path[MAX_PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);
    
 
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Failed to open treasure file: %s\n", strerror(errno));
        return;
    }
    
 
    char temp_path[MAX_PATH_LEN];
    snprintf(temp_path, sizeof(temp_path), "%s/temp_%s", hunt_id, TREASURE_FILE);
    
    int temp_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1) {
        fprintf(stderr, "Failed to create temporary file: %s\n", strerror(errno));
        close(fd);
        return;
    }
    
 
    Treasure treasure;
    int found = 0;
    
    while (read(fd, &treasure, sizeof(Treasure)) == sizeof(Treasure)) {
        if (treasure.id == treasure_id) {
            found = 1;
            continue;
        }
        
        if (write(temp_fd, &treasure, sizeof(Treasure)) != sizeof(Treasure)) {
            fprintf(stderr, "Failed to write to temporary file: %s\n", strerror(errno));
            close(fd);
            close(temp_fd);
            unlink(temp_path);
            return;
        }
    }
    
    close(fd);
    close(temp_fd);
    
    if (!found) {
        fprintf(stderr, "Treasure with ID %d not found in hunt %s\n", treasure_id, hunt_id);
        unlink(temp_path);
        return;
    }
    
 
    if (rename(temp_path, file_path) != 0) {
        fprintf(stderr, "Failed to replace treasure file: %s\n", strerror(errno));
        unlink(temp_path);
        return;
    }
    
  
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Removed treasure %d", treasure_id);
    log_operation(hunt_id, log_msg);
    
    printf("Treasure %d removed successfully from hunt %s\n", treasure_id, hunt_id);
}


void remove_hunt(const char *hunt_id) {
    if (!hunt_exists(hunt_id)) {
        fprintf(stderr, "Hunt does not exist: %s\n", hunt_id);
        return;
    }
    
    char file_path[MAX_PATH_LEN];
    char log_path[MAX_PATH_LEN];
    char symlink_path[MAX_PATH_LEN];
    
   
    snprintf(file_path, sizeof(file_path), "%s/%s", hunt_id, TREASURE_FILE);
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, LOG_FILE);
    snprintf(symlink_path, sizeof(symlink_path), "%s-%s", LOG_FILE, hunt_id);
    
    
    if (unlink(file_path) != 0 && errno != ENOENT) {
        fprintf(stderr, "Failed to remove treasure file: %s\n", strerror(errno));
    }
    
    
    if (unlink(log_path) != 0 && errno != ENOENT) {
        fprintf(stderr, "Failed to remove log file: %s\n", strerror(errno));
    }
    
    
    if (unlink(symlink_path) != 0 && errno != ENOENT) {
        fprintf(stderr, "Failed to remove symlink: %s\n", strerror(errno));
    }
    

    if (rmdir(hunt_id) != 0) {
        fprintf(stderr, "Failed to remove hunt directory: %s\n", strerror(errno));
        return;
    }
    
    printf("Hunt %s removed successfully\n", hunt_id);
}