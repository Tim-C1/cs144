#include <iostream>
#include <assert.h>
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
    , _current_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const {
    return _on_flight;
}

void TCPSender::fill_window() {
    size_t remain = 0;
    uint16_t win_size_trick = _window_size == 0 ? 1 : _window_size;
    if (_window_size == 0) {
        remain = 1;
    } else {
        if (_window_size <= _fill_window_size) {
            remain = 0;
        } else {
            remain = _window_size - _fill_window_size;
        }
    }

    if (!_has_sent) {
        TCPSegment s;
        s.header().syn = true;
        send_tcp_segment(s);
        _has_sent = true;
        _fill_window_size++;
        return;
    }

    if (_stream.eof() && _is_eof) {
        return;
    }

    while (!_stream.buffer_empty() && remain > 0 && (_next_seqno - _abs_ackno_received < win_size_trick)) {
        int16_t payload_size = 0;
        if (remain >= TCPConfig::MAX_PAYLOAD_SIZE) {
            payload_size = TCPConfig::MAX_PAYLOAD_SIZE;
        } else {
            payload_size = remain;
        }

        // construct the segment
        
        TCPSegment s;
        s.payload() = _stream.read(payload_size);
        
        if (_stream.eof() && remain >= s.payload().size() + 1) {
            s.header().fin = true;
            _is_eof = true;
        }
        send_tcp_segment(s);
        remain -= s.length_in_sequence_space();
        _fill_window_size += s.length_in_sequence_space();
    }
    
    if (_stream.eof() && !_is_eof && remain > (_next_seqno - _abs_ackno_received)) {
        _is_eof = true;
        TCPSegment s;
        s.header().fin = true;
        send_tcp_segment(s);
        _fill_window_size++;
        return;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);
    // impossible ack, just ignore 
    if (abs_ackno > _next_seqno) {
        return;
    }
    _window_size = window_size;
    _fill_window_size = 0;

    // update the maximum ackno received
    bool rec_new = false;
    if (abs_ackno > _abs_ackno_received) {
        _abs_ackno_received = abs_ackno;
        // move acked segments
        for(auto it = _segments_outstanding.begin(); it != _segments_outstanding.end();) {
            auto &segs_no = *it;
            if (segs_no.first + segs_no.second.length_in_sequence_space() <= abs_ackno) {
                rec_new = true;
                _on_flight -= segs_no.second.length_in_sequence_space();
                it = _segments_outstanding.erase(it);
            } else {
                it++;
            }
        }

        // set rto back to initial
        _current_retransmission_timeout = _initial_retransmission_timeout;
        if (rec_new) {
            _consecutive_retransmissions = 0;
        }
        _timer.stop();
        if (!_segments_outstanding.empty()) {
            _timer.start(_current_retransmission_timeout);
        }
    }

    if (rec_new) {
        fill_window();
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    bool _has_resent = false;
    _timer.advance(ms_since_last_tick);
    if (_timer.has_expire()) {
        // restart the timer if expire 
        _timer.start(_current_retransmission_timeout);
        //retransmit the earliest segment
        if (!_segments_outstanding.empty()) {
            _segments_out.push(_segments_outstanding.front().second);
            _has_resent = true;
        }
        if (_window_size > 0) {
            if (_has_resent) {
                _consecutive_retransmissions++;
            }
            _current_retransmission_timeout *= 2;
            _timer.stop();
            _timer.start(_current_retransmission_timeout);
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment s;
    s.header().seqno = next_seqno();
    _segments_out.push(s);
}

void TCPSender::send_tcp_segment(TCPSegment s) {
    s.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(s);
    _segments_outstanding.push_back(std::make_pair(_next_seqno, s));
    _next_seqno += s.length_in_sequence_space();
    _on_flight += s.length_in_sequence_space();
    if (!_timer.is_start()) {
        _timer.start(_current_retransmission_timeout);
    }
}
