#include "s3_uploader.hpp"

#include <curl/curl.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace {

auto trim_trailing_slash(std::string path) -> std::string {
    while (!path.empty() && path.back() == '/') {
        path.pop_back();
    }
    return path;
}

auto uri_encode(std::string_view input, bool encode_slash) -> std::string {
    std::ostringstream oss;
    oss << std::uppercase << std::hex;

    for (unsigned char ch : input) {
        const bool unreserved = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                                (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' ||
                                ch == '~';

        if (unreserved || (!encode_slash && ch == '/')) {
            oss << static_cast<char>(ch);
        } else {
            oss << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(ch);
        }
    }

    return oss.str();
}

auto write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
    auto* out = static_cast<std::string*>(userdata);
    const size_t total = size * nmemb;
    out->append(ptr, total);
    return total;
}

auto build_public_object_url(const S3Config& config, const std::string& object_key) -> std::string {
    const std::string endpoint = trim_trailing_slash(config.endpoint);
    if (endpoint.empty()) {
        throw S3UploadError("Missing S3 endpoint");
    }

    return endpoint + "/" + uri_encode(config.bucket, true) + "/" + uri_encode(object_key, false);
}

} // namespace

S3Uploader::S3Uploader(S3Config config)
    : config_(std::move(config)) {
}

auto S3Uploader::upload_user_avatar(int user_id, const std::string& payload,
                                    const std::string& content_type) const -> std::string {
    if (config_.bucket.empty() || config_.endpoint.empty()) {
        throw S3UploadError("Incomplete S3 configuration");
    }

    const std::string object_key = "avatars/user_" + std::to_string(user_id) + "/avatar";
    const std::string request_url = build_public_object_url(config_, object_key);

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw S3UploadError("Failed to initialize CURL");
    }

    std::string response_body;
    struct curl_slist* headers = nullptr;

    headers = curl_slist_append(headers, ("Content-Type: " + content_type).c_str());
    headers = curl_slist_append(headers, "Expect:");

    curl_easy_setopt(curl, CURLOPT_URL, request_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(payload.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "chronos-backend/1.0");

    const CURLcode result = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw S3UploadError(std::string("S3 upload failed: ") + curl_easy_strerror(result));
    }

    if (http_code < 200 || http_code >= 300) {
        throw S3UploadError("S3 upload returned HTTP " + std::to_string(http_code) + ": " +
                            response_body);
    }

    return object_key;
}
