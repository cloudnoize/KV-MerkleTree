#include <unordered_map>

#include "nodes.hpp"

namespace merkle {
class Tree {
   public:
    Tree() {
        BranchNode::setNullNodeHash();
        root_ = BranchNode::createBranchNode();
    }

    void insert(ByteSequence&& key, ByteSequence&& value);

   private:
    std::unique_ptr<BranchNode> root_;
    std::unordered_map<ByteSequence, std::unique_ptr<BranchNode> > db_;
};
};  // namespace merkle