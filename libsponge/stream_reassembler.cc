#include "stream_reassembler.hh"
#include <limits>
#include <algorithm>
#include <vector>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) 
    : _output(capacity), 
    _capacity(capacity),
    _next_index(0),
    _eof_index(std::numeric_limits<size_t>::max()),
    _eof_flag(false),
    _buffer(capacity),
    _unassembled_bytes(0) {}


//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {

    // 1. Trim data outside window
    // Ex: if _next_index = 5, _capacity = 10, data = "BCDEFGHI", index = 1, output.size = 5,
    // stream_end = 5 + 10 - 5 = 10, start = max(1, 5) = 5, end = min(1 + 8, 10, 10) = 9
    // accept range = [5, 10) -> "BCDEFGHI" -> "FGHI"
    size_t stream_end = _next_index + _capacity - _output.buffer_size();
    size_t start = std::max(index, _next_index); 
    size_t end = std::min(index + data.size(), stream_end); 

    if (eof) {
        // remember first eof
        _eof_flag = true;
        _eof_index = std::min(index + data.size(), _eof_index);
    }

    // Ex: mapping to buffer, push_substring("BCDEFGHI", 0, false)
    // | logical idex | Byte | Buffer idx (%10) | Stored | Bool |
    // | 1            | B    | x (i < start)    | -      | -    |
    // | ...          | .    | .                | .      | .    |
    // | 5            | F    | 5                | F      | 1    |
    // | 6            | G    | 6                | G      | 1    |
    // | 7            | H    | 7                | H      | 1    |
    // | 8            | I    | 8                | -      | -    |

    // 2. Copy bytes to buffer
    // start = 5, end = 9, index = 1
    for (size_t i = start; i < end; ++i) {
        // check if byte already in buffer
        if (_buffer[i % _capacity].second) continue;
        _buffer[i % _capacity] = {data[i - index], true};
        _unassembled_bytes++;
    }

    // 3. Write contiguous bytes to output stream
    // next_index = 5, write "FGHI")
    string assembled;
    while (_next_index < _eof_index && _buffer[_next_index % _capacity].second) {
        assembled += _buffer[_next_index % _capacity].first;
        _buffer[_next_index % _capacity] = {'\0', false}; // clear buffer
        _unassembled_bytes--;
        _next_index++;
    }
    _output.write(assembled); // write to output stream

    // 4. Check if read EOF
    if (_next_index == _eof_index) {
        _eof_flag = true;
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }
