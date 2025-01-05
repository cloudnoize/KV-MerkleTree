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

    const std::unique_ptr<BranchNode>& getBranchNode(const ByteSequence& bs) const {
        static const std::unique_ptr<BranchNode> kNotFound;
        if (db_.count(bs) == 0) {
            return kNotFound;
        }
        return db_.at(bs);
    }

    const std::unique_ptr<BranchNode>& getRootNode() const { return root_; }

   private:
    std::unique_ptr<BranchNode> root_;
    std::unordered_map<ByteSequence, std::unique_ptr<BranchNode> > db_;
};
};  // namespace merkle