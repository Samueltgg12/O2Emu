#pragma once

/**
 * @file nfs_server.h
 * @brief Minimal read-only NFSv2 server for the host share folder
 *
 * Serves the configured share folder to the emulated O2 over the private
 * NAT network. Runs on the host loopback (127.0.0.1): the mount protocol
 * on UDP port 2048 and NFS on UDP port 2049, both unprivileged. The guest
 * reaches them at the NAT gateway address.
 *
 * Read-only: write operations return NFSERR_ROFS. Mount inside IRIX with:
 *   mount -o port=2049,mountport=2048 <gateway>:/ /mnt
 */

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

namespace o2emu::system {

class NfsServer {
public:
  NfsServer();
  ~NfsServer();

  NfsServer(const NfsServer &) = delete;
  NfsServer &operator=(const NfsServer &) = delete;

  /// Start serving root_path on 127.0.0.1 (mount: 2048, nfs: 2049).
  bool start(const std::string &root_path);
  void stop();
  bool running() const { return running_; }

  const std::string &last_error() const { return last_error_; }

  static constexpr uint16_t kMountPort = 2048;
  static constexpr uint16_t kNfsPort = 2049;

private:
  void thread_main();

  std::string root_;
  std::string last_error_;
  std::thread thread_;
  std::atomic<bool> stop_requested_{false};
  bool running_ = false;
  int mount_fd_ = -1;
  int nfs_fd_ = -1;
};

} // namespace o2emu::system
