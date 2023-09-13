#include <BS_thread_pool.hpp>
#include <argparse/argparse.hpp>

#include <atomic>
#include <cmath>
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

void get_rec_dir_size(
    fs::path const &path,
    std::atomic<std::uintmax_t> &total_size,
    BS::thread_pool &pool) {
  std::error_code ec;

  {
    auto const status = fs::symlink_status(path);
    if (status.type() != fs::file_type::directory) {
      if (status.type() == fs::file_type::regular) {
        std::uintmax_t const file_size{fs::file_size(path, ec)};
        if (ec != no_error) {
          std::cerr << ec.message() << " (" << path << ")\n";
        } else {
          total_size += file_size;
        }
      }
      return;
    }
  }

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
      pool.push_task(
          get_rec_dir_size,
          dir_it->path(),
          std::ref(total_size),
          std::ref(pool));
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
    int o{};
    auto mantissa = static_cast<double>(hr.m_size);
    while (mantissa >= 1024.0) {
      mantissa /= 1024.0;
      ++o;
    }
    os << std::ceil(mantissa * 10.0) / 10.0 << "BKMGTPE"[o];
    return o != 0 ? (os << "B") : os;
  }
};

std::vector<fs::path> get_paths(std::vector<std::string> const &paths_str) {
  std::vector<fs::path> paths;
  paths.reserve(paths_str.size());
  for (auto const &path_str : paths_str) {
    paths.emplace_back(path_str);
  }
  return paths;
}

bool is_printable(std::string const &s) {
  return std::all_of(s.begin(), s.end(), [](char ch) {
    return std::isprint(ch);
  });
}

argparse::ArgumentParser construct_argument_paraser() {
  argparse::ArgumentParser program("mt_du");
  program.add_argument("-H", "--human-readable")
      .help("display the sizes in a human-readable format")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("paths")
      .help("the list of paths for which to print their disk size")
      .nargs(argparse::nargs_pattern::at_least_one);

  return program;
}

int main(int argc, char const *const *argv) {
  auto program = construct_argument_paraser();

  try {
    program.parse_args(argc, argv);
  } catch (std::runtime_error const &err) {
    std::cerr << err.what() << '\n';
    std::cerr << program;
    std::exit(EXIT_FAILURE);
  }

  auto const hr = program.get<bool>("--human-readable");
  auto const paths_str = program.get<std::vector<std::string>>("paths");
  auto const paths = get_paths(paths_str);
  std::vector<std::atomic<std::uintmax_t>> sizes(paths.size());

  BS::thread_pool pool;
  for (std::size_t i = 0; i < paths.size(); ++i) {
    pool.push_task(
        get_rec_dir_size,
        std::cref(paths[i]),
        std::ref(sizes[i]),
        std::ref(pool));
  }

  pool.wait_for_tasks();

  for (std::size_t i = 0; i < paths.size(); ++i) {
    auto path = paths[i].string();
    if (is_printable(path)) {
      std::cout << path << ' ';
      if (hr) {
        std::cout << HumanReadable(sizes[i]) << '\n';
      } else {
        std::cout << sizes[i] << '\n';
      }
    } else {
      std::cerr << "ERROR: Path contains unprintable characters:\n";
      for (char ch : path) {
        if (!std::isalnum(ch) && ch != '_' && ch != '-' && ch != '/') {
          fprintf(stderr, "$'\\%03o'", ch);
        } else {
          fprintf(stderr, "%c", ch);
        }
      }
      std::cerr << '\n';
    }
  }

  return 0;
}
