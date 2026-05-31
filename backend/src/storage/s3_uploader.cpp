#include "s3_uploader.hpp"

#include <array>
#include <boost/url.hpp>
#include <ctime>
#include <curl/curl.h>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace {

struct ParsedEndpoint {
    std::string scheme;
    std::string host;
    std::string port;
    std::string base_path;
};

struct PreparedRequest {
    std::string url;
    std::string host_header;
    std::string canonical_uri;
};

struct SignedRequest {
    std::string authorization;
    std::string amz_date;
    std::string payload_hash;
};

auto bytes_to_hex(std::string_view input) -> std::string {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char ch : input) {
        oss << std::setw(2) << static_cast<int>(ch);
    }
    return oss.str();
}

auto sha256_hex(std::string_view input) -> std::string {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
    SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), digest.data());
    return bytes_to_hex(
        std::string_view(reinterpret_cast<const char*>(digest.data()), digest.size()));
}

auto hmac_sha256(std::string_view key, std::string_view data) -> std::string {
    unsigned int digest_len = 0;
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};

    HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(data.data()), data.size(), digest.data(),
         &digest_len);

    return std::string(reinterpret_cast<const char*>(digest.data()), digest_len);
}

auto current_amz_timestamp() -> std::pair<std::string, std::string> {
    const std::time_t now = std::time(nullptr);
    const std::tm utc_tm = *std::gmtime(&now);

    std::ostringstream full;
    full << std::put_time(&utc_tm, "%Y%m%dT%H%M%SZ");

    std::ostringstream short_date;
    short_date << std::put_time(&utc_tm, "%Y%m%d");

    return {full.str(), short_date.str()};
}

auto parse_endpoint(const std::string& endpoint) -> ParsedEndpoint {
    const auto parsed = boost::urls::parse_uri(endpoint);
    if (!parsed) {
        throw S3UploadError("Invalid S3 endpoint");
    }

    const auto url = parsed.value();

    ParsedEndpoint out;
    out.scheme = std::string(url.scheme());
    out.host = std::string(url.host());
    out.port = url.has_port() ? std::string(url.port()) : "";
    out.base_path = std::string(url.encoded_path());
    if (out.base_path.empty()) {
        out.base_path = "/";
    }
    return out;
}

auto trim_trailing_slash(std::string path) -> std::string {
    while (path.size() > 1 && path.back() == '/') {
        path.pop_back();
    }
    return path;
}

auto join_path(std::string_view lhs, std::string_view rhs) -> std::string {
    if (lhs.empty() || lhs == "/") {
        return "/" + std::string(rhs);
    }

    if (lhs.back() == '/') {
        return std::string(lhs) + std::string(rhs);
    }

    return std::string(lhs) + "/" + std::string(rhs);
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

auto prepare_request(const S3Config& config, const std::string& key) -> PreparedRequest {
    const ParsedEndpoint endpoint = parse_endpoint(config.endpoint);
    const std::string encoded_key = uri_encode(key, false);
    const std::string base_path = trim_trailing_slash(endpoint.base_path);
    const std::string port_suffix = endpoint.port.empty() ? "" : (":" + endpoint.port);

    PreparedRequest request;

    if (config.use_path_style) {
        request.host_header = endpoint.host + port_suffix;
        request.canonical_uri =
            join_path(join_path(base_path, uri_encode(config.bucket, true)), encoded_key);
        request.url = endpoint.scheme + "://" + request.host_header + request.canonical_uri;
    } else {
        request.host_header = config.bucket + "." + endpoint.host + port_suffix;
        request.canonical_uri = join_path(base_path, encoded_key);
        request.url = endpoint.scheme + "://" + request.host_header + request.canonical_uri;
    }

    return request;
}

auto build_signing_key(const std::string& secret_key, const std::string& short_date,
                       const std::string& region) -> std::string {
    const std::string k_date = hmac_sha256("AWS4" + secret_key, short_date);
    const std::string k_region = hmac_sha256(k_date, region);
    const std::string k_service = hmac_sha256(k_region, "s3");
    return hmac_sha256(k_service, "aws4_request");
}

auto write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
    auto* out = static_cast<std::string*>(userdata);
    const size_t total = size * nmemb;
    out->append(ptr, total);
    return total;
}

