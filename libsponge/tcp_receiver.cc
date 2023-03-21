#include "tcp_receiver.hh"
#include <iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    DUMMY_CODE(seg);
    bool bad_byte = false;
    TCPHeader header = seg.header();
    if (header.syn) {
        _isn = header.seqno;
        _set = true;
    }
    
    if (_set) {
        bad_byte = !header.syn && (header.seqno == _isn);
    }

    uint64_t abs_seqno = unwrap(header.seqno, _isn, _reassembler.last_reassembled()); 
    uint64_t stream_idx = abs_seqno == 0 ? 0 : abs_seqno - 1;
    if (!bad_byte && _set) {
        cout << "Payload" << string(seg.payload()) << endl;
        _reassembler.push_substring(string(seg.payload()), stream_idx, header.fin);
        return;
    }
    _reassembler.push_substring("", stream_idx, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_set) {
        uint64_t abs_expected = _reassembler.expected() + 1;
        if (stream_out().input_ended()) {
            return wrap(abs_expected + 1, _isn);
        }
        return wrap(abs_expected, _isn);
    }
    return {};
}

size_t TCPReceiver::window_size() const {
    return _capacity - stream_out().buffer_size(); 
}

