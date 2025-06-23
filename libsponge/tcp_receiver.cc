#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {

    // Set the initial sequence number if SYN is set
    if (!_isn.has_value()) {
        if (!seg.header().syn) return;  // ignore segments without SYN
        _isn = seg.header().seqno;  // set the initial sequence number
    }

    // push data / eof to StreamReassembler

    // seqNo --(unwrap)--> absSeqNo --(-1)--> streamIndex
    //   unwrap: map 32 bit seqNo to 64 bit number near checkpoint
    //   checkpoint: the spot in the data stream that we ’ve already “passed"
    //     => the last byte that we have received and processed
    //     => _reassembler.stream_out().bytes_written()
    uint64_t abs_ackno = _reassembler.stream_out().bytes_written();
    uint64_t abs_seq_no = unwrap(seg.header().seqno, _isn.value(), abs_ackno);

    uint64_t stream_index = abs_seq_no - 1 + (seg.header().syn? 1 : 0);  // zero-indexed, adjust for SYN byte
    _reassembler.push_substring(seg.payload().copy(), stream_index, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (!_isn.has_value()) return {};
    uint64_t abs_ackno = _reassembler.stream_out().bytes_written() + 1; 
    // eof: seen FIN & drained all data from buffer
    // input_ended: seen FIN 
    bool ended = _reassembler.stream_out().input_ended();
    if (ended) abs_ackno++; // add 1 for FIN byte
    return wrap(abs_ackno, _isn.value());
}

size_t TCPReceiver::window_size() const { 
    // capacity - size of the reassembled stream
    return _capacity - _reassembler.stream_out().buffer_size(); }
