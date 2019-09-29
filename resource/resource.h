#pragma once

#include <array>
#include <boost/iostreams/device/mapped_file.hpp>
#include <filesystem>
#include <string_view>

#include "dvc/file.h"
#include "dvc/log.h"
#include "dvc/string.h"

// FILE:
//     'RESFILE1'
//     DIRBODY
// DIRBODY:
//     uint64(num_entries)
//     ENTRY*
// ENTRY:
//     'DDDDDDDD'
//     uint64(next_entry)
//     uint64(name_entry)
//     DIRBODY
// or
//     'FFFFFFFF'
//     uint64(next_entry)
//     uint64(name_entry)
//     size_t(num_bytes)
//     size_t(data_entry)

using Marker = std::array<char, 8>;

namespace markers {
constexpr Marker magic = {'R', 'E', 'S', 'F', 'I', 'L', 'E', '1'};
constexpr Marker directory = {'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D'};
constexpr Marker file = {'F', 'F', 'F', 'F', 'F', 'F', 'F', 'F'};
}  // namespace markers

class ResourceWriter {
 public:
  ResourceWriter(const std::filesystem::path& directory, const std::filesystem::path& outfile)
      : writer(outfile, dvc::truncate) {
    DVC_ASSERT(is_directory(directory));
    writer.rwrite(markers::magic);
    write_directory(directory);
    for (const auto& [string, backpatch] : string_table) {
      writer.write_backpatch(backpatch, uint64_t(writer.tell()));
      writer.write(string);
      writer.rwrite('\0');
    }
    for (const auto& [path, backpatch] : data_table) {
      writer.write_backpatch(backpatch, uint64_t(writer.tell()));
      writer.append_file(path);
    }
  }

 private:
  dvc::file_writer writer;
  std::vector<std::pair<std::string, uint64_t>> string_table;
  std::vector<std::pair<std::filesystem::path, uint64_t>> data_table;

  void write_directory(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> paths;
    for (auto& p : std::filesystem::directory_iterator(directory)) paths.push_back(p.path());
    writer.rwrite(uint64_t(paths.size()));

    for (const std::filesystem::path& p : paths) {
      if (is_directory(p)) {
        writer.rwrite(markers::directory);
      } else if (is_regular_file(p)) {
        writer.rwrite(markers::file);
      } else {
        DVC_FATAL("neither directory nor file: ", p);
      }

      uint64_t next_entry = writer.prepare_backpatch<uint64_t>();
      string_table.push_back(
          std::pair(p.filename().string(), writer.prepare_backpatch<uint64_t>()));

      if (is_directory(p))
        write_directory(p);
      else if (is_regular_file(p)) {
        writer.rwrite<uint64_t>(file_size(p));
        data_table.push_back(std::pair(p, writer.prepare_backpatch<uint64_t>()));
      } else {
        DVC_FATAL("neither directory nor file: ", p);
      }
      writer.write_backpatch(next_entry, uint64_t(writer.tell()));
    }
  }
};

class ResourceReader {
 public:
  ResourceReader(const std::filesystem::path& resource_file) {
    if (!exists(resource_file)) DVC_FAIL("No such file: ", resource_file);
    file.open(resource_file.string());
    DVC_ASSERT(file.is_open());
    DVC_ASSERT(get<Marker>(0) == markers::magic);
  }

  ResourceReader(const ResourceReader&) = delete;
  ResourceReader& operator=(const ResourceReader&) = delete;

  void dump() { dump_dirbody(0, 8); }

  void dump_dirbody(uint64_t parent, uint64_t pos) {
    DVC_LOG("DIRBODY#", parent, ":", pos);
    uint64_t nentries = get<uint64_t>(pos);
    uint64_t entry = pos + 8;
    for (uint64_t i = 0; i < nentries; i++) {
      dump_entry(pos, i, entry);
      entry = get<uint64_t>(entry + 8);
    }
  }

