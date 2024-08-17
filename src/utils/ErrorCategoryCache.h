#pragma once

#include <boost/system/error_code.hpp>

namespace Utils {

class ErrorCategoryCache {
public:
    static const boost::system::error_category& getSystemCategory() {
        static const boost::system::error_category& cat = boost::system::system_category();
        return cat;
    }

    static const boost::system::error_category& getGenericCategory() {
        static const boost::system::error_category& cat = boost::system::generic_category();
        return cat;
    }

    static boost::system::error_code makeErrorCode(int ev, const boost::system::error_category& cat) {
        static thread_local std::map<std::pair<int, const boost::system::error_category*>, boost::system::error_code> cache;
        auto key = std::make_pair(ev, &cat);
        auto it = cache.find(key);
        if (it == cache.end()) {
            it = cache.emplace(key, boost::system::error_code(ev, cat)).first;
        }
        return it->second;
    }
};

} // namespace Utils