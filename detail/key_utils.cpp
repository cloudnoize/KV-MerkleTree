#include "key_utils.hpp"

namespace merkle {

ExtensionView::CompareResult ExtensionView::compareTo(
    const ByteSequenceView& otherExtension) const {
    auto thisExtension = ByteSequenceView{extension.data() + position, extension.size() - position};
    if ((thisExtension.size() == otherExtension.size()) &&
        std::equal(thisExtension.begin(), thisExtension.end(), otherExtension.begin())) {
        return std::make_pair(CompareResultType::equals, thisExtension.size());
    }

    auto thisSize = thisExtension.size();
    auto otherSize = otherExtension.size();
    auto shorterSize = thisSize < otherSize ? thisSize : otherSize;

    size_t i = 0;
    for (; i < shorterSize; ++i) {
        if (thisExtension[i] != otherExtension[i]) {
            break;
        }
    }

    if (i == shorterSize) {
        auto type = shorterSize == thisExtension.size()
                        ? CompareResultType::substring
                        : CompareResultType::contains_other_extension;
        return std::make_pair(type, i);
    }

    return std::make_pair(CompareResultType::diverge, i);
}

};  // namespace merkle