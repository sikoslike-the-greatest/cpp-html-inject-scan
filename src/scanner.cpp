#include "scanner.hpp"

#include "url_utils.hpp"

namespace his {

bool is_reflected(const std::string& body, const std::string& marker) {
  return !marker.empty() && body.find(marker) != std::string::npos;
}

ProbeResult probe(const std::string& base_url, const std::vector<std::string>& params,
                  const std::string& payload, const std::string& marker, const ResponseFn& send) {
  ProbeResult result;
  result.url = build_url_with_params(base_url, params, payload);
  const HttpResponse resp = send(result.url);
  result.status = resp.status;
  result.length = resp.length;
  result.reflected = is_reflected(resp.text, marker);
  return result;
}

std::size_t pack_batch(const std::string& base_url, const std::vector<std::string>& params,
                       std::size_t start, const std::string& payload, std::size_t max_url_len) {
  std::size_t count = 1;  // Always include at least one parameter.
  while (start + count < params.size()) {
    const std::vector<std::string> candidate(params.begin() + start,
                                              params.begin() + start + count + 1);
    if (build_url_with_params(base_url, candidate, payload).size() <= max_url_len) {
      ++count;
    } else {
      break;
    }
  }
  return count;
}

std::vector<std::string> find_reflected(const std::string& base_url,
                                        const std::vector<std::string>& batch,
                                        const std::string& payload, const std::string& marker,
                                        const ResponseFn& send) {
  if (batch.size() == 1) return batch;

  const std::size_t mid = batch.size() / 2;
  const std::vector<std::string> left(batch.begin(), batch.begin() + mid);
  const std::vector<std::string> right(batch.begin() + mid, batch.end());

  std::vector<std::string> found;
  const bool left_reflects = probe(base_url, left, payload, marker, send).reflected;
  if (left_reflects) {
    const auto in_left = find_reflected(base_url, left, payload, marker, send);
    found.insert(found.end(), in_left.begin(), in_left.end());
  }

  // If the left half does not reflect, the right half must (whole batch does),
  // so we can recurse without an extra probe.
  const bool right_reflects =
      left_reflects ? probe(base_url, right, payload, marker, send).reflected : true;
  if (right_reflects) {
    const auto in_right = find_reflected(base_url, right, payload, marker, send);
    found.insert(found.end(), in_right.begin(), in_right.end());
  }
  return found;
}

std::vector<ScanHit> scan_url(const std::string& base_url, const std::vector<std::string>& params,
                              const std::string& payload, const std::string& marker,
                              std::size_t max_url_len, const ResponseFn& send) {
  std::vector<ScanHit> hits;
  std::size_t i = 0;
  while (i < params.size()) {
    const std::size_t n = pack_batch(base_url, params, i, payload, max_url_len);
    const std::vector<std::string> batch(params.begin() + i, params.begin() + i + n);

    const ProbeResult pr = probe(base_url, batch, payload, marker, send);
    if (pr.reflected) {
      if (n == 1) {
        hits.push_back({batch[0], pr.url, pr.status, pr.length});
      } else {
        for (const auto& param : find_reflected(base_url, batch, payload, marker, send)) {
          const ProbeResult confirm = probe(base_url, {param}, payload, marker, send);
          hits.push_back({param, confirm.url, confirm.status, confirm.length});
        }
      }
    }
    i += n;
  }
  return hits;
}

}  // namespace his
