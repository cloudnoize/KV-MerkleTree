#include <gtest/gtest.h>

#include "../nodes.hpp"

using namespace merkle;

TEST(SerDer, HashOfLeaf) {
    HashOfLeaf orig(ByteSequence{'a', 'b', 'c'}, ByteSequence{'v', 'a'},
                    ByteSequence{'e', 'x', 't'});
    ByteSequence ser;
    orig.serialize(ser);
    HashOfLeaf fromSer;
    size_t pos = 0;
    fromSer.deserialize(ser, pos);
    ASSERT_TRUE(compareHashes(orig.hash(), fromSer.hash()));
    ASSERT_EQ(ser.size(), pos);
    auto res = CompareBytes{}(orig.extension(), fromSer.extension());
    ASSERT_TRUE(res);
}

TEST(SerDer, HashOfBranch) {
    HashOfBranch hob;
    hob.setDirty(true);
    hob.setExtension(ByteSequence{'e', 'x', 't'});
    auto* ph = hob.getMutableHash();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    for (Byte b = 0; b < SHA256_DIGEST_LENGTH; ++b) {
        hash[b] = b;
        ph[b] = b;
    }
    ASSERT_TRUE(compareHashes(hob.hash(), hash));
    ByteSequence ser;
    hob.serialize(ser);
    HashOfBranch fromSer;
    size_t pos = 0;
    fromSer.deserialize(ser, pos);
    ASSERT_TRUE(compareHashes(hob.hash(), fromSer.hash()));
    auto res = CompareBytes{}(hob.extension(), fromSer.extension());
    ASSERT_TRUE(res);
    ASSERT_TRUE(fromSer.isDirty());
    hob.setDirty(false);
    ser.clear();
    pos = 0;
    hob.serialize(ser);
    fromSer.deserialize(ser, pos);
    ASSERT_FALSE(fromSer.isDirty());
}

TEST(SerDer, BranchNode) {
    BranchNode branch;
    branch.setExtension(ByteSequence{2, 3, 4});
    //Hash
    branch.computeHash();
    unsigned char branchHash[SHA256_DIGEST_LENGTH];
    const auto* bh = branch.hash();
    std::memcpy(branchHash,bh,SHA256_DIGEST_LENGTH);
    ASSERT_TRUE(compareHashes(branchHash, bh));
    //Add hash of branch
    std::unique_ptr<Node> sp_hob = std::make_unique<HashOfBranch>();
    sp_hob->setExtension(ByteSequence{23, 24, 25});
    auto* ph = sp_hob->getMutableHash();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    for (Byte b = 0; b < SHA256_DIGEST_LENGTH; ++b) {
        hash[b] = b;
        ph[b] = b;
    }
    Byte hobPos = 12;
    ASSERT_TRUE(compareHashes(sp_hob->hash(), hash));
    branch.swapNodeAtChild(hobPos,sp_hob);
    //Ser and Der and compare
    ByteSequence ser;
    branch.serialize(ser);

    BranchNode fromSer;
    size_t pos = 0;
    fromSer.deserialize(ser, pos);
    //Compare hash
    ASSERT_TRUE(compareHashes(branch.hash(), fromSer.hash()));
    //Compare Extension
    auto res = CompareBytes{}(branch.extension(), fromSer.extension());
    ASSERT_TRUE(res);

    for(int i = 0 ; i <= std::numeric_limits<Byte>::max() ; ++ i){
        auto& c = fromSer.getChildAt(Byte(i));
        if(i == hobPos){
            ASSERT_EQ(c->getType(),Node::Type::HashOfBranch);
            auto res = CompareBytes{}(c->extension(), ByteSequence{23, 24, 25});
            ASSERT_TRUE(res);
            ASSERT_TRUE(compareHashes(c->hash(), hash));
            continue;
        }
        ASSERT_EQ(c,nullptr);
    }

}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
