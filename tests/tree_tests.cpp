#include <gtest/gtest.h>

#include "../tree.hpp"

using namespace merkle;

std::string hashToString(const unsigned char* hash) {
    return std::string(reinterpret_cast<const char*>(hash), SHA256_DIGEST_LENGTH);
}

TEST(Tree, empty) {
    Tree tree;
    ByteSequence bs{'a'};
    const auto& bn = tree.getBranchNode(bs);
    ASSERT_EQ(bn, nullptr);
    const auto& root = tree.getRootNode();
    for (Byte i = 0;; ++i) {
        auto type = root->getTypeOfChild(i);
        ASSERT_EQ(type, Node::Type::NullNode);
        if (i == std::numeric_limits<Byte>::max()) {
            break;
        }
    }
    auto leafType = root->getTypeOfChild(BranchNode::LeafChildPos);
    ASSERT_EQ(leafType, Node::Type::NullNode);
}

TEST(Tree, insert_single_leaf) {
    Tree tree;
    {
        ByteSequence key{'a', 'b', 'c'};
        ByteSequence value{'a'};
        tree.insert(std::move(key), std::move(value));
    }
    const auto& root = tree.getRootNode();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    {
        auto type = root->getTypeOfChild('a');
        ASSERT_EQ(type, Node::Type::HashOfLeaf);
        const auto& node = root->getChildAt('a');
        const auto* pHash = node->hash();
        std::copy(pHash, pHash + SHA256_DIGEST_LENGTH, hash);
        auto ext = node->extension();
        auto expectedExtension = ByteSequence{'b', 'c'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
    }
    // update leaf
    {
        ByteSequence key{'a', 'b', 'c'};
        ByteSequence value{'a', 'a'};
        tree.insert(std::move(key), std::move(value));
    }
    unsigned char updatedHash[SHA256_DIGEST_LENGTH];
    {
        auto type = root->getTypeOfChild('a');
        ASSERT_EQ(type, Node::Type::HashOfLeaf);
        const auto& node = root->getChildAt('a');
        const auto* pHash = node->hash();
        std::copy(pHash, pHash + SHA256_DIGEST_LENGTH, updatedHash);
        auto ext = node->extension();
        auto expectedExtension = ByteSequence{'b', 'c'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
        ASSERT_NE(hash, updatedHash);
    }
}

TEST(Tree, insert_leaf_and_promote_to_branch) {
    Tree tree;
    {
        ByteSequence key{'a', 'b', 'd', 'f'};
        ByteSequence value{'a'};
        tree.insert(std::move(key), std::move(value));
    }
    const auto& root = tree.getRootNode();
    {
        auto type = root->getTypeOfChild('a');
        ASSERT_EQ(type, Node::Type::HashOfLeaf);
        const auto& node = root->getChildAt('a');
        auto ext = node->extension();
        auto expectedExtension = ByteSequence{'b', 'd', 'f'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
    }
    // insert new leaf, should create a new branch node
    {
        ByteSequence key{'a', 'b', 'd', 'e', 'd', 'm'};
        ByteSequence value{'a', 'a'};
        tree.insert(std::move(key), std::move(value));
    }

    {
        auto type = root->getTypeOfChild('a');
        ASSERT_EQ(type, Node::Type::HashOfBranch);
        const auto& hashOfBranch = root->getChildAt('a');
        auto ext = hashOfBranch->extension();
        auto expectedExtension = ByteSequence{'b', 'd'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
    }
    // get new branch node
    {
        const auto& branchNode = tree.getBranchNode(ByteSequence{'a'});
        ASSERT_NE(branchNode, nullptr);
        auto type = branchNode->getType();
        ASSERT_EQ(type, Node::Type::BranchNode);
        auto ext = branchNode->extension();
        auto expectedExtension = ByteSequence{'b', 'd'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
        const auto& leafAtF = branchNode->getChildAt('f');
        ASSERT_EQ(leafAtF->getType(), Node::Type::HashOfLeaf);
        auto leafAtFExt = leafAtF->extension();
        ASSERT_EQ(leafAtFExt.size(), 0);

        const auto& leafAtE = branchNode->getChildAt('e');
        ASSERT_EQ(leafAtE->getType(), Node::Type::HashOfLeaf);
        auto leafAtEExt = leafAtE->extension();
        auto expectedExtensionE = ByteSequence{'d', 'm'};
        auto eqE = std::ranges::equal(leafAtEExt.begin(), leafAtEExt.end(),
                                      expectedExtensionE.begin(), expectedExtensionE.end());
        ASSERT_TRUE(eqE);
    }
}

TEST(Tree, existing_leaf_is_on_path_to_new_leaf) {
    Tree tree;
    {
        ByteSequence key{'a', 'b', 'd', 'f'};
        ByteSequence value{'a'};
        tree.insert(std::move(key), std::move(value));
    }
    const auto& root = tree.getRootNode();
    {
        auto type = root->getTypeOfChild('a');
        ASSERT_EQ(type, Node::Type::HashOfLeaf);
        const auto& node = root->getChildAt('a');
        auto ext = node->extension();
        auto expectedExtension = ByteSequence{'b', 'd', 'f'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
    }
    // insert new leaf on the path of prev leaf, should create a new branch node
    {
        ByteSequence key{'a', 'b', 'd', 'f', 'd', 'm'};
        ByteSequence value{'a', 'a'};
        tree.insert(std::move(key), std::move(value));
    }

    {
        auto type = root->getTypeOfChild('a');
        ASSERT_EQ(type, Node::Type::HashOfBranch);
        const auto& hashOfBranch = root->getChildAt('a');
        auto ext = hashOfBranch->extension();
        auto expectedExtension = ByteSequence{'b', 'd', 'f'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
    }
    // get new branch node
    {
        const auto& branchNode = tree.getBranchNode(ByteSequence{'a'});
        ASSERT_NE(branchNode, nullptr);
        auto type = branchNode->getType();
        ASSERT_EQ(type, Node::Type::BranchNode);
        auto ext = branchNode->extension();
        auto expectedExtension = ByteSequence{'b', 'd', 'f'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
        const auto& leafOfBranch = branchNode->getChildAt(BranchNode::LeafChildPos);
        ASSERT_EQ(leafOfBranch->getType(), Node::Type::HashOfLeaf);
        auto leafOfBranchExt = leafOfBranch->extension();
        ASSERT_EQ(leafOfBranchExt.size(), 0);

        const auto& leafAtD = branchNode->getChildAt('d');
        ASSERT_EQ(leafAtD->getType(), Node::Type::HashOfLeaf);
        auto leafAtDExt = leafAtD->extension();
        auto expectedExtensionD = ByteSequence{'m'};
        auto eqD = std::ranges::equal(leafAtDExt.begin(), leafAtDExt.end(),
                                      expectedExtensionD.begin(), expectedExtensionD.end());
        ASSERT_TRUE(eqD);
    }
}

TEST(Tree, new_leaf_is_substring_of_existing_leaf) {
    Tree tree;
    {
        ByteSequence key{'b', 'd', 'f', 'd', 'm'};
        ByteSequence value{'a'};
        tree.insert(std::move(key), std::move(value));
    }
    const auto& root = tree.getRootNode();
    {
        auto type = root->getTypeOfChild('b');
        ASSERT_EQ(type, Node::Type::HashOfLeaf);
        const auto& node = root->getChildAt('b');
        auto ext = node->extension();
        auto expectedExtension = ByteSequence{'d', 'f', 'd', 'm'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);

        // Compare expected hash
        auto hashOfLeaf = HashOfLeaf{{'b', 'd', 'f', 'd', 'm'}, {'a'}};
        ASSERT_EQ(hashToString(node->hash()), hashToString(hashOfLeaf.hash()));
    }
    // insert new leaf on the path of prev leaf, should create a new branch node
    {
        ByteSequence key{'b', 'd', 'f'};
        ByteSequence value{'a', 'a'};
        tree.insert(std::move(key), std::move(value));
    }

    {
        auto type = root->getTypeOfChild('b');
        ASSERT_EQ(type, Node::Type::HashOfBranch);
        const auto& hashOfBranch = root->getChildAt('b');
        auto ext = hashOfBranch->extension();
        auto expectedExtension = ByteSequence{'d', 'f'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
    }
    // get new branch node
    {
        const auto& branchNode = tree.getBranchNode(ByteSequence{'b'});
        ASSERT_NE(branchNode, nullptr);
        auto type = branchNode->getType();
        ASSERT_EQ(type, Node::Type::BranchNode);
        auto ext = branchNode->extension();
        auto expectedExtension = ByteSequence{'d', 'f'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);

        const auto& leafOfBranch = branchNode->getChildAt(BranchNode::LeafChildPos);
        ASSERT_EQ(leafOfBranch->getType(), Node::Type::HashOfLeaf);
        auto leafOfBranchExt = leafOfBranch->extension();
        ASSERT_EQ(leafOfBranchExt.size(), 0);
        // Compare expected hash
        {
            auto hashOfLeaf = HashOfLeaf{{'b', 'd', 'f'}, {'a', 'a'}};
            ASSERT_EQ(hashToString(leafOfBranch->hash()), hashToString(hashOfLeaf.hash()));
        }

        const auto& leafAtD = branchNode->getChildAt('d');
        ASSERT_EQ(leafAtD->getType(), Node::Type::HashOfLeaf);
        auto leafAtDExt = leafAtD->extension();
        auto expectedExtensionD = ByteSequence{'m'};
        auto eqD = std::ranges::equal(leafAtDExt.begin(), leafAtDExt.end(),
                                      expectedExtensionD.begin(), expectedExtensionD.end());

        {
            auto hashOfLeaf = HashOfLeaf{{'b', 'd', 'f', 'd', 'm'}, {'a'}};
            ASSERT_EQ(hashToString(leafAtD->hash()), hashToString(hashOfLeaf.hash()));
        }
        ASSERT_TRUE(eqD);
    }
}

TEST(Tree, continue_on_branch_node) {
    Tree tree;
    {
        ByteSequence key{'b', 'd', 'f', 'k', 'm'};
        ByteSequence value{'a'};
        tree.insert(std::move(key), std::move(value));
    }
    // insert new leaf on the path of prev leaf, should create a new branch node
    {
        ByteSequence key{'b', 'd', 'f'};
        ByteSequence value{'a', 'a'};
        tree.insert(std::move(key), std::move(value));
    }
    {
        ByteSequence key{'b'};
        auto& branchNode = tree.getBranchNode(key);
        ASSERT_NE(branchNode, nullptr);
        auto& node = branchNode->getChildAt(BranchNode::LeafChildPos);
        ASSERT_EQ(node->getType(), merkle::Node::HashOfLeaf);
        auto& node2 = branchNode->getChildAt('k');
        ASSERT_EQ(node2->getType(), merkle::Node::HashOfLeaf);
    }
    {
        ByteSequence key{'b', 'd', 'f', 'k', 't', 't'};
        ByteSequence value{'a'};
        tree.insert(std::move(key), std::move(value));
    }
    {
        ByteSequence key{'b', 'd', 'f', 'k'};
        auto& branchNode = tree.getBranchNode(key);
        ASSERT_NE(branchNode, nullptr);
        ASSERT_EQ(branchNode->getType(), merkle::Node::BranchNode);
    }
}

TEST(Tree, new_key_is_substring) {
    Tree tree;
    {
        ByteSequence key{'b', 'd', 'f', 'k', 'l', 'm'};
        ByteSequence value{'a'};
        tree.insert(std::move(key), std::move(value));
    }
    // insert new leaf on the path of prev leaf, should create a new branch node
    {
        ByteSequence key{'b', 'd', 'f', 'k', 'l'};
        ByteSequence value{'a', 'a'};
        tree.insert(std::move(key), std::move(value));
    }
    {
        ByteSequence key{'b'};
        auto& branchNode = tree.getBranchNode(key);
        ASSERT_NE(branchNode, nullptr);
        auto& node = branchNode->getChildAt(BranchNode::LeafChildPos);
        ASSERT_EQ(node->getType(), merkle::Node::HashOfLeaf);
        auto& node2 = branchNode->getChildAt('m');
        ASSERT_EQ(node2->getType(), merkle::Node::HashOfLeaf);
    }
    {
        ByteSequence key{'b', 'd'};
        ByteSequence value{'a'};
        tree.insert(std::move(key), std::move(value));
    }
    {
        ByteSequence key{'b'};
        auto& branchNode = tree.getBranchNode(key);
        ASSERT_NE(branchNode, nullptr);
        ASSERT_EQ(branchNode->getType(), merkle::Node::BranchNode);
        auto ext = branchNode->extension();
        auto expectedExtension = ByteSequence{'d'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
    }
    {
        ByteSequence key{'b', 'd', 'f'};
        auto& branchNode = tree.getBranchNode(key);
        ASSERT_NE(branchNode, nullptr);
        ASSERT_EQ(branchNode->getType(), merkle::Node::BranchNode);
        auto ext = branchNode->extension();
        auto expectedExtension = ByteSequence{'k', 'l'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
    }
}

TEST(Tree, new_key_diverges) {
    Tree tree;
    {
        ByteSequence key{'b', 'd', 'f', 'k', 'l', 'm'};
        ByteSequence value{'a'};
        tree.insert(std::move(key), std::move(value));
    }
    // insert new leaf on the path of prev leaf, should create a new branch node
    {
        ByteSequence key{'b', 'd', 'f', 'k', 'l'};
        ByteSequence value{'a', 'a'};
        tree.insert(std::move(key), std::move(value));
    }
    {
        ByteSequence key{'b'};
        auto& branchNode = tree.getBranchNode(key);
        ASSERT_NE(branchNode, nullptr);
        auto& node = branchNode->getChildAt(BranchNode::LeafChildPos);
        ASSERT_EQ(node->getType(), merkle::Node::HashOfLeaf);
        auto& node2 = branchNode->getChildAt('m');
        ASSERT_EQ(node2->getType(), merkle::Node::HashOfLeaf);
        auto ext = branchNode->extension();
        auto expectedExtension = ByteSequence{'d', 'f', 'k', 'l'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
    }
    // diverge
    {
        ByteSequence key{'b', 'd', 'f', 'g', 'q'};
        ByteSequence value{'a'};
        tree.insert(std::move(key), std::move(value));
    }
    {
        ByteSequence key{'b'};
        auto& branchNode = tree.getBranchNode(key);
        ASSERT_NE(branchNode, nullptr);
        ASSERT_EQ(branchNode->getType(), merkle::Node::BranchNode);
        auto ext = branchNode->extension();
        auto expectedExtension = ByteSequence{'d', 'f'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
        {
            auto& node = branchNode->getChildAt('k');
            ASSERT_EQ(node->getType(), merkle::Node::HashOfBranch);
        }
        {
            auto& node = branchNode->getChildAt('g');
            ASSERT_EQ(node->getType(), merkle::Node::HashOfLeaf);
            auto ext = node->extension();
            auto expectedExtension = ByteSequence{'q'};
            auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                         expectedExtension.end());
            ASSERT_TRUE(eq);
        }
    }
    {
        ByteSequence key{'b', 'd', 'f', 'k'};
        auto& branchNode = tree.getBranchNode(key);
        ASSERT_NE(branchNode, nullptr);
        ASSERT_EQ(branchNode->getType(), merkle::Node::BranchNode);
        auto ext = branchNode->extension();
        auto expectedExtension = ByteSequence{'l'};
        auto eq = std::ranges::equal(ext.begin(), ext.end(), expectedExtension.begin(),
                                     expectedExtension.end());
        ASSERT_TRUE(eq);
        auto& node = branchNode->getChildAt(BranchNode::LeafChildPos);
        ASSERT_EQ(node->getType(), merkle::Node::HashOfLeaf);
        auto& node2 = branchNode->getChildAt('m');
        ASSERT_EQ(node2->getType(), merkle::Node::HashOfLeaf);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
