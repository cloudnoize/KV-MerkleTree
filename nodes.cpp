#include "nodes.hpp"

#include <cstring>
namespace merkle {

void HashOfLeaf::updateHash(const ByteSequence& key, const ByteSequence& value) {
    size_t size = key.size();
    auto* size_p = reinterpret_cast<char*>(&size);
    ByteSequence to_hash;
    to_hash.reserve(sizeof(size) + key.size() + value.size());
    std::copy(size_p, size_p + sizeof(size), std::back_insert_iterator(to_hash));
    std::copy(key.cbegin(), key.cend(), std::back_insert_iterator(to_hash));
    std::copy(value.cbegin(), value.cend(), std::back_insert_iterator(to_hash));
    computeSHA256<ByteSequence>(to_hash, getMutableHash());
}

HashOfLeaf::HashOfLeaf(const ByteSequence& key, const ByteSequence& value) {
    updateHash(key, value);
}

const ByteSequence BranchNode::kNullNodeToHash = {0};
unsigned char BranchNode::kNullNodeHash[SHA256_DIGEST_LENGTH] = {};

void BranchNode::computeHash() {
    ByteSequence to_hash;
    if (leaf_ == nullptr) {
        to_hash.insert(to_hash.end(), kNullNodeHash, kNullNodeHash + SHA256_DIGEST_LENGTH);
    } else {
        to_hash.insert(to_hash.end(), leaf_->hash(), leaf_->hash() + SHA256_DIGEST_LENGTH);
    }
    for (const auto& child : children_) {
        if (child == nullptr) {
            to_hash.insert(to_hash.end(), kNullNodeHash, kNullNodeHash + SHA256_DIGEST_LENGTH);
        } else {
            to_hash.insert(to_hash.end(), child->hash(), child->hash() + SHA256_DIGEST_LENGTH);
        }
    }
    computeSHA256<ByteSequence>(to_hash, getMutableHash());
}

void BranchNode::updateHashOfLeafChild(Byte child, const ByteSequence& key,
                                       const ByteSequence& value) {
    assert(children_[child] != nullptr);
    assert(children_[child]->getType() == Node::Type::HashOfLeaf);
    auto* hashOfLeaf = static_cast<merkle::HashOfLeaf*>(children_[child].get());
    hashOfLeaf->updateHash(key, value);
}

void Node::serialize(ByteSequence& out) const {
    out.insert(out.end(), hash_, hash_ + SHA256_DIGEST_LENGTH);
    uint64_t extSize = extension_.size();
    assert(sizeof(extSize) == kSizeField);
    // TODO encode is big endian
    auto* pExtSize = reinterpret_cast<Byte*>(&extSize);
    out.insert(out.end(), pExtSize, pExtSize + kSizeField);
    out.insert(out.end(), extension_.cbegin(), extension_.cend());
}

void HashOfBranch::serialize(ByteSequence& out) const {
    out.push_back(static_cast<Byte>(Node::HashOfBranch));
    Node::serialize(out);
    out.push_back(static_cast<Byte>(is_dirty_));
}

void HashOfLeaf::serialize(ByteSequence& out) const {
    out.push_back(static_cast<Byte>(Node::HashOfLeaf));
    Node::serialize(out);
}

void BranchNode::serialize(ByteSequence& out) const {
    out.push_back(static_cast<Byte>(Node::BranchNode));
    Node::serialize(out);

    if (leaf_ == nullptr) {
        out.push_back(static_cast<Byte>(Node::NullNode));
    } else {
        leaf_->serialize(out);
    }

    for (const auto& child : children_) {
        if (child == nullptr) {
            out.push_back(static_cast<Byte>(Node::NullNode));
        } else {
            child->serialize(out);
        }
    }
}

std::unique_ptr<BranchNode> BranchNode::deserialize(const ByteSequence& in) {
    auto bn = std::make_unique<BranchNode>();
    size_t pos = 0;
    bn->deserialize(in, pos);
    return bn;
}

void Node::deserialize(const ByteSequenceView& in, size_t& pos) {
    std::memcpy(hash_, in.data() + pos, SHA256_DIGEST_LENGTH);
    pos += SHA256_DIGEST_LENGTH;
    uint64_t extSize = *(reinterpret_cast<const uint64_t*>(in.data() + pos));
    pos += kSizeField;
    extension_.insert(extension_.end(), in.begin() + pos, in.begin() + pos + extSize);
    pos += extSize;
}

void HashOfLeaf::deserialize(const ByteSequenceView& in, size_t& pos) {
    assert(in[pos] == Node::Type::HashOfLeaf);
    ++pos;
    Node::deserialize(in, pos);
}

void HashOfBranch::deserialize(const ByteSequenceView& in, size_t& pos) {
    assert(in[pos] == Node::Type::HashOfBranch);
    ++pos;
    Node::deserialize(in, pos);
    is_dirty_ = static_cast<bool>(in[pos]);
    ++pos;
}

void BranchNode::deserialize(const ByteSequenceView& in, size_t& pos) {
    assert(in.size() > 0);
    assert(static_cast<Node::Type>(in[0]) == Node::BranchNode);
    ++pos;
    Node::deserialize(in, pos);
    // leaf
    auto type = in[pos];
    if (type == Node::Type::NullNode) {
        ++pos;
        // continue
    } else if (type == Node::Type::HashOfLeaf) {
        leaf_.reset(new merkle::HashOfLeaf{});
        leaf_->deserialize(in, pos);
    } else {
        assert(false);
    }
    // children_
    for (auto& child : children_) {
        auto type = in[pos];
        if (type == Node::Type::NullNode) {
            ++pos;
            continue;
        } else if (type == Node::Type::HashOfLeaf) {
            child.reset(new merkle::HashOfLeaf{});

        } else if (type == Node::Type::HashOfBranch) {
            child.reset(new merkle::HashOfBranch{});
        } else {
            assert(false);
        }
        child->deserialize(in, pos);
    }
}

}  // namespace merkle
   // namespace merkle