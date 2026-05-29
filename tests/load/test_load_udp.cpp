//
// UDP Load Test - Нагрузочное тестирование UDP сервера
//
// Запуск: ./bin/test_load_udp [requests] [concurrency]
// Пример: ./bin/test_load_udp 1000 20
//

#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <future>
#include <vector>
#include <atomic>
#include <mutex>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <algorithm>

#include "Server.h"


// Структура для хранения результатов одного запроса
struct RequestResult {
    uint64_t request_id;
    std::string imsi;
    int status_code;
    std::string response;
    uint64_t duration_ns;
};

// Конфигурация нагрузочного теста
struct LoadTestConfig {
    size_t total_requests = 1000;
    size_t concurrency = 20;
    std::string server_config = "test_load_server.json";
    std::string db_file = "test_load.db";
    std::string cdr_file = "test_load_cdr.log";
    std::string log_file = "test_load.log";
    uint16_t udp_port = 9000;
    uint16_t http_port = 8080;
};

// Результаты теста
struct LoadTestResults {
    std::vector<RequestResult> results;
    std::mutex results_mutex;

    uint64_t total_time_ns = 0;
    uint64_t min_latency_ns = UINT64_MAX;
    uint64_t max_latency_ns = 0;
    uint64_t sum_latency_ns = 0;
    size_t success_count = 0;
    size_t error_count = 0;

    void addResult(const RequestResult& result) {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back(result);

        if (result.duration_ns < min_latency_ns) min_latency_ns = result.duration_ns;
        if (result.duration_ns > max_latency_ns) max_latency_ns = result.duration_ns;
        sum_latency_ns += result.duration_ns;

        if (result.status_code == 0) success_count++;
        else error_count++;
    }
};


// Генерация уникального IMSI
std::string generateImsi(size_t id) {
    std::stringstream ss;
    ss << "005050" << std::setfill('0') << std::setw(9) << id;
    return ss.str();
}


// Создание тестовых конфигов
void createLoadTestConfigs(const LoadTestConfig& config) {
    std::ofstream serverConfig(config.server_config);
    serverConfig << R"({
        "udp_ip": "0.0.0.0",
        "udp_port": )" << config.udp_port << R"(,
        "tcp_ip": "0.0.0.0",
        "tcp_port": 9090,
        "session_timeout_sec": 30,
        "database_file": ")" << config.db_file << R"(",
        "cdr_file": ")" << config.cdr_file << R"(",
        "http_port": )" << config.http_port << R"(,
        "graceful_shutdown_rate": 10,
        "log_file": ")" << config.log_file << R"(",
        "log_level": "WARN",
        "blacklist": ["101010101010101"]
    })";
    serverConfig.close();
}


// Простой UDP клиент (без использования pgw::client::Client)
int sendUdpRequest(const std::string& imsi, uint16_t port, std::string& response) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // Отправляем IMSI
    sendto(sock, imsi.c_str(), imsi.length(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

    // Получаем ответ
    char buffer[256];
    socklen_t addrLen = sizeof(serverAddr);
    ssize_t bytesReceived = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr*)&serverAddr, &addrLen);

    close(sock);

    if (bytesReceived > 0) {
        response = std::string(buffer, bytesReceived);
        return 0;
    }
    return -1;
}


// Функция для одного рабочего потока
void loadTestWorker(
    size_t worker_id,
    size_t start_request,
    size_t end_request,
    uint16_t udp_port,
    LoadTestResults& results
) {
    (void)worker_id;  // Suppress unused warning

    for (size_t i = start_request; i < end_request; i++) {
        RequestResult result;
        result.request_id = i;
        result.imsi = generateImsi(i);

        auto start_ns = std::chrono::steady_clock::now().time_since_epoch().count();

        std::string response;
        result.status_code = sendUdpRequest(result.imsi, udp_port, response);
        result.response = response;

        auto end_ns = std::chrono::steady_clock::now().time_since_epoch().count();
        result.duration_ns = end_ns - start_ns;

        results.addResult(result);
    }
}


