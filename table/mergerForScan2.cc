// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
#include "db/db_impl.h"
#include "db/dbformat.h"
#include "db/version_set.h"
#include <iostream>
#include <list>
#include <vector>

#include "leveldb/comparator.h"
#include "leveldb/iterator.h"

#include "table/iterator_wrapper.h"
#include "table/mergerForScan.h"

using namespace std;

namespace leveldb {
std::map<std::string, CustomData> range_track_map;
std::string minkey_;
std::string Map_maxkey;
namespace {
class MergingIteratorForScan : public Iterator {
 public:
  int next_num = 0;
  mutable bool isAddLevel = false;
  MergingIteratorForScan(const Comparator* comparator, Iterator** children,
                         int n, std::map<int, int> plMap, bool useMap,
                         ReadOptions op, VersionSet* versions)
      : comparator_(comparator),
        children(*children),
        n_(n),
        current_(nullptr),
        direction_(kForward),
        smallest_counts_(n, 0),
        participateLevelMap(plMap),
        isUseMap(useMap),
        options(op),
        versions_(versions) {
    /*    for (int i = 0; i < n; i++) {
          children_[i].Set(children[i]);
        }*/
    //cout << "初始化MergingIteratorForScan" << endl;
  }
  // 添加新的迭代器
  void AddIterator(Iterator* child) const {
    children_.emplace_back(std::move(child));
    smallest_counts_.push_back(0);
  }

  bool Valid() const override { return (current_ != nullptr); }

