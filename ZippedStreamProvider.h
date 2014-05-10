#ifndef ZIPPED_STREAM_PROVIDER
#define ZIPPED_STREAM_PROVIDER

#include "StreamProvider.h"

#include <tuple>

namespace detail {

template<typename L, typename R>
struct ZipperImpl {
    using Type = std::tuple<L, R>;

    static Type zip(L&& a, R&& b) {
        return std::make_tuple<L, R>(std::move(a), std::move(b));
    }
};

template<typename L, typename... RArgs>
struct ZipperImpl<L, std::tuple<RArgs...>> {
    using Type = std::tuple<L, RArgs...>;

    static Type zip(L&& a, std::tuple<RArgs...>&& b) {
        return std::tuple_cat(std::make_tuple<L>(std::move(a)), std::move(b));
    }
};


template<typename... LArgs, typename R>
struct ZipperImpl<std::tuple<LArgs...>, R> {
    using Type = std::tuple<LArgs..., R>;

    static Type zip(std::tuple<LArgs...>&& a, R&& b) {
        return std::tuple_cat(std::move(a), std::make_tuple<R>(std::move(b)));
    }
};

template<typename... LArgs, typename... RArgs>
struct ZipperImpl<std::tuple<LArgs...>, std::tuple<RArgs...>> {
    using Type = std::tuple<LArgs..., RArgs...>;

    static Type zip(std::tuple<LArgs...>&& a, std::tuple<RArgs...>&& b) {
        return std::tuple_cat(std::move(a), std::move(b));
    }
};

} /* detail */

struct Zipper {
    template<typename L, typename R>
    auto operator() (L&& left, R&& right) {
        using Left = typename std::decay<L>::type;
        using Right = typename std::decay<R>::type;
        return detail::ZipperImpl<Left, Right>::zip(std::forward<Left>(left),
                                                    std::forward<Right>(right));
    }
};

template<typename L, typename R, typename Function>
class ZippedStreamProvider : public StreamProvider<ReturnType<Function, L&, R&>> {

public:
    using ValueType = ReturnType<Function, L&, R&>;

    ZippedStreamProvider(StreamProviderPtr<L> left_source,
                         StreamProviderPtr<R> right_source,
                         Function&& zipper)
        : left_source_(std::move(left_source)),
          right_source_(std::move(right_source)),
          zipper_(zipper) {}

    std::shared_ptr<ValueType> get() override {
        return current_;
    }

    bool advance() override {
        if(left_source_->advance() && right_source_->advance()) {
            current_ = std::make_shared<ValueType>(
                zipper_(std::move(*left_source_->get()),
                        std::move(*right_source_->get())));
            return true;
        }
        current_.reset();
        return false;
    }

private:
    StreamProviderPtr<L> left_source_;
    StreamProviderPtr<R> right_source_;
    std::shared_ptr<ValueType> current_;
    Function zipper_;
};

#endif
