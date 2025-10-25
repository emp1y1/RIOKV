// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_MEMTABLE_H_
#define STORAGE_LEVELDB_DB_MEMTABLE_H_

#include "db/dbformat.h"
#include "db/skiplist.h"
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include "leveldb/db.h"

#include "util/arena.h"
using namespace std;
namespace leveldb {

class InternalKeyComparator;
class MemTableIterator;

class MemTable {
 public:
  // MemTables are reference counted.  The initial reference count
  // is zero and the caller must call Ref() at least once.
  explicit MemTable(const InternalKeyComparator& comparator);

  MemTable(const MemTable&) = delete;
  MemTable& operator=(const MemTable&) = delete;

  // Increase reference count.
  void Ref() { ++refs_; }
  static std::unordered_map<uint64_t, MemTable*> saved_imm_tables_;
  static void SerializeSavedImmTables(const std::string& filename) {
    // std::cout << "保存map" << std::endl;
    std::ofstream file(filename);
    if (!file.is_open()) {
      // Handle error
      return;
    }
    for (const auto& pair : MemTable::saved_imm_tables_) {
      // Output to console
      // std::cout << "Key: " << pair.first << ", Address: " << std::hex <<
      // reinterpret_cast<uint64_t>(pair.second) << std::dec << std::endl;
      // Save to file in hexadecimal format
      file << pair.first << " " << std::hex
           << reinterpret_cast<uint64_t>(pair.second) << std::dec << std::endl;
    }
    file.close();
  }

  static void DeserializeSavedImmTables(const std::string& filename) {
    MemTable::saved_imm_tables_.clear();
    // std::cout << "还原map" << std::endl;
    std::ifstream file(filename);
    if (!file.is_open()) {
      // Handle error or first-time run without a map saved yet
      return;
    }
    uint64_t key;
    uint64_t address;
    while (file >> key) {
      file >> std::hex >> address >> std::dec;  // Read in hexadecimal format
      MemTable* memtable = reinterpret_cast<MemTable*>(address);
      MemTable::saved_imm_tables_[key] = memtable;
      // Output to console
      // std::cout << "Key: " << key << ", Address: " << std::hex << address <<
      // std::dec << std::endl;
    }
    file.close();
  }

  // Drop reference count.  Delete if no more references exist.
  // 用于保存memtable结构
  void FlushUnref(uint64_t* output_file_number, int* sstlevel) {
    --refs_;
    assert(refs_ >= 0);
    if (*sstlevel == 0) {
      // cout<<output_file_number<<"调用FlushUnref 当前的refs: "<<refs_<<endl;
      //cout<<"save第"<<*sstlevel<<"层的"<<*output_file_number<<endl;
      saved_imm_tables_[*output_file_number] = this;
      // std::cout<< "保存imm" <<*output_file_number<<"指针地址"<<
      // this<<std::endl;
/*      if (refs_ <= 0) {
        // delete this;
      }*/
    }else{
      //todo
      if (refs_ <= 0) {
       // cout<<"delete 第"<<*sstlevel<<"层的"<<*output_file_number<<endl;
        // std::cout<<"mem refs_ <= 0"<<std::endl;
        delete this;
      }
    }
  }
  void NormalUnref() {
    --refs_;
    // assert(refs_ >= 0);
    if (refs_ <= 0) {
      // std::cout<<"Normalrefs_ <= 0"<<std::endl;
    }
  }
  void MemUnref() {
    --refs_;
    assert(refs_ >= 0);
    if (refs_ <= 0) {
      // std::cout<<"mem refs_ <= 0"<<std::endl;
      delete this;
    }
  }
  // L0中对应SST被删除，删除对应的memtable
  void ImmuMemUnref() {
    // assert(refs_ >= 0);
    if (refs_ <= 0) {
      // std::cout<<"immumem refs_ <= 0"<<"指针地址"<< this<<std::endl;
      delete this;
    }
  }
  void ImmuUseUnref() {
    assert(refs_ >= 0);
    if (refs_ <= 0) {
      // std::cout<<"immumem Use Over"<<std::endl;
    }
  }
  // Returns an estimate of the number of bytes of data in use by this
  // data structure. It is safe to call when MemTable is being modified.
  size_t ApproximateMemoryUsage();

  // Return an iterator that yields the contents of the memtable.
  //
  // The caller must ensure that the underlying MemTable remains live
  // while the returned iterator is live.  The keys returned by this
  // iterator are internal keys encoded by AppendInternalKey in the
  // db/format.{h,cc} module.
  Iterator* NewIterator();

  // Add an entry into memtable that maps key to value at the
  // specified sequence number and with the specified type.
  // Typically value will be empty if type==kTypeDeletion.
  void Add(SequenceNumber seq, ValueType type, const Slice& key,
           const Slice& value);

  // If memtable contains a value for key, store it in *value and return true.
  // If memtable contains a deletion for key, store a NotFound() error
  // in *status and return true.
  // Else, return false.
  bool Get(const LookupKey& key, std::string* value, Status* s);

 private:
  friend class MemTableIterator;
  friend class MemTableBackwardIterator;

  struct KeyComparator {
    const InternalKeyComparator comparator;
    explicit KeyComparator(const InternalKeyComparator& c) : comparator(c) {}
    int operator()(const char* a, const char* b) const;
  };

  typedef SkipList<const char*, KeyComparator> Table;

  ~MemTable();  // Private since only Unref() should be used to delete it

  KeyComparator comparator_;
  int refs_;
  Arena arena_;
  Table table_;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_MEMTABLE_H_
