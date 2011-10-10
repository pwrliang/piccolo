#ifndef SPARSE_MAP_H_
#define SPARSE_MAP_H_

#include "util/common.h"
#include "worker/worker.pb.h"
#include "kernel/table.h"
#include "kernel/local-table.h"
#include <boost/noncopyable.hpp>

namespace dsm {

static const double kLoadFactor = 0.8;

template <class K, class V>
class SparseTable :
  public LocalTable,
  public TypedTable<K, V>,
  private boost::noncopyable {
private:
#pragma pack(push, 1)
  struct Bucket {
    K k;
    V v;
    bool in_use;
  };
#pragma pack(pop)

public:
  typedef DecodeIterator<K, V> UpdateDecoder;

  struct Iterator : public TypedTableIterator<K, V> {
    Iterator(SparseTable<K, V>& parent) : pos(-1), parent_(parent) {
      Next();
    }

    Marshal<K>* kmarshal() { return parent_.kmarshal(); }
    Marshal<V>* vmarshal() { return parent_.vmarshal(); }

    void Next() {
      do {
        ++pos;
      } while (pos < parent_.size_ && !parent_.buckets_[pos].in_use);
    }

    bool done() {
      return pos == parent_.size_;
    }

    const K& key() { return parent_.buckets_[pos].k; }
    V& value() { return parent_.buckets_[pos].v; }

    int pos;
    SparseTable<K, V> &parent_;
  };

  struct Factory : public TableFactory {
    TableBase* New() { return new SparseTable<K, V>(); }
  };

  // Construct a SparseTable with the given initial size; it will be expanded as necessary.
  SparseTable(int size=1);
  ~SparseTable() {}

  void Init(const TableDescriptor * td) {
    TableBase::Init(td);
  }

  V get(const K& k);
  bool contains(const K& k);
  void put(const K& k, const V& v);
  void update(const K& k, const V& v);
  void remove(const K& k) {
    LOG(FATAL) << "Not implemented.";
  }

  void resize(int64_t size);

  bool empty() { return size() == 0; }
  int64_t size() { return entries_; }

  void clear() {
    for (int i = 0; i < size_; ++i) { buckets_[i].in_use = 0; }
    entries_ = 0;
  }

  TableIterator *get_iterator() {
      return new Iterator(*this);
  }

  void Serialize(TableCoder *out);
  void DecodeUpdates(TableCoder *in, DecodeIteratorBase *itbase);

  Marshal<K>* kmarshal() { return ((Marshal<K>*)info_.key_marshal); }
  Marshal<V>* vmarshal() { return ((Marshal<V>*)info_.value_marshal); }

private:
  uint32_t bucket_idx(K k) {
    return hashobj_(k) % size_;
  }

  int bucket_for_key(const K& k) {
    int start = bucket_idx(k);
    int b = start;

    do {
      if (buckets_[b].in_use) {
        if (buckets_[b].k == k) {
          return b;
        }
      } else {
        return -1;
      }

       b = (b + 1) % size_;
    } while (b != start);

    return -1;
  }

  std::vector<Bucket> buckets_;

  int64_t entries_;
  int64_t size_;

  std::tr1::hash<K> hashobj_;
};

template <class K, class V>
SparseTable<K, V>::SparseTable(int size)
  : buckets_(0), entries_(0), size_(0) {
  clear();

  resize(size);
}

template <class K, class V>
void SparseTable<K, V>::Serialize(TableCoder *out) {
  Iterator *i = (Iterator*)get_iterator();
  string k, v;
  while (!i->done()) {
    k.clear(); v.clear();
    ((Marshal<K>*)info_.key_marshal)->marshal(i->key(), &k);
    ((Marshal<V>*)info_.value_marshal)->marshal(i->value(), &v);
    out->WriteEntry(k, v);
    i->Next();
  }
  delete i;
}

template <class K, class V>
void SparseTable<K, V>::DecodeUpdates(TableCoder *in, DecodeIteratorBase *itbase) {
  UpdateDecoder* it = static_cast<UpdateDecoder*>(itbase);
  K k;
  V v;
  string kt, vt;

  it->clear();
  while (in->ReadEntry(&kt, &vt)) {
    ((Marshal<K>*)info_.key_marshal)->unmarshal(kt, &k);
    ((Marshal<V>*)info_.value_marshal)->unmarshal(vt, &v);
    it->append(k, v);
  }
  it->rewind();
  return;
}

template <class K, class V>
void SparseTable<K, V>::resize(int64_t size) {
  CHECK_GT(size, 0);
  if (size_ == size)
    return;

  std::vector<Bucket> old_b = buckets_;
  int old_entries = entries_;

//  LOG(INFO) << "Rehashing... " << entries_ << " : " << size_ << " -> " << size;

  buckets_.resize(size);
  size_ = size;
  clear();

  for (size_t i = 0; i < old_b.size(); ++i) {
    if (old_b[i].in_use) {
      put(old_b[i].k, old_b[i].v);
    }
  }

  CHECK_EQ(old_entries, entries_);
}

template <class K, class V>
bool SparseTable<K, V>::contains(const K& k) {
  return bucket_for_key(k) != -1;
}

template <class K, class V>
V SparseTable<K, V>::get(const K& k) {
  int b = bucket_for_key(k);
  //The following key display is a hack hack hack and only yields valid
  //results for ints.  It will display nonsense for other types.
  CHECK_NE(b, -1) << "No entry for requested key <" << *((int*)&k) << ">";

  return buckets_[b].v;
}

template <class K, class V>
void SparseTable<K, V>::update(const K& k, const V& v) {
  int b = bucket_for_key(k);

  if (b != -1) {
    if (info_.accum->accumtype == ACCUMULATOR) {
      ((Accumulator<V>*)info_.accum)->Accumulate(&buckets_[b].v, v);
    } else if (info_.accum->accumtype == TRIGGER) {
      V v2 = buckets_[b].v;
      bool doUpdate = false;
//      LOG(INFO) << "Executing Trigger [sparse]" << endl;
      ((Trigger<K,V>*)info_.accum)->Fire(&k,&v2,v,&doUpdate,false);	//isNew=false
      if (doUpdate)
        buckets_[b].v = v2;
    } else {
      LOG(FATAL) << "update() called with neither TRIGGER nor ACCUMULATOR";
    }
  } else {
    if (info_.accum->accumtype == TRIGGER) {
      bool doUpdate = true;
      V v2 = v;
      ((Trigger<K,V>*)info_.accum)->Fire(&k,&v2,v,&doUpdate,true); //isNew=true
      if (doUpdate)
        put(k, v2);
    } else {
      put(k, v);
    }
  }
}

template <class K, class V>
void SparseTable<K, V>::put(const K& k, const V& v) {
  int start = bucket_idx(k);
  int b = start;
  bool found = false;

  do {
    if (!buckets_[b].in_use) {
      break;
    }

    if (buckets_[b].k == k) {
      found = true;
      break;
    }

    b = (b + 1) % size_;
  } while(b != start);

  // Inserting a new entry:
  if (!found) {
    if (entries_ > size_ * kLoadFactor) {
      resize((int)(1 + size_ * 2));
      put(k, v);
    } else {
      buckets_[b].in_use = 1;
      buckets_[b].k = k;
      buckets_[b].v = v;
      ++entries_;
    }
  } else {
    // Replacing an existing entry
    buckets_[b].v = v;
  }
}
}
#endif /* SPARSE_MAP_H_ */
