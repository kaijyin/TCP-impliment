#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.


const uint64_t max_int=4294967296;
using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if(seg.header().syn){
        recieve_isn=true;
        isn=seg.header().seqno;
    }
    if(!recieve_isn){
        return ;
    }
  const uint64_t check_point=static_cast<uint64_t>(_reassembler.stream_out().bytes_written());
  const uint64_t win_left=check_point;
  const uint64_t win_right=static_cast<uint64_t>(_capacity-_reassembler.stream_out().buffer_size())+win_left;
  const uint64_t seg_start=unwrap(WrappingInt32(seg.header().seqno),isn,check_point);
    if(seg_start>win_right){
        return ;
    }
    _reassembler.push_substring(string(seg.payload().str().data()),seg_start,seg.header().fin);

}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(recieve_isn){
     return wrap(static_cast<uint64_t>(_reassembler.stream_out().bytes_written()),isn);
    }
    return {};
 }

size_t TCPReceiver::window_size() const { 
    
    if(_capacity-_reassembler.stream_out().buffer_size()>1u){
        return _capacity-_reassembler.stream_out().buffer_size();
    }else{
        return 1u;
    }
 }
