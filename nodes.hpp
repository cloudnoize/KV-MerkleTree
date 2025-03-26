#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>

#include "detail/crypto_utils.hpp"
#include "detail/key_utils.hpp"

#pragma once

namespace merkle {

class Node {
   public:
    static constexpr uint16_t kSizeField = 8;
    enum Type : uint8_t { HashOfBranch, HashOfLeaf, BranchNode, NullNode };
    // using NullNode = nullptr;
    const unsigned char* hash() const { return hash_; }

    unsigned char* getMutableHash() { return hash_; }

    ByteSequenceView extension() const { return ByteSequenceToView(extension_); }
    void setExtension(ByteSequence&& extension) { extension_ = std::move(extension); }
    void truncateExtension(size_t count) {
        auto oldExtension = extension();
        auto oldExtensionView = ExtensionView(oldExtension);
        oldExtensionView.incrementPositionBy(count);
        auto updatedExtensionView = oldExtensionView.getExtentionFromCurrentPosition();
        setExtension(ByteSequence{updatedExtensionView.begin(), updatedExtensionView.end()});
    }

    virtual Type getType() const = 0;
    virtual std::ostream& print(std::ostream& os) const = 0;

    virtual void serialize(ByteSequence& out) const;
    virtual void deserialize(const ByteSequenceView& in, size_t& pos);

    // TODO add ser/der

    virtual ~Node() noexcept = default;
    Node() = default;

    static std::string toHex(const unsigned char* data) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            oss << std::setw(2) << static_cast<int>(data[i]);
        }

        return oss.str();
    }

   private:
    unsigned char hash_[SHA256_DIGEST_LENGTH] = {};
    ByteSequence extension_;
};

class HashOfBranch : public Node {
   public:
    Node::Type getType() const override { return Node::HashOfBranch; }

    std::ostream& print(std::ostream& os) const override {
        os << "HashOfBranch: Hash: " << Node::toHex(hash()) << " extension " << extension()
           << " is dirty " << is_dirty_;
        return os;
    }

    bool isDirty() const { return is_dirty_; }

    void setDirty(bool dirty) { is_dirty_ = dirty; }

    void serialize(ByteSequence& out) const override;
    void deserialize(const ByteSequenceView& in, size_t& pos) override;

    ~HashOfBranch() override = default;
    HashOfBranch() = default;
    HashOfBranch(const HashOfBranch&) = delete;
    HashOfBranch& operator=(const HashOfBranch&) = delete;

   private:
    bool is_dirty_{false};
};

class HashOfLeaf : public Node {
   public:
    HashOfLeaf() = default;
    HashOfLeaf(const ByteSequence& key, const ByteSequence& value);
    HashOfLeaf(const ByteSequence& key, const ByteSequence& value, ByteSequence&& extension)
        : HashOfLeaf(key, value) {
        setExtension(std::move(extension));
    }

    void updateHash(const ByteSequence& key, const ByteSequence& value);

    Node::Type getType() const override { return Node::HashOfLeaf; }
    ~HashOfLeaf() override = default;

    void serialize(ByteSequence& out) const override;
    void deserialize(const ByteSequenceView& in, size_t& pos) override;

    static std::unique_ptr<Node> createhashOfLeaf(const ByteSequence& key,
                                                  const ByteSequence& value,
                                                  ByteSequence&& extension) {
        return std::make_unique<HashOfLeaf>(key, value, std::move(extension));
    }

    std::ostream& print(std::ostream& os) const override {
        os << "HashOfLeaf: Hash: " << Node::toHex(hash()) << " extension " << extension();
        return os;
    }
};

class BranchNode : public Node {
   public:
    using ChildPos = std::optional<Byte>;
    static constexpr ChildPos LeafChildPos = std::nullopt;
    using ChildAndPos = std::pair<std::reference_wrapper<std::unique_ptr<Node>>, ChildPos>;
    static constexpr uint16_t kBranchingFactor = std::numeric_limits<uint8_t>::max();
    static const ByteSequence kNullNodeToHash;
    static unsigned char kNullNodeHash[SHA256_DIGEST_LENGTH];
    using ChildrenArray = std::array<std::unique_ptr<Node>, kBranchingFactor>;
    Node::Type getType() const override { return Node::BranchNode; }
    void computeHash();

