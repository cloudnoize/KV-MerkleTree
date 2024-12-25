#include <cassert>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#pragma once

namespace merkle {

using Byte = uint8_t;
using ByteSequence = std::vector<Byte>;
using ByteSequenceView = std::span<const Byte>;

inline ByteSequenceView ByteSequenceToView(const ByteSequence& bs) {
    return ByteSequenceView{bs.begin(), bs.end()};
}

class ExtensionView {
   public:
    enum CompareResultType : uint8_t {
        equals = 0,
        diverge = 1,
        substring = 2,
        contains_other_extension
    };
    using CompareResult = std::pair<CompareResultType, size_t>;
    ExtensionView(ByteSequenceView ext) : extension(ext) {}
    size_t size() const { return extension.size(); }
    bool operator==(const ExtensionView& other) const {
        return extension.size() == other.extension.size() &&
               std::equal(extension.begin(), extension.end(), other.extension.begin());
    }

    CompareResult compareTo(const ExtensionView& other) const {
        return compareTo(other.getWholeExtension());
    };

    CompareResult compareTo(const ByteSequenceView&) const;

    const ByteSequenceView& getWholeExtension() const { return extension; }
    // Position related methods.
    ByteSequenceView getKeySoFar() const { return ByteSequenceView(extension.data(), position); }

    // E.L Do i have a test where extension.cbegin() + position >= extension.cend()? should return
    // empty
    ByteSequenceView getExtentionFromCurrentPosition() const {
        return ByteSequenceView{extension.begin() + position, extension.end()};
    }

    ByteSequenceView getExtentionFromCurrentPositionUntil(size_t until) const {
        auto itrUntil = (position + until) > extension.size()
                            ? extension.end()
                            : extension.begin() + position + until;
        return ByteSequenceView{extension.begin() + position, itrUntil};
    }

    std::optional<Byte> getCurrentByte() const {
        if (position >= extension.size()) {
            return std::nullopt;
        }
        return extension[position];
    }

    void incrementPositionBy(size_t steps) {
        auto expected = (position + steps);
        position = expected > extension.size() ? extension.size() : expected;
    }

    size_t getPosition() const { return position; }

   private:
    ByteSequenceView extension;
    size_t position = 0;
};

};  // namespace merkle

namespace std {
template <>
struct hash<merkle::ByteSequence> {
    size_t operator()(const merkle::ByteSequence& seq) const {
        return std::hash<std::string>{}(std::string(seq.begin(), seq.end()));
    }
};
}  // namespace std
