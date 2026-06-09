#include "metrics.hpp"

#include <prometheus/counter.h>
#include <prometheus/histogram.h>
#include <string>

namespace server::metrics {

namespace {

prometheus::counter_metric_t& http_requests_counter() {
    static prometheus::counter_metric_t counter("chronos_http_requests_total",
                                                "Total HTTP requests handled by Chronos");
    return counter;
}

prometheus::counter_metric_t& http_errors_counter() {
    static prometheus::counter_metric_t counter("chronos_http_errors_total",
                                                "Total HTTP responses with status >= 400");
    return counter;
}

prometheus::counter_metric_t& db_errors_counter() {
    static prometheus::counter_metric_t counter("chronos_db_errors_total",
                                                "Total database errors returned by Chronos");
    return counter;
}

prometheus::counter_metric_t api_requests_counter(const std::string& model,
                                                  const std::string& operation,
                                                  const std::string& method, int status_code) {
    return prometheus::counter_metric_t(
        "chronos_api_requests_total", "Total API responses by model, operation, method and status",
        {{"model", model},
         {"operation", operation},
         {"method", method},
         {"status_code", std::to_string(status_code)}});
}

prometheus::counter_metric_t api_errors_counter(const std::string& model,
                                                const std::string& operation,
                                                const std::string& error_code, int status_code) {
    return prometheus::counter_metric_t(
        "chronos_api_errors_total", "Total API error responses by model, operation and error code",
        {{"model", model},
         {"operation", operation},
         {"error_code", error_code},
         {"status_code", std::to_string(status_code)}});
}

prometheus::histogram_metric_t api_request_duration_histogram(const std::string& model,
                                                              const std::string& operation,
                                                              const std::string& method,
                                                              int status_code) {
    return prometheus::histogram_metric_t(
        "chronos_api_request_duration_seconds", "API request duration by model and operation",
        {{"model", model},
         {"operation", operation},
         {"method", method},
         {"status_code", std::to_string(status_code)}},
        {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0});
}

} // namespace

void initialize() {
    (void) http_requests_counter();
    (void) http_errors_counter();
    (void) db_errors_counter();
}

void record_http_request() {
    auto& counter = http_requests_counter();
    counter++;
}

void record_http_error() {
    auto& counter = http_errors_counter();
    counter++;
}

void record_db_error() {
    auto& counter = db_errors_counter();
    counter++;
}

void record_api_request(const std::string& model, const std::string& operation,
                        const std::string& method, int status_code) {
    auto counter = api_requests_counter(model, operation, method, status_code);
    counter++;
}

void record_api_error(const std::string& model, const std::string& operation,
                      const std::string& error_code, int status_code) {
    auto counter = api_errors_counter(model, operation, error_code, status_code);
    counter++;
}

void observe_api_request_duration(const std::string& model, const std::string& operation,
                                  const std::string& method, int status_code,
                                  double duration_seconds) {
    auto histogram = api_request_duration_histogram(model, operation, method, status_code);
    histogram.Observe(duration_seconds);
}

} // namespace server::metrics