// Вывод статистики
void printStatistics(const LoadTestResults& results, const LoadTestConfig& config) {
    std::cout << "\n========================================\n";
    std::cout << "UDP LOAD TEST RESULTS\n";
    std::cout << "========================================\n";
    std::cout << "Configuration:\n";
    std::cout << "  Total requests:   " << config.total_requests << "\n";
    std::cout << "  Concurrency:      " << config.concurrency << "\n";
    std::cout << "  UDP Port:         " << config.udp_port << "\n";
    std::cout << "\n";

    // Основная статистика
    double avg_latency_us = static_cast<double>(results.sum_latency_ns) / results.results.size() / 1000.0;
    double min_latency_us = static_cast<double>(results.min_latency_ns) / 1000.0;
    double max_latency_us = static_cast<double>(results.max_latency_ns) / 1000.0;
    double total_time_sec = static_cast<double>(results.total_time_ns) / 1000000000.0;
    double rps = static_cast<double>(results.results.size()) / total_time_sec;

    std::cout << "Results:\n";
    std::cout << "  Total time:       " << std::fixed << std::setprecision(3) << total_time_sec << " sec\n";
    std::cout << "  Requests/sec:     " << std::fixed << std::setprecision(2) << rps << "\n";
    std::cout << "\n";

    std::cout << "Latency (microseconds):\n";
    std::cout << "  Min:              " << std::fixed << std::setprecision(2) << min_latency_us << " us\n";
    std::cout << "  Max:              " << std::fixed << std::setprecision(2) << max_latency_us << " us\n";
    std::cout << "  Average:          " << std::fixed << std::setprecision(2) << avg_latency_us << " us\n";
    std::cout << "\n";

    std::cout << "Latency (milliseconds):\n";
    std::cout << "  Min:              " << std::fixed << std::setprecision(3) << min_latency_us / 1000.0 << " ms\n";
    std::cout << "  Max:              " << std::fixed << std::setprecision(3) << max_latency_us / 1000.0 << " ms\n";
    std::cout << "  Average:          " << std::fixed << std::setprecision(3) << avg_latency_us / 1000.0 << " ms\n";
    std::cout << "\n";

    // Перцентили
    std::vector<uint64_t> latencies;
    latencies.reserve(results.results.size());
    for (const auto& r : results.results) {
        latencies.push_back(r.duration_ns);
    }
    std::sort(latencies.begin(), latencies.end());

    size_t p50_idx = latencies.size() * 50 / 100;
    size_t p90_idx = latencies.size() * 90 / 100;
    size_t p95_idx = latencies.size() * 95 / 100;
    size_t p99_idx = latencies.size() * 99 / 100;

    std::cout << "Percentiles (milliseconds):\n";
    std::cout << "  P50:              " << std::fixed << std::setprecision(3) << static_cast<double>(latencies[p50_idx]) / 1000000.0 << " ms\n";
    std::cout << "  P90:              " << std::fixed << std::setprecision(3) << static_cast<double>(latencies[p90_idx]) / 1000000.0 << " ms\n";
    std::cout << "  P95:              " << std::fixed << std::setprecision(3) << static_cast<double>(latencies[p95_idx]) / 1000000.0 << " ms\n";
    std::cout << "  P99:              " << std::fixed << std::setprecision(3) << static_cast<double>(latencies[p99_idx]) / 1000000.0 << " ms\n";
    std::cout << "\n";

    // Распределение
    std::cout << "Latency distribution:\n";
    size_t count_0_10ms = 0, count_10_50ms = 0, count_50_100ms = 0, count_100_500ms = 0, count_500plus = 0;
    for (const auto& r : results.results) {
        double ms = static_cast<double>(r.duration_ns) / 1000000.0;
        if (ms < 10.0) count_0_10ms++;
        else if (ms < 50.0) count_10_50ms++;
        else if (ms < 100.0) count_50_100ms++;
        else if (ms < 500.0) count_100_500ms++;
        else count_500plus++;
    }
    std::cout << "  <10ms:            " << count_0_10ms << " (" << std::fixed << std::setprecision(1) << 100.0 * count_0_10ms / results.results.size() << "%)\n";
    std::cout << "  10-50ms:          " << count_10_50ms << " (" << std::fixed << std::setprecision(1) << 100.0 * count_10_50ms / results.results.size() << "%)\n";
    std::cout << "  50-100ms:         " << count_50_100ms << " (" << std::fixed << std::setprecision(1) << 100.0 * count_50_100ms / results.results.size() << "%)\n";
    std::cout << "  100-500ms:        " << count_100_500ms << " (" << std::fixed << std::setprecision(1) << 100.0 * count_100_500ms / results.results.size() << "%)\n";
    std::cout << "  >500ms:           " << count_500plus << " (" << std::fixed << std::setprecision(1) << 100.0 * count_500plus / results.results.size() << "%)\n";
    std::cout << "\n";

    // Success rate
    std::cout << "Success rate:\n";
    std::cout << "  Successful:       " << results.success_count << "\n";
    std::cout << "  Failed:           " << results.error_count << "\n";
    std::cout << "  Success rate:     " << std::fixed << std::setprecision(2) << 100.0 * results.success_count / results.results.size() << "%\n";
    std::cout << "========================================\n";
}



