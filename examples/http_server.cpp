#include "../src/core/io_event.hpp"
#include "../src/core/mutex.hpp"
#include "../src/core/scheduler.hpp" // Added
#include "../src/core/thread.hpp"    // Added
#include "../src/runtime/runtime.hpp"
#include <arpa/inet.h>
#include <chrono> // Added
#include <cstdio>
#include <cstring>
#include <ctime> // Added
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>


using namespace std::chrono_literals;

// Global pointer used by the server thread entry function
// Global pointer definition moved below class definition

class HTTPServer {
public:
  using Handler = std::function<std::string(
      const std::string &,
      const std::unordered_map<std::string, std::string> &)>;

  HTTPServer(int port) : port_(port), server_fd_(-1), running_(false) {}

  ~HTTPServer() { stop(); }

  void add_route(const std::string &path, Handler handler) {
    routes_[path] = std::move(handler);
  }

  void start() {
    if (running_)
      return;

    // Create server socket
    server_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_fd_ == -1) {
      perror("socket");
      return;
    }

    // Set SO_REUSEADDR
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
        -1) {
      perror("setsockopt");
      close(server_fd_);
      return;
    }

    // Bind
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      perror("bind");
      close(server_fd_);
      return;
    }

    // Listen
    if (listen(server_fd_, SOMAXCONN) == -1) {
      perror("listen");
      close(server_fd_);
      return;
    }

    printf("HTTP server listening on port %d\n", port_);

    // Register server socket with IO event system
    auto &io = IOEvent::instance();
    io.register_event(
        server_fd_, IOEvent::Type::READ,
        [this](int fd, IOEvent::Type) { handle_new_connection(); });

    running_ = true;
  }

  void stop() {
    if (!running_)
      return;

    printf("Stopping HTTP server...\n");

    // Close all client connections
    for (int client_fd : client_fds_) {
      close(client_fd);
    }
    client_fds_.clear();

    // Close server socket
    if (server_fd_ != -1) {
      close(server_fd_);
      server_fd_ = -1;
    }

    running_ = false;
  }

  bool is_running() const { return running_; }

private:
  void handle_new_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (true) {
      int client_fd = accept4(server_fd_, (struct sockaddr *)&client_addr,
                              &client_addr_len, SOCK_NONBLOCK);

      if (client_fd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          // No more connections to accept
          break;
        } else {
          perror("accept4");
          break;
        }
      }

      char client_ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
      printf("New connection from %s:%d\n", client_ip,
             ntohs(client_addr.sin_port));

      // Add to client list
      client_fds_.push_back(client_fd);

      // Register client socket for reading
      auto &io = IOEvent::instance();
      io.register_event(client_fd, IOEvent::Type::READ,
                        [this, client_fd](int fd, IOEvent::Type type) {
                          handle_client_request(fd);
                        });
    }
  }

  void handle_client_request(int client_fd) {
    char buffer[4096];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
      // Connection closed or error
      if (bytes_read == 0) {
        printf("Client disconnected\n");
      } else {
        perror("recv");
      }

      // Clean up
      close(client_fd);
      client_fds_.erase(
          std::remove(client_fds_.begin(), client_fds_.end(), client_fd),
          client_fds_.end());
      return;
    }

    // Null-terminate the request
    buffer[bytes_read] = '\0';

    // Parse HTTP request
    std::string request(buffer, bytes_read);
    std::istringstream request_stream(request);
    std::string method, path, version;
    request_stream >> method >> path >> version;

    // Parse headers (simplified)
    std::unordered_map<std::string, std::string> headers;
    std::string header_line;
    while (std::getline(request_stream, header_line) && header_line != "\r") {
      size_t colon_pos = header_line.find(':');
      if (colon_pos != std::string::npos) {
        std::string key = header_line.substr(0, colon_pos);
        std::string value = header_line.substr(colon_pos + 2); // Skip ": "
        if (!value.empty() && value.back() == '\r') {
          value.pop_back();
        }
        headers[key] = value;
      }
    }

    // Find a matching route
    auto it = routes_.find(path);
    if (it != routes_.end()) {
      // Call the handler
      std::string response_body = it->second(path, headers);

      // Send HTTP response
      std::ostringstream response;
      response << "HTTP/1.1 200 OK\r\n";
      response << "Content-Type: text/plain\r\n";
      response << "Content-Length: " << response_body.length() << "\r\n";
      response << "Connection: close\r\n";
      response << "\r\n";
      response << response_body;

      std::string response_str = response.str();
      send(client_fd, response_str.c_str(), response_str.length(), 0);
    } else {
      // 404 Not Found
      const char *not_found = "HTTP/1.1 404 Not Found\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: 9\r\n"
                              "Connection: close\r\n"
                              "\r\n"
                              "Not Found";
      send(client_fd, not_found, strlen(not_found), 0);
    }

    // Close the connection (for simplicity)
    close(client_fd);
    client_fds_.erase(
        std::remove(client_fds_.begin(), client_fds_.end(), client_fd),
        client_fds_.end());
  }

  int port_;
  int server_fd_;
  bool running_;
  std::vector<int> client_fds_;
  std::unordered_map<std::string, Handler> routes_;
};

// Global pointer used by the server thread entry function
static HTTPServer *g_http_server = nullptr;

// Entry function for the HTTP server green thread
void http_server_thread_entry() {
  if (!g_http_server) {
    return;
  }
  g_http_server->start();

  // Run the event loop
  auto &io = IOEvent::instance();
  io.run();
}

int main() {
  // Initialize the runtime
  runtime_init();

  printf("=== Green Thread HTTP Server ===\n");

  // Create and configure HTTP server
  HTTPServer server(8080);
  g_http_server = &server;

  // Add some routes
  server.add_route("/", [](const std::string &, const auto &) {
    return "Hello from Green Thread HTTP Server!\n";
  });

  server.add_route("/time", [](const std::string &, const auto &) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S",
                  std::localtime(&time));
    return std::string("Current time: ") + time_str + "\n";
  });

  // Start the server in a separate thread
  Thread *server_thread = new Thread(&http_server_thread_entry);

  server_thread->set_name("HTTP Server");
  Scheduler::schedule(server_thread);

  printf("Server started at http://localhost:8080/\n");
  printf("Press Ctrl+C to stop...\n");

  // Run the scheduler
  runtime_run();

  // Clean up
  server.stop();
  delete server_thread;

  printf("Server stopped.\n");
  return 0;
}
