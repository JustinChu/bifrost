#ifndef BFG_COMPRESSED_COVERAGE_HPP
#define BFG_COMPRESSED_COVERAGE_HPP

#include <cstring>
#include <stdint.h>
#include <vector>

using std::vector;
using std::pair;

/* Short description:
 *  - Tagged pointer union that is either
 *    - a pointer to a char array that stores 2-bit integers
 *    - a local 2-bit integer array
 *  - The bits are stored either as
 *    pppppppp|pppppppp|pppppppp|pppppppp|pppppppp|pppppppp|pppppppp|ppppppF0
 *    dddddddd|dddddddd|dddddddd|dddddddd|dddddddd|dddddddd|dddddddd|ssssssF1
 *
 *    - Last bit is 0 for pointer and 1 for local array
 *    - Second last bit is 1 for a full coverage and 0 for non-full
 *    - For local array last 8 except last 2 bits store size of the array
 *    - For the pointer version first 62 bits encode the pointer (last two bits
 *      are zeroed out before dereferencing)
 *    - The pointer points to an array of bytes, where the first 8 encode the
 *      size of the array used in uint32_t and the number of full positions.
 *    - The remainder of bytes are 2-bit encoded integers.
 *    - If the full bit is set then the pointer must be 0 and the memory released
 * */
class CompressedCoverage {

    public:

        CompressedCoverage(size_t sz=0, bool full=false);
        ~CompressedCoverage();

        // Copy constructors
        CompressedCoverage(const CompressedCoverage& o);
        CompressedCoverage& operator=(const CompressedCoverage& o);

        // move constructors
        CompressedCoverage(CompressedCoverage&& o);
        CompressedCoverage& operator=(CompressedCoverage&& o);

        void initialize(const size_t sz, const bool full);

        void cover(size_t start, size_t end);
        uint8_t covAt(const size_t index) const;

        bool isFull() const;
        void setFull();

        vector<pair<int, int>> splittingVector() const;
        pair<size_t, size_t> lowCoverageInfo() const;

        size_t size() const;
        std::string toString() const; // for debugging

        static const size_t size_limit = 28; // 56 bit array, 28 2-bit integers
        static const size_t cov_full = 2;

    private:

        static const uintptr_t tagMask = 1; // local array bit
        static const uintptr_t fullMask = 2; // full bit
        static const uintptr_t sizeMask = 0xFC; // 0b11111100
        static const uintptr_t localCoverageMask = 0xAAAAAAAAAAAAAA; // 0b10101010101010101010101010101010101010101010101010101010
        static const uintptr_t pointerMask = ~(tagMask | fullMask); // rest of bits

        inline size_t round_to_bytes(const size_t len) const { return (len + 3) / 4; }

        inline uint8_t *get8Pointer() const { return reinterpret_cast<uint8_t *>(asBits & pointerMask); }
        inline uint32_t *get32Pointer() const { return reinterpret_cast<uint32_t *>(asBits & pointerMask); }
        inline const uint32_t *getConst32Pointer() const { return reinterpret_cast<const uint32_t *>(asBits & pointerMask); }

        void releasePointer();

        union {

            uint8_t *asPointer;
            uintptr_t asBits;
        };
};

template<typename T> struct CompressedCoverage_t {

    CompressedCoverage_t(size_t sz = 0, bool full = false){ ccov = CompressedCoverage(sz, full); }

    inline const T* getData() const { return &data; }
    inline T* getData() { return &data; }
    inline void setData(const T* const data_){ data = *data_; }
    //inline void releaseData(T* const data){ delete data; }

    CompressedCoverage ccov;
    T data;
};

template<> struct CompressedCoverage_t<void> {

    CompressedCoverage_t(size_t sz = 0, bool full = false){ ccov = CompressedCoverage(sz, full); }

    inline const void* getData() const { return nullptr; }
    inline void* getData() { return nullptr; }
    inline void setData(const void* const data_){ return; }
    //inline void releaseData(void* const data){ return; }

    CompressedCoverage ccov;
};

#endif // BFG_COMPRESSED_COVERAGE_HPP
