#include <BS_thread_pool.hpp>
#include <argparse/argparse.hpp>
#include <atomic>
#include <cmath>
#include <cstddef> // std::size_t
#include <cstdint> // std::uintmax_t
#include <filesystem>
#include <functional> // std::ref, std::cref
#include <iostream>
#include <ranges>
#include <stdexcept> // std::runtime_error
#include <string_view>
#include <system_error> // std::error_code
#include <thread> // std::thread
#include <vector> // std::vector

namespace fs = std::filesystem;

void get_rec_dir_size(
    fs::path const &path,
    std::atomic<std::uintmax_t> &total_size,
    BS::thread_pool<BS::tp::none> &pool) {
  std::error_code ec;

  // if the path is not to a directory, check if it's a regular file and return
  // its size
  {
    auto const status = fs::symlink_status(path);
    if (status.type() != fs::file_type::directory) {
      if (status.type() == fs::file_type::regular) {
        std::uintmax_t const file_size{fs::file_size(path, ec)};
        if (!ec) {
          total_size += file_size;
        }
        // else {
        //   std::cerr << ec.message() << " (" << path << ") (1)\n";
        // }
      }
      return;
    }
  }

  for (auto const &item : fs::directory_iterator(path, ec)) {
    if (ec) {
      // std::cerr << ec.message() << " (" << path << ") (2)\n";
      break;
    }

    auto const &status = item.symlink_status(ec);
    if (ec) {
      // std::cerr << ec.message() << " (" << dir_it->path() << ") (3)\n";
      ec.clear();
      continue;
    }

    switch (status.type()) {
    case fs::file_type::regular: {
      std::uintmax_t const file_size{item.file_size(ec)};
      if (ec) {
        // std::cerr << ec.message() << " (" << dir_it->path() << ") (4)\n";
        ec.clear();
        continue;
      }
      total_size += file_size;
      break;
    }
    case fs::file_type::directory: {
      pool.detach_task([path = item.path(), &total_size, &pool] {
        get_rec_dir_size(std::cref(path), std::ref(total_size), std::ref(pool));
      });
      break;
    }
    case fs::file_type::symlink:
    case fs::file_type::none:
    case fs::file_type::not_found:
    case fs::file_type::block:
    case fs::file_type::character:
    case fs::file_type::fifo:
    case fs::file_type::socket:
    case fs::file_type::unknown:
      break;
    }
  }
}

class HumanReadable {
public:
  std::uintmax_t m_size{};

private:
  friend std::ostream &operator<<(std::ostream &os, HumanReadable const &hr) {
    static constexpr double two_to_ten = 1024.0;
    static constexpr double ten_to_one = 10.0;
    int o{};
    auto mantissa = static_cast<double>(hr.m_size);
    while (mantissa >= two_to_ten) {
      mantissa /= two_to_ten;
      ++o;
    }
    os << std::ceil(mantissa * ten_to_one) / ten_to_one << "BKMGTPE"[o];
    return o != 0 ? (os << "B") : os;
  }
};

class Args {
public:
  bool human_readable;
  unsigned int num_threads;
  std::vector<fs::path> paths;
};

int main(int argc, char const *const *argv) {
  argparse::ArgumentParser program("mt_du");
  program.add_argument("-H", "--human-readable")
      .help("display the sizes in a human-readable format")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-j", "--threads")
      .help("number of threads to use")
      .default_value(std::thread::hardware_concurrency())
      .scan<'u', unsigned int>();
  program.add_argument("paths")
      .help("the list of paths for which to print their disk size")
      .nargs(argparse::nargs_pattern::at_least_one);

  try {
    program.parse_args(argc, argv);
  } catch (std::runtime_error const &err) {
    std::cerr << err.what() << '\n';
    return 1;
  }

  Args args{
      program.get<bool>("--human-readable"),
      program.get<unsigned int>("-j"),
      program.get<std::vector<std::string>>("paths")
          | std::ranges::to<std::vector<fs::path>>()};

  std::vector<std::atomic<std::uintmax_t>> sizes(args.paths.size());

  BS::thread_pool pool(args.num_threads);
  for (std::size_t i = 0; i < args.paths.size(); ++i) {
    pool.detach_task([path = args.paths[i], &size = sizes[i], &pool] {
      get_rec_dir_size(std::cref(path), std::ref(size), std::ref(pool));
    });
  }
  pool.wait();

  for (std::size_t i = 0; i < args.paths.size(); ++i) {
    auto path = args.paths[i].string();
    std::cout << path << ' ';
    if (args.human_readable) {
      std::cout << HumanReadable(sizes[i]) << '\n';
    } else {
      std::cout << sizes[i] << '\n';
    }
  }

  return 0;
}
