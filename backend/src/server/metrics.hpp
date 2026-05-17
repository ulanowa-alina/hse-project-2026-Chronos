#ifndef SERVER_METRICS_HPP
#define SERVER_METRICS_HPP

namespace server::metrics {

void initialize();
void record_http_request();
void record_http_error();
void record_db_error();

} // namespace server::metrics

#endif // SERVER_METRICS_HPP
