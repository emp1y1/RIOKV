// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "db/builder.h"

#include "db/dbformat.h"
#include "db/filename.h"
#include "db/table_cache.h"
#include "db/version_edit.h"
#include <iostream>

#include "leveldb/db.h"
#include "leveldb/env.h"
#include "leveldb/iterator.h"

#include "db_impl.h"
using namespace  std;
namespace leveldb {

Status BuildTable(const std::string& dbname, Env* env, const Options& options,
                  TableCache* table_cache, Iterator* iter, FileMetaData* meta) {
  Status s;
  meta->file_size = 0;
  iter->SeekToFirst();

  std::string fname = TableFileName(dbname, meta->number);
  if (iter->Valid()) {
    WritableFile* file;
    s = env->NewWritableFile(fname, &file);
    if (!s.ok()) {
      return s;
    }

    TableBuilder* builder = new TableBuilder(options, file);
    meta->smallest.DecodeFrom(iter->key());
    Slice key;
/* for (; iter->Valid(); iter->Next()) {
      key = iter->key();
      builder->Add(key, iter->value());
   }*/
      key = iter->key();
      builder->Add(key, "");
      iter->SeekToLast();
      key = iter->key();
    if (!key.empty()) {
      meta->largest.DecodeFrom(key);
    }

    // Finish and check for builder errors
    s = builder->Finish();
    if (s.ok()) {
      meta->file_size = builder->FileSize();
      assert(meta->file_size > 0);
    }
    delete builder;

    // Finish and check for file errors
    if (s.ok()) {
      s = file->Sync();
    }
    if (s.ok()) {
      s = file->Close();
    }
    delete file;
    file = nullptr;

    if (s.ok()) {
      // Verify that the table is usable
      Iterator* it = table_cache->NewIterator(ReadOptions(), meta->number,
                                              meta->file_size);
      s = it->status();
      delete it;
    }
  }

  // Check for input iterator errors
  if (!iter->status().ok()) {
    s = iter->status();
  }

  if (s.ok() && meta->file_size > 0) {
    // Keep it
  } else {
    env->RemoveFile(fname);
  }
  return s;
}
Status BuildTableForTrivialMove(const std::string& dbname, Env* env, const Options& options,
                  TableCache* table_cache, Iterator* iter, FileMetaData* meta) {
  Status s;
  meta->file_size = 0;
  iter->SeekToFirst();

  std::string fname = TableFileName(dbname, meta->number);
  if (iter->Valid()) {
    WritableFile* file;
    s = env->NewWritableFile(fname, &file);
    if (!s.ok()) {
      return s;
    }

    TableBuilder* builder = new TableBuilder(options, file);
    meta->smallest.DecodeFrom(iter->key());
    Slice key;
     for (; iter->Valid(); iter->Next()) {
          key = iter->key();
          // 查找大于 key 的第一个元素,写入L1时删除对应map
          std::string keyStr = key.ToString();
          auto it = range_track_map.upper_bound(keyStr);
          // 检查找到的元素是否存在并且满足条件
          while (it != range_track_map.end()) {
            if (keyStr < it->first) {
              // keyStr 小于当前元素的 maxkey，检查 minkey
              if (keyStr >= it->second.minkey) {
                if (std::find(it->second.minkey.begin(), it->second.minkey.end(), 1) == it->second.minkey.end()) {
                  // 向量中不包含1，插入1
                  it->second.minkey.push_back(1);
                }
              } else {
                // 一旦 keyStr 小于当前元素的 maxkey，并且不满足 minkey 条件，停止遍历
                break;
              }
            }
            it++;
          }
          builder->Add(key, iter->value());
       }
/*    key = iter->key();
    builder->Add(key, "");
    iter->SeekToLast();
    key = iter->key();*/
    if (!key.empty()) {
      meta->largest.DecodeFrom(key);
    }

    // Finish and check for builder errors
    s = builder->Finish();
    if (s.ok()) {
      meta->file_size = builder->FileSize();
      assert(meta->file_size > 0);
    }
    delete builder;

    // Finish and check for file errors
    if (s.ok()) {
      s = file->Sync();
    }
    if (s.ok()) {
      s = file->Close();
    }
    delete file;
    file = nullptr;

    if (s.ok()) {
      // Verify that the table is usable
      Iterator* it = table_cache->NewIterator(ReadOptions(), meta->number,
                                              meta->file_size);
      s = it->status();
      delete it;
    }
  }

  // Check for input iterator errors
  if (!iter->status().ok()) {
    s = iter->status();
  }

  if (s.ok() && meta->file_size > 0) {
    // Keep it
  } else {
    env->RemoveFile(fname);
  }
  return s;
}
}  // namespace leveldb
