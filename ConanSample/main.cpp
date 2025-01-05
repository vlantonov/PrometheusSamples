#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <prometheus/summary.h>
#include <prometheus/text_serializer.h>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <iostream>
#include <thread>
#include <vector>

// using namespace prometheus;

namespace beast = boost::beast;  // from <boost/beast.hpp>
namespace http = beast::http;    // from <boost/beast/http.hpp>
namespace net = boost::asio;     // from <boost/asio.hpp>
using tcp = net::ip::tcp;        // from <boost/asio/ip/tcp.hpp>

std::atomic_bool running{true};
std::atomic_bool serverRunning{true};

void shutdown_handler(int) { running = false; }

int main() {
  if (signal(SIGTERM | SIGKILL, shutdown_handler) == SIG_ERR) {
    return EXIT_FAILURE;
  }

  const auto host = "127.0.0.1";
  const auto port = "9998";
  const auto target = "/metrics";
  const int version = 11;

  // These objects perform our I/O
  net::io_context ioc;
  tcp::resolver resolver(ioc);
  beast::tcp_stream stream(ioc);

  auto reg = std::make_shared<prometheus::Registry>();

  std::cout << "Starting metric client...\n";

  auto &rpc_duration_family = prometheus::BuildSummary()
                                  .Name("rpc_duration")
                                  .Help("Duration of the request")
                                  .Register(*reg);
  auto &rpc_duration = rpc_duration_family.Add(
      {}, std::vector<prometheus::detail::CKMSQuantiles::Quantile>{
              {.01, .001},  //  1%
              {.05, .001},  //  5%
              {.5, .001},   // 50%
              {.90, .001},  // 90%
              {.99, .001},  // 99%
          });

  auto &some_histo_family = prometheus::BuildHistogram()
                                .Name("request_time")
                                .Help("How long the response took.")
                                .Register(*reg);
  auto &some_histo = some_histo_family.Add(
      {},
      prometheus::Histogram::BucketBoundaries{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});

  auto &request_family = prometheus::BuildCounter()
                             .Name("total_requests")
                             .Help("Total number of requests")
                             .Register(*reg);
  auto &request_counter = request_family.Add({{"somekey", "somevalue"}});

  std::string endpoint = "127.0.0.1:9998";
  auto exposer = std::make_unique<prometheus::Exposer>(endpoint);

  exposer->RegisterCollectable(reg);

  // Look up the domain name
  auto const results = resolver.resolve(host, port);

  std::jthread worker([&]() {
    while (running) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    serverRunning = false;
  });

  std::cout << "Starting http server...\n";
  while (serverRunning) {
    // Make the connection on the IP address we get from a lookup
    stream.connect(results);

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, target, version};

    // Send the HTTP request to the remote host
    http::write(stream, req);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    http::read(stream, buffer, res);

    // Write the message to standard out
    std::cout << res << std::endl;

    // time how long this request takes to serve
    rpc_duration.Observe([&]() {
      auto startPoint = std::chrono::steady_clock::now();
      request_counter.Increment();

      // some random data
      some_histo.Observe(rand() % 10);

      auto endPoint = std::chrono::steady_clock::now();
      return std::chrono::duration_cast<std::chrono::milliseconds>(endPoint -
                                                                   startPoint)
          .count();
    }());

    prometheus::TextSerializer textSerializer;

    // Serialize all metrics
    std::cout << "Serialized: " << textSerializer.Serialize(reg->Collect())
              << '\n';

    // Gracefully close the socket
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
  }

  // Stop the thread
  running = false;

  std::cout << "Done.\n";
  return 0;
}
