#include <map>

#include "nodes.hpp"

namespace merkle {
class Tree {
   public:
    using KVDB = std::map<ByteSequence, std::unique_ptr<BranchNode>, LessThan>;
    struct Proof {
        BranchNode root;
        KVDB db;
    };
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

    void calculateHash();
    size_t dbSize() const { return db_.size(); }
    const std::map<ByteSequence, std::unique_ptr<BranchNode>, LessThan>& getRoDB() const {
        return db_;
    }

    void printTree();

    // KVDB generateProof(const ByteSequenceView key) const;

    // counters
    size_t numDirtynodes_ = 0;

   private:
    template <typename SPAN>
    std::unique_ptr<BranchNode>& getMutableBranchNode(const SPAN& span) {
        auto itr = db_.find(span);
        assert(itr != db_.end());
        return itr->second;
    }

    std::unique_ptr<BranchNode> root_;
    KVDB db_;
};
};  // namespace merkle