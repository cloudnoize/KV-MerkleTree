#include "nodes.hpp"

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

}  // namespace merkle