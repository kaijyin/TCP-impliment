#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

const uint64_t max_int=4294967296;
using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    WrappingInt32 res(static_cast<uint32_t>((static_cast<uint64_t>(isn.raw_value())+n)%max_int));
    return res;
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
 
    //坑点,如果求得checkpoint就是最近的怎么办
    uint64_t mid = (max_int - static_cast<uint64_t>(isn.raw_value()) + static_cast<uint64_t>(n.raw_value())) % max_int;
    mid += (checkpoint / max_int) * max_int;
    uint64_t right = mid + max_int;
    uint64_t left = mid - max_int;
    if (mid > checkpoint) {
        if (mid - checkpoint < checkpoint - left||mid<max_int) {
            return mid;
        }
        else {
            return left;
        }
    }
    else {
        if (right - checkpoint < checkpoint - mid) {
            return right;
        }
        else {
            return mid;
        }
    }
}
