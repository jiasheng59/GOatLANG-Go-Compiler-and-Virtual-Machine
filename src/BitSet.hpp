#ifndef BITSET_HPP
#define BITSET_HPP

#include <memory>

#include "Common.hpp"

class BitSet
{
public:
    using word_type = u64;
    static constexpr u64 bits_per_word = sizeof(word_type) * 8;

    BitSet(u64 nbits)
    {
        u64 nwords = (nbits / bits_per_word) + (nbits % bits_per_word != 0);
        words = std::make_unique<word_type[]>(nwords);
    }

    BitSet(const BitSet& other) = delete;
    BitSet& operator=(const BitSet& other) = delete;

    BitSet(BitSet&& other) : words{std::move(other.words)} {}

    BitSet& operator=(BitSet&& other)
    {
        words = std::move(other.words);
        return *this;
    }

    void set(u64 index)
    {
        u64 word_index = index / bits_per_word;
        u64 bit_index = index % bits_per_word;
        words[word_index] |= 1 << bit_index;
    }

    bool get(u64 index) const
    {
        u64 word_index = index / bits_per_word;
        u64 bit_index = index % bits_per_word;
        return words[word_index] & (1 << bit_index);
    }

private:
    std::unique_ptr<word_type[]> words;
};

#endif /* BITSET_HPP */
