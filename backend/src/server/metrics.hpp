#ifndef SERVER_METRICS_HPP
#define SERVER_METRICS_HPP

#include <string>

namespace server::metrics {

void initialize();
void record_http_request();
void record_http_error();
void record_db_error();
void record_api_request(const std::string& model, const std::string& operation,
                        const std::string& method, int status_code);
void record_api_error(const std::string& model, const std::string& operation,
                      const std::string& error_code, int status_code);
void observe_api_request_duration(const std::string& model, const std::string& operation,
                                  const std::string& method, int status_code,
                                  double duration_seconds);

} // namespace server::metrics

#endif // SERVER_METRICS_HPP
