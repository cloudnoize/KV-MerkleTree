#include <gtest/gtest.h>

#include "../detail/key_utils.hpp"
#include "../nodes.hpp"

using namespace merkle;

merkle::ByteSequence convertByteSequenceView(const merkle::ByteSequenceView& view) {
    return merkle::ByteSequence{view.begin(), view.end()};
}

merkle::ByteSequence convertString(const std::string& str) {
    return merkle::ByteSequence{str.begin(), str.end()};
}

merkle::ByteSequenceView convertStringToView(const std::string& str) {
    return merkle::ByteSequenceView(reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

namespace std {
bool operator==(const std::span<const unsigned char>& lhs,
                const std::span<const unsigned char>& rhs) {
    return rhs.size() == lhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}
};  // namespace std

TEST(Extension, compare_to_equals_no_position) {
    {
        auto str = std::string{"abcd"};
        auto bsView = convertStringToView(str);
        auto ext = merkle::ExtensionView{bsView};
        auto str2 = std::string{"abcd"};
        auto bsView2 = convertStringToView(str2);
        auto ext2 = merkle::ExtensionView{bsView2};
        auto [res, num] = ext.compareTo(ext2);
        ASSERT_EQ(res, merkle::ExtensionView::equals);
        ASSERT_EQ(num, ext.size());
    }

    {
        auto emptyStr1 = std::string{""};
        auto emptyView1 = convertStringToView(emptyStr1);
        auto ext = merkle::ExtensionView{emptyView1};
        auto emptyStr2 = std::string{""};
        auto emptyView2 = convertStringToView(emptyStr2);
        auto ext2 = merkle::ExtensionView{emptyView2};
        auto [res, num] = ext.compareTo(ext2);
        ASSERT_EQ(res, merkle::ExtensionView::equals);
        ASSERT_EQ(num, ext.size());
    }
}

TEST(Extension, compare_to_substrings_no_position) {
    {
        auto str = std::string("abcd");
        auto bsView = convertStringToView(str);
        auto ext = merkle::ExtensionView{bsView};
        auto str2 = std::string("abcdde");
        auto bsView2 = convertStringToView(str2);
        auto ext2 = merkle::ExtensionView{bsView2};
        {
            auto [res, num] = ext.compareTo(ext2);
            ASSERT_EQ(res, merkle::ExtensionView::substring);
            ASSERT_EQ(num, ext.size());
        }
        {
            auto [res, num] = ext2.compareTo(ext);
            ASSERT_EQ(res, merkle::ExtensionView::contains_other_extension);
            ASSERT_EQ(num, ext.size());
        }
    }

    {
        auto str = std::string("");
        auto bsView = convertStringToView(str);
        auto ext = merkle::ExtensionView{bsView};
        auto str2 = std::string("abcdde");
        auto bsView2 = convertStringToView(str2);
        auto ext2 = merkle::ExtensionView{bsView2};

        auto [res, num] = ext.compareTo(ext2);
        ASSERT_EQ(res, merkle::ExtensionView::substring);
        ASSERT_EQ(num, ext.size());
    }
}

TEST(Extension, compare_to_diverges_no_position) {
    {
        auto common = std::string("123");
        auto str = common + std::string("f");
        auto bsView = convertStringToView(str);
        auto ext = merkle::ExtensionView{bsView};
        auto str2 = common + std::string("ggg");
        auto bsView2 = convertStringToView(str2);
        auto ext2 = merkle::ExtensionView{bsView2};
        auto [res, num] = ext.compareTo(ext2);
        ASSERT_EQ(res, merkle::ExtensionView::diverge);
        ASSERT_EQ(num, common.size());
    }
}

TEST(Extension, move_position) {
    auto origExtension = std::string("fgde");
    auto bsView = convertStringToView(origExtension);
    auto ext = merkle::ExtensionView{bsView};
    ASSERT_TRUE(ext.getCurrentByte().has_value());
    ASSERT_EQ(*ext.getCurrentByte(), origExtension[ext.getPosition()]);
    ext.incrementPositionBy(2);
    ASSERT_EQ(*ext.getCurrentByte(), origExtension[ext.getPosition()]);
    ext.incrementPositionBy(5);
    ASSERT_EQ(ext.getPosition(), origExtension.size());
    ASSERT_FALSE(ext.getCurrentByte().has_value());
    ext.incrementPositionBy(1);
    ASSERT_EQ(ext.getPosition(), origExtension.size());
    ASSERT_FALSE(ext.getCurrentByte().has_value());
}

TEST(Extension, get_extension) {
    auto origExtension = std::string("fgde");
    auto bsView = convertStringToView(origExtension);
    auto ext = merkle::ExtensionView{bsView};
    const auto& wholeExt = ext.getWholeExtension();
    {
        auto extFromCurrentPosition = ext.getExtentionFromCurrentPosition();
        ASSERT_EQ(wholeExt, extFromCurrentPosition);
        auto keySoFarView = ext.getKeySoFar();
        ASSERT_EQ(keySoFarView.size(), 0);
    }

    ASSERT_EQ(origExtension[ext.getPosition()], *ext.getCurrentByte());
    for (; ext.getPosition() < ext.size(); ext.incrementPositionBy(1)) {
        ASSERT_EQ(origExtension[ext.getPosition()], *ext.getCurrentByte());
        auto extFromCurrentPosition = ext.getExtentionFromCurrentPosition();
        merkle::ByteSequence expected;
        auto pos = ext.getPosition();
        auto expectedStr = origExtension.substr(pos, origExtension.size());
        auto bsView = convertStringToView(expectedStr);
        ASSERT_EQ(bsView, extFromCurrentPosition);
        auto keySoFarView = ext.getKeySoFar();
        ASSERT_EQ(keySoFarView.size(), ext.getPosition());
    }
}

TEST(Extension, compare_with_position) {
    auto extStr1 = std::string("fggg");
    auto bsView = convertStringToView(extStr1);
    auto ext = merkle::ExtensionView{bsView};
    auto extStr2 = std::string("ggg");
    auto bsView2 = convertStringToView(extStr2);
    auto ext2 = merkle::ExtensionView{bsView2};
    {
        auto [res, num] = ext.compareTo(ext2);
        ASSERT_EQ(res, merkle::ExtensionView::diverge);
        ASSERT_EQ(num, 0);
    }

    ext.incrementPositionBy(1);
    {
        auto [res, num] = ext.compareTo(ext2);
        ASSERT_EQ(res, merkle::ExtensionView::equals);
        ASSERT_EQ(num, ext2.size());
    }

    ext.incrementPositionBy(1);
    {
        auto [res, num] = ext.compareTo(ext2);
        ASSERT_EQ(res, merkle::ExtensionView::substring);
        ASSERT_EQ(res, 2);
    }

    ext.incrementPositionBy(1);
    {
        auto [res, num] = ext.compareTo(ext2);
        ASSERT_EQ(res, merkle::ExtensionView::substring);
        ASSERT_EQ(num, 1);
    }
    ext.incrementPositionBy(1);
    {
        auto [res, num] = ext.compareTo(ext2);
        ASSERT_EQ(res, merkle::ExtensionView::substring);
        ASSERT_EQ(num, 0);
    }
    {
        auto shouldBeEmptyExt = ext.getExtentionFromCurrentPosition();
        ASSERT_EQ(shouldBeEmptyExt.size(), 0);
    }

    ext.incrementPositionBy(1);
    {
        auto shouldBeEmptyExt = ext.getExtentionFromCurrentPosition();
        ASSERT_EQ(shouldBeEmptyExt.size(), 0);
    }
}

TEST(Extension, get_extensiom_until) {
    auto extStr1 = std::string("fgggklo");
    auto bsView = convertStringToView(extStr1);
    auto ext = merkle::ExtensionView{bsView};
    size_t pos = 0;
    for (; pos < ext.size(); ++pos) {
        auto until = pos;
        for (; until <= ext.size(); ++until) {
            auto extUntil = ext.getExtentionFromCurrentPositionUntil(until);
            auto strUntil = extStr1.substr(pos, until);
            auto bsUntilView = convertStringToView(strUntil);
            auto extStrUntil = merkle::ExtensionView{bsUntilView};
            ASSERT_EQ(extUntil, extStrUntil);
        }
        ext.incrementPositionBy(1);
    }
}

TEST(ByteSequence, less_than) {
    {
        ByteSequence key{'a', 'b', 'd', 'f', 'd', 'm'};
        ByteSequence key2{'a', 'b', 'd', 'f', 'd'};
        auto key2View = ByteSequenceToView(key2);
        auto res = CompareBytes{}(key2View, key);
        ASSERT_TRUE(res);
    }

    {
        ByteSequence key{'a', 'b', 'd', 'f', 'd', 'm'};
        ByteSequence key2{'a', 'b', 'd', 'f', 'd', 'z'};
        auto key2View = ByteSequenceToView(key2);
        auto res = CompareBytes{}(key2View, key);
        ASSERT_FALSE(res);
    }

    {
        ByteSequence key{'a', 'b', 'd', 'f', 'd'};
        ByteSequence key2{'a', 'b', 'd', 'f', 'd'};
        auto key2View = ByteSequenceToView(key2);
        auto res = CompareBytes{}(key2View, key);
        ASSERT_FALSE(res);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
