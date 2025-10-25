// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/table_cache.h"

#include "db/filename.h"
#include "leveldb/env.h"
#include "leveldb/table.h"
#include "util/coding.h"

namespace leveldb {
int cache_hits = 0; // 缓存命中的次数
int cache_misses = 0; // 缓存未命中的次数
double  total_NewIterator_elapsed_ms =0.0;
struct TableAndFile {
  RandomAccessFile* file;
  Table* table;
};

static void DeleteEntry(const Slice& key, void* value) {
  TableAndFile* tf = reinterpret_cast<TableAndFile*>(value);
  delete tf->table;
  delete tf->file;
  delete tf;
}

static void UnrefEntry(void* arg1, void* arg2) {
  Cache* cache = reinterpret_cast<Cache*>(arg1);
  Cache::Handle* h = reinterpret_cast<Cache::Handle*>(arg2);
  cache->Release(h);
}

TableCache::TableCache(const std::string& dbname, const Options& options,
                       int entries)
    : env_(options.env),
      dbname_(dbname),
      options_(options),
      cache_(NewLRUCache(entries)) {}

TableCache::~TableCache() { delete cache_; }

Status TableCache::FindTable(uint64_t file_number, uint64_t file_size,
                             Cache::Handle** handle) {
  Status s;
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  Slice key(buf, sizeof(buf));
  *handle = cache_->Lookup(key);
  //std::cout<<"find table"<< std::endl;
  //std::this_thread::sleep_for(std::chrono::seconds(1));
  if (*handle == nullptr) {
    cache_misses++; // 增加缓存未命中计数
    std::string fname = TableFileName(dbname_, file_number);
    RandomAccessFile* file = nullptr;
    Table* table = nullptr;
    auto start2 = std::chrono::high_resolution_clock::now();  // 开始计时
    s = env_->NewRandomAccessFile(fname, &file);
    auto end2 = std::chrono::high_resolution_clock::now();  // 结束计时
    std::chrono::duration<double, std::milli> elapsed2 =
        end2 - start2;  // 计算时间差
    total_NewIterator_elapsed_ms += elapsed2.count();  // 累加时间

    if (!s.ok()) {
      std::string old_fname = SSTTableFileName(dbname_, file_number);
      if (env_->NewRandomAccessFile(old_fname, &file).ok()) {
        s = Status::OK();
      }
    }

    if (s.ok()) {
      s = Table::Open(options_, file, file_size, &table);
    }

    if (!s.ok()) {
      assert(table == nullptr);
      delete file;
      // We do not cache error results so that if the error is transient,
      // or somebody repairs the file, we recover automatically.
    } else {
      TableAndFile* tf = new TableAndFile;
      tf->file = file;
      tf->table = table;
      *handle = cache_->Insert(key, tf, 1, &DeleteEntry);
    }
  }else {
    // 缓存命中
    cache_hits++; // 增加缓存命中计数
  }

  return s;
}
Iterator* TableCache::NewIterator(const ReadOptions& options,
                                  uint64_t file_number, uint64_t file_size,
                                  Table** tableptr) {
  if (tableptr != nullptr) {
    *tableptr = nullptr;
  }

  Cache::Handle* handle = nullptr;
  Status s = FindTable(file_number, file_size, &handle);
  if (!s.ok()) {
    return NewErrorIterator(s);
  }

  Table* table = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
  Iterator* result = table->NewIterator(options);
  result->RegisterCleanup(&UnrefEntry, cache_, handle);
  if (tableptr != nullptr) {
    *tableptr = table;
  }
  return result;
}

Status TableCache::Get(const ReadOptions& options, uint64_t file_number,
                       uint64_t file_size, const Slice& k, void* arg,
                       void (*handle_result)(void*, const Slice&,
                                             const Slice&)) {
  Cache::Handle* handle = nullptr;
  Status s = FindTable(file_number, file_size, &handle);
  if (s.ok()) {
    Table* t = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
    s = t->InternalGet(options, k, arg, handle_result);
    cache_->Release(handle);
  }
  return s;
}

void TableCache::Evict(uint64_t file_number) {
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  cache_->Erase(Slice(buf, sizeof(buf)));
}

}  // namespace leveldb
