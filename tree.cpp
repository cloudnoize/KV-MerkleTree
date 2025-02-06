#include "tree.hpp"

namespace merkle {

void Tree::insert(ByteSequence&& key, ByteSequence&& value) {
    auto* branchNode = root_.get();
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
            extension.incrementPositionBy(1);
            if (nodeType == Node::Type::NullNode) {
                // Add the a leaf and set extension
                auto leafExtension = extension.getExtentionFromCurrentPosition();
                auto newLeaf = HashOfLeaf::createhashOfLeaf(
                    key, value, ByteSequence{leafExtension.begin(), leafExtension.end()});
                branchNode->swapNodeAtChild(currentByte, newLeaf);
                return;
            } else if (nodeType == Node::Type::HashOfLeaf) {
                // We have a leaf on the path, need to compare its extension to deduce the action.
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

                // Truncate the common extension from the old leaf and update
                nodeToSwap->truncateExtension(matchBytes);
                auto byteToSetHashOfLeaf = ExtensionView{nodeToSwap->extension()}.getCurrentByte();
                // if extension is not empty, the leaf node will be places at a child therefore need
                // to truncate the byte of that child from the extension
                if (byteToSetHashOfLeaf.has_value()) {
                    nodeToSwap->truncateExtension(1);
                }
                // nodeToSwap in null after the swap!
                newBranchNode->swapNodeAtChild(byteToSetHashOfLeaf, nodeToSwap);

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
                return;
            } else if (nodeType == Node::Type::HashOfBranch) {
                // TODO next child is branch node, this should be hash., set as dirty, load next
                // node and continue;
                auto nextbranchDbKey = extension.getKeySoFar();
                branchNode->setDirty(currentByte);
                branchNode = getMutableBranchNode(nextbranchDbKey).get();
                assert(branchNode != nullptr);
            } else {
                // TODO , we should not reach here, assert;
            }
        } else if (result == ExtensionView::CompareResultType::substring) {
            // Insert a new branch on the path which shares a substring of the current path and
            // point to the current branch node which its extension is truncated.
            auto currentBranchDbKey = extension.getKeySoFar();
            auto newBranchNode = BranchNode::createBranchNode();
            auto newExtensionView = extension.getExtentionFromCurrentPosition();
            newBranchNode->setExtension(
                ByteSequence{newExtensionView.begin(), newExtensionView.end()});
            newBranchNode->setLeaf(key, value);

            // truncate the extension of the older branchnode
            branchNode->truncateExtension(matchBytes);
            Byte nextByte = branchNode->extension()[0];
            branchNode->truncateExtension(1);
            // set the hashofBranch to point to the old branchnode
            auto hashOfBranch = branchNode->createHashOfBranchForThisNode();
            newBranchNode->swapNodeAtChild(nextByte, hashOfBranch);
            // At this phase the new branch node is ready with the leaf and pointing to the old
            // branch node, need to set it in the db with the key of the old branchnode
            auto& mutableBranchNode = getMutableBranchNode(currentBranchDbKey);
            mutableBranchNode.swap(newBranchNode);
            auto newDbKeyView = extension.getWholeExtension();
            auto newDbKEy = ByteSequence{newDbKeyView.begin(), newDbKeyView.end()};
            newDbKEy.push_back(nextByte);
            db_.emplace(std::move(newDbKEy), newBranchNode.release());
            return;
        } else if (result == ExtensionView::CompareResultType::diverge) {
            // TODO new branch node with the common extension, insert hash of leaf with extension
            // for the new key/value,
            auto currentBranchDbKey = extension.getKeySoFar();
            auto newBranchNode = BranchNode::createBranchNode();
            auto commonExtensionView = extension.getExtentionFromCurrentPositionUntil(matchBytes);
            newBranchNode->setExtension(
                ByteSequence{commonExtensionView.begin(), commonExtensionView.end()});
            extension.incrementPositionBy(matchBytes);
            auto newBranchDbKey = extension.getKeySoFar();
            auto optCurrentByte = extension.getCurrentByte();
            assert(optCurrentByte.has_value());
            auto currentByte = *optCurrentByte;
            extension.incrementPositionBy(1);
            {
                auto hashofleaf = HashOfLeaf::createhashOfLeaf(
                    key, value,
                    ByteSequence{extension.getExtentionFromCurrentPosition().begin(),
                                 extension.getExtentionFromCurrentPosition().end()});
                newBranchNode->swapNodeAtChild(currentByte, hashofleaf);
            }

            // truncate the extension of the older branchnode
            branchNode->truncateExtension(matchBytes);
            Byte nextByte = branchNode->extension()[0];
            branchNode->truncateExtension(1);

            // set the hashofBranch to point to the old branchnode
            auto hashOfBranch = branchNode->createHashOfBranchForThisNode();
            newBranchNode->swapNodeAtChild(nextByte, hashOfBranch);

            // At this phase the new branch node is ready with the leaf and pointing to the old
            // branch node, need to set it in the db with the key of the old branchnode
            auto& mutableBranchNode = getMutableBranchNode(currentBranchDbKey);
            mutableBranchNode.swap(newBranchNode);
            auto newDbKEy = ByteSequence{newBranchDbKey.begin(), newBranchDbKey.end()};
            newDbKEy.push_back(nextByte);
            db_.emplace(std::move(newDbKEy), newBranchNode.release());
            return;
        }
    }
}

};  // namespace merkle