    void setLeaf(const ByteSequence& key, const ByteSequence& value) {
        leaf_ = std::make_unique<merkle::HashOfLeaf>(key, value);
    }

    Node::Type getTypeOfChild(ChildPos optChild) const {
        if (optChild == LeafChildPos) {
            return leaf_ == nullptr ? Node::NullNode : Node::HashOfLeaf;
        }
        auto child = *optChild;
        if (children_[child] == nullptr) {
            return Node::NullNode;
        }
        return children_[child]->getType();
    }

    const std::unique_ptr<Node>& getChildAt(ChildPos optChild) const {
        if (optChild == LeafChildPos) {
            return leaf_;
        }
        return children_[*optChild];
    }

    void setDirty(ChildPos optChild, bool dirty) {
        auto type = getTypeOfChild(optChild);
        assert(type == Node::Type::HashOfBranch);
        static_cast<merkle::HashOfBranch*>(children_[*optChild].get())->setDirty(dirty);
    }

    void updateHashOfBranchHash(ChildPos optChild, const unsigned char* hash) {
        auto type = getTypeOfChild(optChild);
        assert(type == Node::Type::HashOfBranch);
        auto* node = children_[*optChild].get();
        std::memcpy(node->getMutableHash(), hash, SHA256_DIGEST_LENGTH);
    }

    void swapNodeAtChild(std::optional<Byte> optChild, std::unique_ptr<Node>& other) {
        if (optChild == LeafChildPos) {
            swapLeaf(other);
            return;
        }
        children_[*optChild].swap(other);
    }
    void updateHashOfLeafChild(Byte child, const ByteSequence& key, const ByteSequence& value);

    BranchNode() = default;
    ~BranchNode() override = default;
    BranchNode(const BranchNode&) = delete;
    BranchNode& operator=(const BranchNode&) = delete;

    static std::unique_ptr<BranchNode> createBranchNode(ByteSequence&& extension,
                                                        std::vector<ChildAndPos>& cnps) {
        auto node = std::make_unique<BranchNode>();
        node->setExtension(std::move(extension));
        for (auto& cnp : cnps) {
            node->swapNodeAtChild(cnp.second, cnp.first.get());
        }
        return node;
    }
    static std::unique_ptr<BranchNode> createBranchNode() { return std::make_unique<BranchNode>(); }

    std::unique_ptr<Node> createHashOfBranchForThisNode() {
        auto hashOfBranch = std::make_unique<merkle::HashOfBranch>();
        hashOfBranch->setDirty(true);
        auto extView = extension();
        hashOfBranch->setExtension(ByteSequence{extView.begin(), extView.end()});
        return hashOfBranch;
    }

    static void setNullNodeHash() { computeSHA256<ByteSequence>(kNullNodeToHash, kNullNodeHash); }

    static std::unique_ptr<BranchNode> deserialize(const ByteSequence& in);
    void serialize(ByteSequence& out) const override;
    void deserialize(const ByteSequenceView& in, size_t& pos) override;

    std::ostream& print(std::ostream& os) const override {
        os << "BranchNode: hash " << Node::toHex(hash()) << " extension " << extension()
           << " leaf: " << (leaf_ == nullptr)
            ? os << "0"
            : leaf_->print(os) << "\n";
        os << "children:\n";
        for (int i = 0; i <= std::numeric_limits<Byte>::max(); ++i) {
            os << i << ": ";
            if (children_[i] == nullptr) {
                os << "0";
            } else {
                children_[i]->print(os);
            }
            os << " | ";
        }
        os << "\n";
        return os;
    }

   private:
    void swapLeaf(std::unique_ptr<Node>& other) { leaf_.swap(other); }
    ChildrenArray children_;
    std::unique_ptr<Node> leaf_;
};

};  // namespace merkle