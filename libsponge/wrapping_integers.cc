#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    // n = (isn + n_abs ) (% 2^32)
    return WrappingInt32{static_cast<uint32_t>(n) + isn.raw_value()};
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

    // n = (isn + n_abs ) (% 2^32)
    // n_abs = (n - isn) (% 2^32)
    // auto low32 = static_cast<uint32_t>(n - isn);

    // auto high32_upper = (checkpoint + (1 << 31)) & 0xFFFFFFFF00000000; 
    // auto high32_lower = (checkpoint - (1 << 31)) & 0xFFFFFFFF00000000;

    // auto candidate1 = high32_upper + low32;
    // auto candidate2 = high32_lower + low32;

    // auto d1 = candidate1 > checkpoint ? candidate1 - checkpoint : checkpoint - candidate1;
    // auto d2 = candidate2 > checkpoint ? candidate2 - checkpoint : checkpoint - candidate2;

    // return d1 < d2 ? candidate1 : candidate2;



    WrappingInt32 cp = wrap(checkpoint, isn);
    int32_t offset = n - cp;
    printf("n: %u, isn: %u, checkpoint: %lu, offset: %d\n", n.raw_value(), isn.raw_value(), checkpoint, offset);
    return checkpoint + offset;
}
