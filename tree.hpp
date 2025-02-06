#include <map>

#include "nodes.hpp"

namespace merkle {
class Tree {
   public:
    Tree() {
        BranchNode::setNullNodeHash();
        root_ = BranchNode::createBranchNode();
    }

    void insert(ByteSequence&& key, ByteSequence&& value);

    template <typename SPAN>
    const std::unique_ptr<BranchNode>& getBranchNode(const SPAN& span) const {
        static const std::unique_ptr<BranchNode> kNotFound;
        auto itr = db_.find(span);
        if (itr == db_.end()) {
            return kNotFound;
        }
        return itr->second;
    }

    const std::unique_ptr<BranchNode>& getRootNode() const { return root_; }

   private:
    template <typename SPAN>
    std::unique_ptr<BranchNode>& getMutableBranchNode(const SPAN& span) {
        auto itr = db_.find(span);
        assert(itr != db_.end());
        return itr->second;
    }

    std::unique_ptr<BranchNode> root_;
    std::map<ByteSequence, std::unique_ptr<BranchNode>, CompareBytes> db_;
};
};  // namespace merkle