auto build_signed_request(const S3Config& config, const PreparedRequest& request,
                          std::string_view method, std::string_view payload,
                          std::string_view canonical_headers, std::string_view signed_headers,
                          const std::string& amz_date,
                          const std::string& short_date) -> SignedRequest {
    const std::string payload_hash = sha256_hex(payload);

    const std::string canonical_request = std::string(method) + "\n" + request.canonical_uri +
                                          "\n\n" + std::string(canonical_headers) + "\n" +
                                          std::string(signed_headers) + "\n" + payload_hash;

    const std::string credential_scope = short_date + "/" + config.region + "/s3/aws4_request";
    const std::string string_to_sign = "AWS4-HMAC-SHA256\n" + amz_date + "\n" + credential_scope +
                                       "\n" + sha256_hex(canonical_request);

    const std::string signing_key = build_signing_key(config.secret_key, short_date, config.region);
    const std::string signature = bytes_to_hex(hmac_sha256(signing_key, string_to_sign));

    SignedRequest signed_request;
    signed_request.authorization =
        "AWS4-HMAC-SHA256 Credential=" + config.access_key + "/" + credential_scope +
        ", SignedHeaders=" + std::string(signed_headers) + ", Signature=" + signature;
    signed_request.amz_date = amz_date;
    signed_request.payload_hash = payload_hash;
    return signed_request;
}

} // namespace

S3Uploader::S3Uploader(S3Config config)
    : config_(std::move(config)) {
}

auto S3Uploader::upload_user_avatar(int user_id, const std::string& payload,
                                    const std::string& content_type) const -> std::string {
    if (config_.bucket.empty() || config_.region.empty() || config_.endpoint.empty() ||
        config_.access_key.empty() || config_.secret_key.empty()) {
        throw S3UploadError("Incomplete S3 configuration");
    }

    const std::string object_key = "avatars/user_" + std::to_string(user_id) + "/avatar";
    const PreparedRequest request = prepare_request(config_, object_key);
    const auto [amz_date, short_date] = current_amz_timestamp();
    const std::string payload_hash = sha256_hex(payload);
    const std::string signed_headers =
        "content-type;host;x-amz-content-sha256;x-amz-date;x-amz-server-side-encryption";

    const std::string canonical_headers =
        "content-type:" + content_type + "\n" + "host:" + request.host_header + "\n" +
        "x-amz-content-sha256:" + payload_hash + "\n" + "x-amz-date:" + amz_date + "\n" +
        "x-amz-server-side-encryption:AES256\n";
    const SignedRequest signed_request = build_signed_request(
        config_, request, "PUT", payload, canonical_headers, signed_headers, amz_date, short_date);

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw S3UploadError("Failed to initialize CURL");
    }

    std::string response_body;
    struct curl_slist* headers = nullptr;

    headers = curl_slist_append(headers, ("Content-Type: " + content_type).c_str());
    headers = curl_slist_append(headers, ("Host: " + request.host_header).c_str());
    headers = curl_slist_append(headers,
                                ("x-amz-content-sha256: " + signed_request.payload_hash).c_str());
    headers = curl_slist_append(headers, ("x-amz-date: " + signed_request.amz_date).c_str());
    headers = curl_slist_append(headers, "x-amz-server-side-encryption: AES256");
    headers =
        curl_slist_append(headers, ("Authorization: " + signed_request.authorization).c_str());
    headers = curl_slist_append(headers, "Expect:");

    curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());
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

void S3Uploader::delete_object(const std::string& object_key) const {
    if (config_.bucket.empty() || config_.region.empty() || config_.endpoint.empty() ||
        config_.access_key.empty() || config_.secret_key.empty()) {
        throw S3UploadError("Incomplete S3 configuration");
    }

    const PreparedRequest request = prepare_request(config_, object_key);
    const auto [amz_date, short_date] = current_amz_timestamp();
    const std::string signed_headers = "host;x-amz-content-sha256;x-amz-date";
    const std::string empty_payload;
    const std::string payload_hash = sha256_hex(empty_payload);
    const std::string canonical_headers = "host:" + request.host_header + "\n" +
                                          "x-amz-content-sha256:" + payload_hash + "\n" +
                                          "x-amz-date:" + amz_date + "\n";
    const SignedRequest signed_request =
        build_signed_request(config_, request, "DELETE", empty_payload, canonical_headers,
                             signed_headers, amz_date, short_date);

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw S3UploadError("Failed to initialize CURL");
    }

    std::string response_body;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Host: " + request.host_header).c_str());
    headers = curl_slist_append(headers,
                                ("x-amz-content-sha256: " + signed_request.payload_hash).c_str());
    headers = curl_slist_append(headers, ("x-amz-date: " + signed_request.amz_date).c_str());
    headers =
        curl_slist_append(headers, ("Authorization: " + signed_request.authorization).c_str());
    headers = curl_slist_append(headers, "Expect:");

    curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "chronos-backend/1.0");

    const CURLcode result = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw S3UploadError(std::string("S3 delete failed: ") + curl_easy_strerror(result));
    }

    if (http_code < 200 || http_code >= 300) {
        throw S3UploadError("S3 delete returned HTTP " + std::to_string(http_code) + ": " +
                            response_body);
    }
}
