#include "tree.hpp"

namespace merkle {

// void Tree::insert(ByteSequence&& key, ByteSequence&& value) {
//     auto hash_of_leaf = HashOfLeaf{key, value};
//     auto leaf = std::make_unique<HashOfLeaf>(hash_of_leaf);
//     auto leaf_ptr = leaf.get();
//     db_[key] = std::move(leaf);
//     auto extension = Extension{key};
//     auto current = root_.get();
//     for (size_t i = 0; i < extension.size(); i++) {
//         auto [res, num] = extension.compareTo(Extension{current->extension()});
//         if (res == Extension::equals) {
//             break;
//         }
//         if (res == Extension::diverge) {
//             auto branch = std::make_unique<BranchNode>();
//             auto branch_ptr = branch.get();
//             branch_ptr->setChild(extension.extension[i], std::move(leaf));
//             branch_ptr->setChild(current->extension()[i], std::move(current->children_[i]));
//             current->setExtension(extension.extension);
//             current->setChild(extension.extension[i], std::move(branch));
//             break;
//         }
//         if (res == Extension::substring) {
//             auto branch = std::make_unique<BranchNode>();
//             auto branch_ptr = branch.get();
//             branch_ptr->setChild(extension.extension[i], std::move(leaf));
//             branch_ptr->setChild(current->extension()[i], std::move(current->children_[i]));
//             current->setExtension(extension.extension);
//             current->setChild(extension.extension[i], std::move(branch));
//             current = branch_ptr;
//         }
//     }
void Tree::insert(ByteSequence&& key, ByteSequence&& value) {
    auto& branchNode = root_;
    ExtensionView extension{key};
    while (true) {
        auto [result, matchBytes] = extension.compareTo(branchNode->extension());
        if (result == ExtensionView::CompareResultType::equals) {
            branchNode->setLeaf(key, value);
            return;
        } else if (result == ExtensionView::CompareResultType::contains_other_extension) {
            // this means that the current branch node is on the path.
            extension.incrementPositionBy(matchBytes);
            auto optCurrentByte = extension.getCurrentByte();
            assert(optCurrentByte.has_value());
            auto currentByte = *optCurrentByte;
            auto nodeType = branchNode->getTypeOfChild(currentByte);
            if (nodeType == Node::Type::NullNode) {
                // Add the a leaf and set extension
                // Increment the extension by one as the current byte is being set as the child
                extension.incrementPositionBy(1);
                auto leafExtension = extension.getExtentionFromCurrentPosition();
                auto newLeaf = HashOfLeaf::createhashOfLeaf(
                    key, value, ByteSequence{leafExtension.begin(), leafExtension.end()});
                branchNode->swapNodeAtChild(currentByte, newLeaf);
                return;
            } else if (nodeType == Node::Type::HashOfLeaf) {
                // We have a leaf on the path, need to compare its extension to deduce the action.
                extension.incrementPositionBy(1);
                auto [result, matchBytes] =
                    extension.compareTo(branchNode->getChildAt(currentByte)->extension());
                if (result == ExtensionView::CompareResultType::equals) {
                    // it's an update
                    branchNode->updateHashOfLeafChild(currentByte, key, value);
                    return;
                }
                // the new inserted key and the existing leaf, shares a common path i.e. we need to
                // insert a new branch node.
                // the key in the db to the new branch node
                auto newBranchNodeKey = extension.getKeySoFar();
                // the common extension is the extension of the new branch node.
                auto newExtension = extension.getExtentionFromCurrentPositionUntil(matchBytes);
                auto newBranchNode = BranchNode::createBranchNode();
                newBranchNode->setExtension(ByteSequence{newExtension.begin(), newExtension.end()});
                // this will create the dirty hashof branch for this node, and will swap with the
                // hashofleaf
                auto nodeToSwap = newBranchNode->createHashOfBranchForThisNode();
                // After the swap, the node to swap holds the hash of leaf.
                branchNode->swapNodeAtChild(currentByte, nodeToSwap);

                // At this point the branchnode is updated to point to the new branchnode, now we
                // need to set the new branchnode to contain the old leaf and the new leaf.

                // Truncate the common extension from the old  leaf and update
                auto updatedExtension = ExtensionView{nodeToSwap->extension()};
                updatedExtension.incrementPositionBy(matchBytes);
                auto optByteToSetHashOfLeaf = updatedExtension.getCurrentByte();
                updatedExtension.incrementPositionBy(1);
                nodeToSwap->setExtension(
                    ByteSequence{updatedExtension.getExtentionFromCurrentPosition().begin(),
                                 updatedExtension.getExtentionFromCurrentPosition().end()});
                // nodeToSwap in null after the swap!
                newBranchNode->swapNodeAtChild(optByteToSetHashOfLeaf, nodeToSwap);

                // New leaf preparation
                // get to the next byte after the common extension
                extension.incrementPositionBy(matchBytes);
                auto optNewLeafCurrentByte = extension.getCurrentByte();
                // if current byte is nullopt it means that its path terminates at this node
                extension.incrementPositionBy(1);
                auto hashofleaf = HashOfLeaf::createhashOfLeaf(
                    key, value,
                    ByteSequence{extension.getExtentionFromCurrentPosition().begin(),
                                 extension.getExtentionFromCurrentPosition().end()});
                newBranchNode->swapNodeAtChild(optNewLeafCurrentByte, hashofleaf);

                db_.emplace(ByteSequence{newBranchNodeKey.begin(), newBranchNodeKey.end()},
                            newBranchNode.release());
            } else if (nodeType == Node::Type::HashOfBranch) {
                // TODO next child is branch node, this should be hash., set as dirty, load next
                // node and continue;
            } else {
                // TODO , we should not reach here, assert;
            }
        } else if (result == ExtensionView::CompareResultType::substring) {
            // TODO create new branchnode, with this key/value as its leaf, set its extension,
            // insert hashofbranch to link to current node, and change the current node extension.
        } else if (result == ExtensionView::CompareResultType::diverge) {
            // TODO new branch node with the common extension, insert hash of leaf with extension
            // for the new key/value,
        }
    }
}
};  // namespace merkle