  ~MergingIteratorForScan() override {
 /*   if (Valid()) {
      // map中不存在记录，添加新记录
      if (!isUseMap) {
        // string maxkey=leveldb::ExtractUserKey(current_->key()).ToString();
        CustomData test = {minkey_, {}, 0};
        cout << "maxkey: " << current_->key().ToString() << endl;
        range_track_map[current_->key().ToString()] = test;
        // 获取level并保存
        for (const auto& pair : participateLevelMap) {
          std::cout << "iter_num: " << pair.first << ", level: " << pair.second
                    << std::endl;
          if (smallest_counts_[pair.first] > 0) {
            cout << "neededlevel " << pair.second << " chosen as smallest "
                 << smallest_counts_[pair.first] << " times." << endl;
            range_track_map[current_->key().ToString()].neededlevel.push_back(
                pair.second);
          } else {
            cout << "Notneededlevel " << pair.second << " chosen as smallest "
                 << smallest_counts_[pair.first] << " times." << endl;
          }
        }
        // 使用map并且超出maxkey，更新maxkey
      } else if (isAddLevel) {
        // range_track_map[current_->key().ToString()].neededlevel.clear();//清空原先的内容
         bool isUseExLevel=false;
         std::vector<int> UseExLevel;
        for (const auto& pair : participateLevelMap) {
          if (smallest_counts_[pair.first] > 0 &&
              std::find(range_track_map[Map_maxkey].neededlevel.begin(),
                        range_track_map[Map_maxkey].neededlevel.end(),
                        pair.second) ==
                  range_track_map[Map_maxkey].neededlevel.end()) {
            cout << "level：" << pair.second
                 << "在Addlevel后被使用到了，需要添加新level" << endl;
            UseExLevel.push_back(pair.second);
            // 在 neededlevel
            // 中不存在当前level,且这level被使用了，需要保存一条新的记录
             isUseExLevel= true;
            // 执行相应的操作
          }
        }
        auto it = range_track_map.lower_bound(Map_maxkey);
        // 修改当前记录的maxkey
         cout<<"it->first: "<<it->first<<endl;
         if(!isUseExLevel){
        if (++it != range_track_map.end()) {
          // 1.没有重叠
          if (it->second.minkey >= current_->key().ToString()) {
            cout<<"没有重叠 "<<endl;
            range_track_map[current_->key().ToString()] =
                range_track_map[Map_maxkey];
            range_track_map.erase(Map_maxkey);
          }
          // 2.current_->key().ToString()大于后一个范围的minkey了，但小于后一个范围的maxkey
          if (it->second.minkey < current_->key().ToString() &&
              current_->key().ToString() < it->first) {
            cout<<"current_->key().ToString()大于后一个范围的minkey了，但小于后一个范围的maxkey "<<endl;
            // 检查两个范围是否能够合并
            auto prev_it = std::prev(it);
            std::sort(it->second.neededlevel.begin(),
                      it->second.neededlevel.end());
            std::sort(prev_it->second.neededlevel.begin(),
                      prev_it->second.neededlevel.end());
            if (it->second.neededlevel == prev_it->second.neededlevel) {
              cout<<"能够合并 "<<endl;
              it->second.minkey = prev_it->second.minkey;
              it->second.execute_times += prev_it->second.execute_times;
              range_track_map.erase(Map_maxkey);
            } else {
              cout<<"更新两个range,current_->key().ToString()为临界值 "<<endl;
              // 更新两个range,current_->key().ToString()为临界值
              // 1.
              range_track_map[current_->key().ToString()] =
                  range_track_map[Map_maxkey];
              range_track_map.erase(Map_maxkey);
              // 2.
              it->second.minkey = current_->key().ToString();
            }
          }
          // 3.超出范围
          while (current_->key().ToString() > it->first &&
                 it != range_track_map.end()) {
            cout<<"超出范围 "<<endl;
            range_track_map[current_->key().ToString()] =
                range_track_map[Map_maxkey];
            range_track_map.erase(Map_maxkey);
            string eraseMap = it->first;
            it++;
            range_track_map.erase(eraseMap);
          }
        } else {
          // 没有元素了，没有范围了
          range_track_map[current_->key().ToString()] =
              range_track_map[Map_maxkey];
          range_track_map.erase(Map_maxkey);
        }
         }else{
        if (++it != range_track_map.end()) {
          // 1.没有重叠

            // 有使用到新的level,添加与之前相连的范围
            range_track_map[current_->key().ToString()] =
                range_track_map[Map_maxkey];
            range_track_map[current_->key().ToString()].minkey = Map_maxkey;
            range_track_map[current_->key().ToString()].execute_times = 0;
            range_track_map[current_->key().ToString()].neededlevel.insert(
                range_track_map[current_->key().ToString()].neededlevel.end(),
                UseExLevel.begin(), UseExLevel.end());

        }
         }
      }
    }
*/
    // Output the counts before deleting the children

/*    for (int i = 0; i < n_; i++) {
      std::cout << "Iterator " << i << " was chosen as smallest "
                << smallest_counts_[i] << " times." << std::endl;
    }
    std::cout << "~MergingIteratorForScan" << endl;*/
    // delete[] children_;
  }

  void SeekToFirst() override {
    for (int i = 0; i < n_; i++) {
      children_[i].SeekToFirst();
    }
    FindSmallest();
    direction_ = kForward;
  }

  void SeekToLast() override {
    for (int i = 0; i < n_; i++) {
      children_[i].SeekToLast();
    }
    FindLargest();
    direction_ = kReverse;
  }

