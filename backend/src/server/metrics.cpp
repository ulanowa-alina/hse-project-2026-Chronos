#include "metrics.hpp"

#include <prometheus/counter.h>

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

} // namespace server::metrics
