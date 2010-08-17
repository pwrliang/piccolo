#include "client/client.h"
#include "examples/examples.pb.h"

using namespace dsm;

DECLARE_int32(shards);
DECLARE_int32(iterations);
DECLARE_int32(block_size);
DECLARE_int32(edge_size);

namespace dsm {
template <>
struct Marshal<Bucket> {
  static void marshal(const Bucket& t, string *out) { t.SerializePartialToString(out); }
  static void unmarshal(const StringPiece& s, Bucket* t) { t->ParseFromArray(s.data, s.len); }
};
}
