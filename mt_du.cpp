#include "thread-pool/include/BS_thread_pool.hpp"
#include <atomic>
#include <filesystem>
#include <forward_list>
#include <future>
#include <iostream>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
static std::error_code const no_error;

void
get_rec_dir_size(fs::path const &path,
                 std::atomic<std::uintmax_t> &total_size,
                 BS::thread_pool &pool) {
  std::error_code ec;
  for (auto dir_it = fs::directory_iterator(path, ec);
       dir_it != fs::directory_iterator();
       ++dir_it) {
    if (ec != no_error) {
      std::cerr << ec.message() << " (" << path << ")\n";
      break;
    }

    auto const &status = dir_it->symlink_status(ec);
    if (ec != no_error) {
      std::cerr << ec.message() << " (" << dir_it->path() << ")\n";
      ec.clear();
      continue;
    }

    switch (status.type()) {
    case fs::file_type::regular: {
      std::uintmax_t const file_size{dir_it->file_size(ec)};
      if (ec != no_error) {
        std::cerr << ec.message() << " (" << dir_it->path() << ")\n";
        ec.clear();
        continue;
      }
      total_size += file_size;
      break;
    }
    case fs::file_type::directory: {
      pool.push_task(get_rec_dir_size,
                     dir_it->path(),
                     std::ref(total_size),
                     std::ref(pool));
      break;
    }
    case fs::file_type::symlink:
      break;
    case fs::file_type::none: {
      std::cerr << dir_it->path() << " has `not-evaluated-yet` type\n";
      break;
    }
    case fs::file_type::not_found: {
      std::cerr << dir_it->path() << " does not exist\n";
      break;
    }
    case fs::file_type::block: {
      std::cerr << dir_it->path() << " is a block device\n";
      break;
    }
    case fs::file_type::character: {
      std::cerr << dir_it->path() << " is a character device\n";
      break;
    }
    case fs::file_type::fifo: {
      std::cerr << dir_it->path() << " is a named IPC pipe\n";
      break;
    }
    case fs::file_type::socket: {
      std::cerr << dir_it->path() << " is a named IPC socket\n";
      break;
    }
    case fs::file_type::unknown: {
      std::cerr << dir_it->path() << " has `unknown` type\n";
      break;
    }
    }
  }
}

std::vector<fs::path>
get_paths(std::span<std::string_view const> args) {
  std::vector<fs::path> paths;
  paths.reserve(args.size() - 1);
  for (auto const &arg : std::ranges::drop_view{args, 1}) {
    paths.emplace_back(arg);
  }
  return paths;
}

int
main(int argc, char const *const *argv) {
  std::vector<std::string_view> args(
      argv,
      std::next(argv, static_cast<std::ptrdiff_t>(argc)));

  std::vector<fs::path> const paths = get_paths(args);
  std::vector<std::atomic<std::uintmax_t>> sizes(paths.size());

  BS::thread_pool pool;
  for (std::size_t i = 0; i < paths.size(); ++i) {
    pool.push_task(get_rec_dir_size,
                   std::cref(paths[i]),
                   std::ref(sizes[i]),
                   std::ref(pool));
  }

  pool.wait_for_tasks();

  for (std::size_t i = 0; i < paths.size(); ++i) {
    std::cout << paths[i].c_str() << ' ' << sizes[i] << '\n';
  }

  return 0;
}
