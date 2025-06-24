#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { 
    return _bytes_in_flight; 
}

void TCPSender::fill_window() {
    uint16_t window = _window_size == 0 ? 1 : _window_size;

    // Continue to send segment if receiver allow / input not empty
    while (window > _bytes_in_flight && _stream.buffer_size() > 0) {
        // send SYN seg first
        TCPSegment seg;
        if (!_syn_sent) {
            seg.header().syn = true;
            seg.header().seqno = _isn;
        }

        // Prepare seg: fill seqno, SYN/FIN, payload
        size_t to_read = min(window - _bytes_in_flight, TCPConfig::MAX_PAYLOAD_SIZE);
        auto data = _stream.read(to_read);
        seg.payload() = std::move(data);

        // if encounter EOF, set FIN
        if (!_fin_sent && _stream.eof() 
        && seg.length_in_sequence_space() < (window - _bytes_in_flight)) {
            // to fit FIN, the receiver should have at least payload_len + 1 space
            _fin_sent = true;
            seg.header().fin = true;
        }
        // Send the whole TCP seg
        seg.header().seqno = next_seqno(); // wrap
        _segments_out.push(seg);

        // Record outstanding seg for retrans
        _outstanding_seg.emplace_back(_next_seqno, seg);

        // Update seqno & bytes sent 
        size_t len = seg.length_in_sequence_space();
        _next_seqno += len;
        _bytes_in_flight += len;

        // Start the retrans timer if there's unACKed seg
        if (!_timer_running) {
            _timer_running = true;
            _time_elapsed = 0;
        }
    }
    
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    // Validate ackno
    uint64_t ack_abs = unwrap(ackno, _isn, next_seqno_absolute());
    if (ack_abs < next_seqno_absolute()) { return; } // Ignore duplicate ACKs

    // Update window size
    _window_size = window_size;

    // Remove ACKed segments from outstanding segs
    bool forward = 0;
    while (!_outstanding_seg.empty()) {
        auto &[seq_abs, seg] = _outstanding_seg.front();
        uint64_t seg_end = seq_abs + seg.length_in_sequence_space();
        if (ack_abs >= seg_end) {
            // ACKed the oldest segment, remove it
            _bytes_in_flight -= seg.length_in_sequence_space();
            _outstanding_seg.pop_front();
            forward = true;
        } else {
            // ACKed a segment that is not the oldest, stop
            break;
        }
    }

    // If we have acknowledged, reset timer
    if (forward) {
        _time_elapsed = 0;
        _consecutive_retrans = 0;
        _retrans_timeout = _initial_retransmission_timeout;
    } 

    // If all outstanding data has been ACKed, stop the timer
    if (_outstanding_seg.empty()) {
        _timer_running = false;
    }

    // If there are new space, fill the window
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { DUMMY_CODE(ms_since_last_tick); }

unsigned int TCPSender::consecutive_retransmissions() const { return {}; }

void TCPSender::send_empty_segment() {
    // Create empty segment & send
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}