  void Seek(const Slice& target) override {
    minkey_ = target.ToString();
   // cout << "seek的minkey ：" << minkey_ << endl;
    std::vector<Iterator*> levellist;
    auto it = range_track_map.upper_bound(target.ToString());
    if (it != range_track_map.end() && it->second.minkey <= minkey_) {
      // 找到了键
    //  cout << "找到对应的minxKey" << endl;
    //  std::cout << "map中存储的level: ";
      if (it->second.minkey == minkey_)
    //    cout << "seek的minkey与map中的minkey相等 ：" << minkey_ << endl;
      if (it->second.minkey < minkey_)
/*      cout << "seek的minkey大于map中的minkey ："
             << "seek的minkey" << minkey_
             << " map中的minkey: " << it->second.minkey
             << "map中的maxkey: " << it->first << endl;*/
      Map_maxkey = it->first;
      it->second.execute_times++;

      for (int level : it->second.neededlevel) {
        std::cout << level << " ";
      }
      usedCustomData = it->second;
      isUseMap = true;
      versions_->current()->AddIteratorsbylevel(
          options, &levellist, &participateLevelMap,
          &usedCustomData.neededlevel, &n_);
    } else {
      cout << "没找到对应的minxKey" << endl;
      versions_->current()->AddIteratorsForScan(
          options, &levellist, &participateLevelMap, &isUseMap, &n_);
    }
    cout << "n_ + list.size() " << n_ + levellist.size() << endl;
    children_.resize(20);
    smallest_counts_.resize(n_ + levellist.size());
    // 填充原始 children
    for (int i = 0; i < n_; i++) {
      children_[i].Set(children);
    }
    // 添加新的 children
    for (size_t i = 0; i < levellist.size(); i++) {
      if (levellist[i] != nullptr) {
        children_[n_ + i].Set(levellist[i]);
      } else {
        cout << "传递了空的 Iterator* 到 list 中" << endl;
      }
    }
    n_ += levellist.size();
    cout << "n_ : " << n_ << endl;
    for (int i = 0; i < n_; i++) {
      // cout << "开始seek" << endl;
      children_[i].Seek(target);
    }
    FindSmallest();
    direction_ = kForward;
  }

  void Next() override {
    assert(Valid());
    // cout<<current_->key().ToString()<<endl;
    cout << ++next_num << endl;
    // Ensure that all children are positioned after key().
    // If we are moving in the forward direction, it is already
    // true for all of the non-current_ children since current_ is
    // the smallest child and key() == current_->key().  Otherwise,
    // we explicitly position the non-current_ children.
    if (direction_ != kForward) {
      for (int i = 0; i < n_; i++) {
        IteratorWrapper* child = &children_[i];
        if (child != current_) {
          child->Seek(key());
          if (child->Valid() &&
              comparator_->Compare(key(), child->key()) == 0) {
            child->Next();
          }
        }
      }
      direction_ = kForward;
    }

    current_->Next();
    FindSmallest();
    if (isUseMap && !isAddLevel) {
      if (current_->key().ToString() > Map_maxkey) {
        // 使用了map，并且因为超出范围而添加迭代器
        cout << "超出范围而添加迭代器" << endl;
        string out_key = current_->key().ToString();
        isAddLevel = true;
        cout << "smallest->key().ToString():" << out_key << endl;
        cout << "超出Map_maxkey:" << Map_maxkey << endl;
        // cout << "current_->key().ToString():" <<
        // current_->key().ToString()<<endl;
        std::vector<int> unUsedlevels;
        for (int level = 1; level < config::kNumLevels; level++) {
          if (std::find(usedCustomData.neededlevel.begin(),
                        usedCustomData.neededlevel.end(),
                        level) == usedCustomData.neededlevel.end()) {
            // 如果没有找到，加入到 unUsedlevels
            cout << "unUsedlevels push_back: " << level << endl;
            unUsedlevels.push_back(level);
          }
        }
        std::vector<Iterator*> list2;
        versions_->current()->AddIteratorsbylevel(
            options, &list2, &participateLevelMap, &unUsedlevels, &n_);
        cout << "before children_ size: " << children_.size() << endl;
        cout << "list2.size() : " << list2.size() << endl;
        // children_.resize(n_+ list2.size());
        smallest_counts_.resize(n_ + list2.size());

        // 添加新的 children
        for (size_t i = 0; i < list2.size(); i++) {
          if (list2[i] != nullptr) {
            cout << "n_ + i : " << n_ + i << endl;
            children_[n_ + i].Set(list2[i]);
          } else {
            cout << "传递了空的 Iterator* 到 list 中" << endl;
          }
        }
        cout << "after children_ size: " << children_.size() << endl;
        cout << "n_ : " << n_ << endl;
        for (int i = n_; i < n_ + list2.size(); i++) {
          cout << "i:" << i << endl;
          children_[i].Seek(out_key);
        }
        cout << "smallest->key().ToString():" << out_key << endl;
        n_ += list2.size();
        cout << "n_ : " << n_ << endl;
        FindSmallest();
        direction_ = kForward;

        cout << current_->key().ToString() << endl;
      }
    }
  }

