#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;
//发送rst包不需要重传
void TCPConnection::bad_shutdown(bool send_rst) {
     close=true;
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    if(send_rst){
     TCPSegment seg;
     seg.header().rst=true;
     seg.header().seqno=_sender.next_seqno();
    _segments_out.push(seg);
    }
}
void TCPConnection::check_close(){
    if(stream_both_eof()){
       if(!_linger_after_streams_finish||now_time-last_recive_time>=10*_cfg.rt_timeout){
         close=true;
       }
    }
}
bool TCPConnection::stream_both_eof(){
    return _receiver.stream_out().eof()&&_sender.stream_in().eof()&&_sender.bytes_in_flight()==0ull;
}
void TCPConnection::set_window_info(){
    _sender.reset_host_window(_receiver.ackno(),_receiver.window_size());
}
void TCPConnection::clean(){
    queue<TCPSegment>&buffer=_sender.segments_out();
    while(!buffer.empty()){
        TCPSegment seg=buffer.front();
        _segments_out.push(seg);
        buffer.pop();
    }
    check_close();
}
size_t TCPConnection::remaining_outbound_capacity() const {
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const {
    return _sender.bytes_in_flight();
 }

size_t TCPConnection::unassembled_bytes() const { 
    return _receiver.unassembled_bytes();
 }

size_t TCPConnection::time_since_last_segment_received() const {
    return now_time-last_recive_time;
 }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    if(seg.header().rst){
        bad_shutdown(false);
        return ;
    }
    _receiver.segment_received(seg);
    //如果己方还有需要发送的数据没有发送就收到了对方的fin,收到自己的fin后就不需要等待
    if(_receiver.stream_out().eof()&&!_sender.stream_in().eof()){
        _linger_after_streams_finish=false;
    }

    //reciver处理结束,sender处理
    //连接成功
    if(seg.header().syn){
        _sender.connect();
    }
    set_window_info();
    if(seg.header().ack){
        _sender.ack_received(seg.header().ackno,seg.header().win);
    }
    //确认ack
    if(seg.length_in_sequence_space()>0u){
         _sender.send_empty_ack();
         clean();
    }
    last_recive_time=now_time;
 }

bool TCPConnection::active() const { 
     return !close;
}

size_t TCPConnection::write(const string &data) {
    size_t size=_sender.stream_in().write(data);
    set_window_info();
    _sender.fill_window();
    clean();
    return size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    now_time+=ms_since_last_tick;
    if(_sender.consecutive_retransmissions()>=8){
        bad_shutdown(true);
        return ;
    }
    _sender.tick(ms_since_last_tick);
    clean();
}

void TCPConnection::end_input_stream() {;
    _sender.stream_in().end_input();
    //发送fin
    _sender.fill_window();
    clean();
}

void TCPConnection::connect() {
    _sender.fill_window();
    clean();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
           bad_shutdown(true);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
