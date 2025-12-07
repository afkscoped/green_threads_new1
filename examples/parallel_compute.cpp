#include "../src/core/task.hpp"
#include "../src/runtime/runtime.hpp"
#include <iostream>
#include <vector>
#include <numeric>
#include <chrono>
#include <cmath>

using namespace std::chrono;

// A CPU-intensive function to test parallel execution
double compute_value(int x) {
    double result = 0.0;
    for (int i = 0; i < 1000; ++i) {
        result += std::sqrt(std::sin(x) * std::sin(x) + std::cos(x) * std::cos(x));
    }
    return result;
}

// Sequential computation
std::vector<double> compute_sequential(const std::vector<int>& input) {
    std::vector<double> result;
    result.reserve(input.size());
    
    for (int x : input) {
        result.push_back(compute_value(x));
    }
    
    return result;
}

// Parallel computation using tasks
std::vector<double> compute_parallel(const std::vector<int>& input) {
    std::vector<double> result(input.size());
    std::vector<std::future<double>> futures;
    
    // Submit tasks in parallel
    for (size_t i = 0; i < input.size(); ++i) {
        futures.push_back(async([&input, i, &result] {
            return compute_value(input[i]);
        }));
    }
    
    // Wait for all tasks to complete
    for (size_t i = 0; i < futures.size(); ++i) {
        result[i] = futures[i].get();
    }
    
    return result;
}

// Parallel for loop example
std::vector<double> compute_parallel_for(const std::vector<int>& input) {
    std::vector<double> result(input.size());
    
    parallel_for(input, [&result, &input](int x) {
        result[&x - input.data()] = compute_value(x);
    });
    
    return result;
}

// Check if two vectors are approximately equal
bool verify_results(const std::vector<double>& a, const std::vector<double>& b, double epsilon = 1e-10) {
    if (a.size() != b.size()) return false;
    
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::abs(a[i] - b[i]) > epsilon) {
            std::cerr << "Mismatch at index " << i << ": " << a[i] << " != " << b[i] << std::endl;
            return false;
        }
    }
    
    return true;
}

int main() {
    // Initialize the runtime
    runtime_init();
    
    // Generate some input data
    const int N = 10000;
    std::vector<int> input(N);
    std::iota(input.begin(), input.end(), 0);
    
    std::cout << "=== Parallel Computation Example ===\n";
    std::cout << "Input size: " << N << " elements\n";
    
    // Sequential computation
    std::cout << "\nRunning sequential computation..." << std::flush;
    auto start = high_resolution_clock::now();
    auto seq_result = compute_sequential(input);
    auto seq_time = duration_cast<milliseconds>(high_resolution_clock::now() - start).count();
    std::cout << " Done in " << seq_time << " ms\n";
    
    // Parallel computation
    std::cout << "Running parallel computation..." << std::flush;
    start = high_resolution_clock::now();
    auto par_result = compute_parallel(input);
    auto par_time = duration_cast<milliseconds>(high_resolution_clock::now() - start).count();
    std::cout << " Done in " << par_time << " ms";
    std::cout << " (Speedup: " << static_cast<double>(seq_time) / par_time << "x)\n";
    
    // Verify results
    if (!verify_results(seq_result, par_result)) {
        std::cerr << "Error: Parallel computation produced incorrect results!\n";
        return 1;
    }
    
    // Parallel for loop
    std::cout << "\nRunning parallel for loop..." << std::flush;
    start = high_resolution_clock::now();
    auto par_for_result = compute_parallel_for(input);
    auto par_for_time = duration_cast<milliseconds>(high_resolution_clock::now() - start).count();
    std::cout << " Done in " << par_for_time << " ms";
    std::cout << " (Speedup: " << static_cast<double>(seq_time) / par_for_time << "x)\n";
    
    // Verify results
    if (!verify_results(seq_result, par_for_result)) {
        std::cerr << "Error: Parallel for loop produced incorrect results!\n";
        return 1;
    }
    
    // Task-based parallel computation with dependencies
    std::cout << "\nRunning task-based computation with dependencies..." << std::flush;
    start = high_resolution_clock::now();
    
    // Create a chain of dependent tasks
    auto task1 = async([&input] {
        std::vector<double> result;
        result.reserve(input.size() / 2);
        for (size_t i = 0; i < input.size() / 2; ++i) {
            result.push_back(compute_value(input[i]));
        }
        return result;
    });
    
    auto task2 = async([&input] {
        std::vector<double> result;
        result.reserve(input.size() / 2);
        for (size_t i = input.size() / 2; i < input.size(); ++i) {
            result.push_back(compute_value(input[i]));
        }
        return result;
    });
    
    // Final task that combines results
    auto final_task = async([task1 = std::move(task1), task2 = std::move(task2)]() mutable {
        auto result1 = task1.get();
        auto result2 = task2.get();
        
        std::vector<double> combined;
        combined.reserve(result1.size() + result2.size());
        combined.insert(combined.end(), result1.begin(), result1.end());
        combined.insert(combined.end(), result2.begin(), result2.end());
        return combined;
    });
    
    auto task_result = final_task.get();
    auto task_time = duration_cast<milliseconds>(high_resolution_clock::now() - start).count();
    
    std::cout << " Done in " << task_time << " ms";
    std::cout << " (Speedup: " << static_cast<double>(seq_time) / task_time << "x)\n";
    
    // Verify results
    if (!verify_results(seq_result, task_result)) {
        std::cerr << "Error: Task-based computation produced incorrect results!\n";
        return 1;
    }
    
    std::cout << "\nAll tests completed successfully!\n";
    
    // Shut down the task scheduler
    TaskScheduler::instance().stop();
    
    return 0;
}