  void Prev() override {
    assert(Valid());

    // Ensure that all children are positioned before key().
    // If we are moving in the reverse direction, it is already
    // true for all of the non-current_ children since current_ is
    // the largest child and key() == current_->key().  Otherwise,
    // we explicitly position the non-current_ children.
    if (direction_ != kReverse) {
      for (int i = 0; i < n_; i++) {
        IteratorWrapper* child = &children_[i];
        if (child != current_) {
          child->Seek(key());
          if (child->Valid()) {
            // Child is at first entry >= key().  Step back one to be < key()
            child->Prev();
          } else {
            // Child has no entries >= key().  Position at last entry.
            child->SeekToLast();
          }
        }
      }
      direction_ = kReverse;
    }

    current_->Prev();
    FindLargest();
  }

  Slice key() const override {
    assert(Valid());
    return current_->key();
  }

  Slice value() const override {
    assert(Valid());
    return current_->value();
  }

  Status status() const override {
    Status status;
    for (int i = 0; i < n_; i++) {
      status = children_[i].status();
      if (!status.ok()) {
        break;
      }
    }
    return status;
  }

 private:
  // Which direction is the iterator moving?
  enum Direction { kForward, kReverse };
  mutable std::map<int, int> participateLevelMap;
  bool isUseMap;
  void FindSmallest();
  void FindLargest();
  ReadOptions options;
  VersionSet* versions_;
  mutable std::vector<int> smallest_counts_;
  // We might want to use a heap in case there are lots of children.
  // For now we use a simple array since we expect a very small number
  // of children in leveldb.
  const Comparator* comparator_;
  mutable Iterator* children;
  mutable std::vector<IteratorWrapper> children_;
  // IteratorWrapper* children_;
  mutable int n_;
  IteratorWrapper* current_;
  Direction direction_;
  CustomData usedCustomData;
};

void MergingIteratorForScan::FindSmallest() {
  IteratorWrapper* smallest = nullptr;
  int smallest_index = -1;
  for (int i = 0; i < n_; i++) {
    // cout<<"n_ in FindSmallest:"<<n_<<endl;
    IteratorWrapper* child = &children_[i];
    if (child->Valid()) {
      if (smallest == nullptr) {
        smallest = child;
        smallest_index = i;
      } else if (comparator_->Compare(child->key(), smallest->key()) < 0) {
        smallest = child;
        smallest_index = i;
      }
      // cout<<"Smallest in iter:"<<i<< "  key:"<<child->key().ToString()<<endl;
    }
  }
  if (smallest_index != -1) {
    smallest_counts_[smallest_index]++;  // Increment the count for this
                                         // iterator
    cout << "smallest in iter :" << smallest_index << endl;
  }
  current_ = smallest;
  // cout<<"smallest key :"<<current_->key().ToString()<<endl;
  // cout<<"FindSmallest结束"<<endl;
}

void MergingIteratorForScan::FindLargest() {
  IteratorWrapper* largest = nullptr;
  for (int i = n_ - 1; i >= 0; i--) {
    IteratorWrapper* child = &children_[i];
    if (child->Valid()) {
      if (largest == nullptr) {
        largest = child;
      } else if (comparator_->Compare(child->key(), largest->key()) > 0) {
        largest = child;
      }
    }
  }
  current_ = largest;
}
}  // namespace

Iterator* NewMergingIteratorForScan(const Comparator* comparator,
                                    Iterator** children, int n,
                                    std::map<int, int>* participateLevelMap,
                                    bool* isUseMap, const ReadOptions* options,
                                    VersionSet* versions_) {
  assert(n >= 0);
  cout << "n: " << n << endl;
  if (n == 0) {
    cout << "n==0" << endl;
    return NewEmptyIterator();
    /*  } else if (n == 1) {
        return children[0];*/
  } else {
    MergingIteratorForScan* miter = new MergingIteratorForScan(
        comparator, children, n, *participateLevelMap, *isUseMap, *options,
        versions_);
    return miter;
  }
}

}  // namespace leveldb