// Для моей убунточки:
// vimero@Kazutoro:~$ nproc -> 8
// Значит, чтобы пропустить контекстные переключения
// между ядрами, и получить самый чистый бенчмарк
// для латентности сервера берем 8

// Точка входа
int main(int argc, char** argv) {
    LoadTestConfig config;
    config.total_requests = 1000;
    config.concurrency = 8;

    // Перводим возможные аргументы сразу в unsigned long для расчетов
    if (argc > 1) config.total_requests = std::stoul(argv[1]);
    if (argc > 2) config.concurrency = std::stoul(argv[2]);

    std::cout << "========================================\n";
    std::cout << "UDP LOAD TEST (Standalone)\n";
    std::cout << "========================================\n";
    std::cout << "Configuration:\n";
    std::cout << "  Total requests:   " << config.total_requests << "\n";
    std::cout << "  Concurrency:      " << config.concurrency << "\n";
    std::cout << "\n";

    // Создаём конфиги
    createLoadTestConfigs(config);

    // Запускаем сервер
    // pgw::server::Server server(config.server_config);
    // auto serverFuture = std::async(std::launch::async, [&server]() {
    //     return server.run();
    // });

    // // Даем время серверу запуститься
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Запускаем тест
    LoadTestResults results;

    auto start_time = std::chrono::steady_clock::now();

    size_t requests_per_worker = config.total_requests / config.concurrency;
    std::vector<std::thread> workers;

    // TODO: это означет, что латентность для всех сессий всключает
    // создание/закрытие сокета и всё находящееся между ними
    // но не содерит создание клиентского логгера, а нужно ли?
    // TODO: Стоит сделать клиента подходящего под данное тестирование
    // чтобы не править если что всё в двух местах
    for (size_t i = 0; i < config.concurrency; i++) {
        size_t start = i * requests_per_worker;
        size_t end = (i == config.concurrency - 1) ? config.total_requests : start + requests_per_worker;
        workers.emplace_back(loadTestWorker, i, start, end, config.udp_port, std::ref(results));
    }

    // Ждём завершения всех рабочих потоков
    for (auto& worker : workers) {
        worker.join();
    }

    auto end_time = std::chrono::steady_clock::now();
    results.total_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

    // Останавливаем сервер
    //server.stop();
    //serverFuture.get();

    // Выводим результаты
    printStatistics(results, config);

    // Проверки
    if (results.error_count != 0) {
        std::cerr << "\nERROR: " << results.error_count << " failed requests\n";
        return 1;
    }
    if (results.success_count != config.total_requests) {
        std::cerr << "\nERROR: Expected " << config.total_requests
                  << " successful requests, got " << results.success_count << "\n";
        return 1;
    }

    std::cout << "\nTEST PASSED\n";
    return 0;
}
