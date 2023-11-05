#include <array>
#include <ranges>
#include <cassert>
#include <algorithm>

template<typename T, std::size_t MaxTransactions>
struct TransactionManager{
    private:
    std::array<std::pair<T, bool>, MaxTransactions> transactions{};

    public:
    T& getFreeTransaction(){
        auto it = std::ranges::find(transactions, false, [](auto const& pair){
            return pair.second;
        });
        assert(it != transactions.end());
        it->second = true;
        it->first = T{};
        return it->first;
    }

    void releaseTransaction(T const* trans){
        auto it = std::ranges::find(transactions, trans, [](auto& pair){
                return &pair.first;
            });
            assert(it != transactions.end());
            it->second = false;
    }
};