  void dump_entry(uint64_t parent, uint64_t index, uint64_t pos) {
    DVC_LOG("  ENTRY#", parent, ":", index, "@", pos,
            " type=", get_kind(pos) == Entry::file ? "F" : "D", " next=", get<uint64_t>(pos + 8),
            " name=", get<uint64_t>(pos + 16), " ", get_cstr(get<uint64_t>(pos + 16)));
    if (get_kind(pos) == Entry::file) {
      DVC_LOG("  LENGTH ", get<uint64_t>(pos + 24));
      DVC_LOG("  DATA ", get<uint64_t>(pos + 32));
    } else if (get_kind(pos) == Entry::dir) {
      dump_dirbody(pos, pos + 24);
    } else {
      DVC_FATAL("get_kind");
    }
  }

  void unpack(const std::filesystem::path& dest) {
    create_directory(dest);
    unpack_dirbody(dest, 8);
  }

  void unpack_dirbody(const std::filesystem::path& dest, uint64_t pos) {
    create_directory(dest);
    uint64_t nentries = get<uint64_t>(pos);
    uint64_t entry = pos + 8;
    for (uint64_t i = 0; i < nentries; i++) {
      unpack_entry(dest, entry);
      entry = get<uint64_t>(entry + 8);
    }
  }

  void unpack_entry(const std::filesystem::path& dir, uint64_t entry) {
    std::filesystem::path dest = dir / get_cstr(get<uint64_t>(entry + 16));

    if (get_kind(entry) == Entry::file) {
      std::string_view filedata = get(get<uint64_t>(entry + 32), get<uint64_t>(entry + 24));
      dvc::save_file(dest, filedata);
    } else if (get_kind(entry) == Entry::dir) {
      unpack_dirbody(dest, entry + 24);
    } else {
      DVC_FATAL("get_kind");
    }
  }

  std::string_view get_file(const std::string& name) {
    std::vector<std::string> parts = dvc::split("/", name);
    DVC_ASSERT_GT(parts.size(), 0);
    uint64_t dirbody = 8;
    for (size_t i = 0; i < parts.size() - 1; i++) dirbody = find_dirbody(dirbody, parts[i]);
    uint64_t filebody = find_filebody(dirbody, parts.back());

    uint64_t filelen = get<uint64_t>(filebody);
    uint64_t filedata = get<uint64_t>(filebody + 8);
    return get(filedata, filelen);
  }

  ~ResourceReader() { file.close(); }

 private:
  struct Entry {
    enum Kind { file, dir };
    Kind kind;
    uint64_t next;
    uint64_t name;
  };

  Entry::Kind get_kind(uint64_t offset) {
    Marker marker = get<Marker>(offset);
    if (marker == markers::file)
      return Entry::file;
    else if (marker == markers::directory)
      return Entry::dir;
    else
      DVC_FATAL("Resource file corruption @ ", offset);
  }

  Entry parse_entry(uint64_t offset) {
    return {get_kind(offset), get<uint64_t>(offset + 8), get<uint64_t>(offset + 16)};
  }

  uint64_t find_entry(Entry::Kind kind, uint64_t parent, const std::string& name) {
    uint64_t num_entries = get<uint64_t>(parent);
    uint64_t next = parent + 8;
    for (uint64_t i = 0; i < num_entries; i++) {
      uint64_t offset = next;
      Entry entry = parse_entry(next);
      next = entry.next;
      if (entry.kind != kind) continue;
      if (name != get_cstr(entry.name)) continue;
      return offset + 24;
    }
    DVC_FATAL("No such entry: ", name);
  }

  uint64_t find_dirbody(uint64_t parent, const std::string& name) {
    return find_entry(Entry::Kind::dir, parent, name);
  }

  uint64_t find_filebody(uint64_t parent, const std::string& name) {
    return find_entry(Entry::Kind::file, parent, name);
  }

  template <typename T>
  const T& get(uint64_t offset) const {
    DVC_ASSERT_LE(offset + sizeof(T), file.size());
    const char* pc = file.data() + offset;
    const T* pt = (const T*)(pc);
    return *pt;
  }

  std::string_view get(uint64_t offset, size_t len) {
    DVC_ASSERT_LE(offset + len, file.size());
    const char* pc = file.data() + offset;
    return {pc, len};
  }

  const char* get_cstr(uint64_t offset) { return file.data() + offset; }

  boost::iostreams::mapped_file_source file;
};
