#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;
//发送rst包不需要重传
void TCPConnection::send_rst() {
     TCPSegment seg;
     seg.header().rst=true;
     seg.header().seqno=_sender.next_seqno();
    _segments_out.push(seg);
}
void TCPConnection::set_window_info(){
    _sender.reset_host_window(_receiver.ackno(),INT16_MAX<_receiver.window_size()?INT16_MAX:_receiver.window_size());
}
void TCPConnection::send(){
    queue<TCPSegment>&buffer=_sender.segments_out();
    while(!buffer.empty()){
        TCPSegment seg=buffer.front();
        // cout<<"commit seg segseqno:"<<seg.header().seqno<<" ack:"<<(seg.header().ack)
        // <<" syn:"<<(seg.header().syn)<<endl;
        _segments_out.push(seg);
        buffer.pop();
    }
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
        close=true;
        _receiver.stream_out().set_error();
        _sender.stream_in().set_error();
    }
    //   cout<<"recive_seg segseqno:"<<seg.header().seqno<<" ack:"<<(seg.header().ack)
    //     <<" syn:"<<(seg.header().syn)<<endl;
    if(close)return;
//     cout<<"id:"<<id;
//     cout<<"recive begin outeof:"<<(_receiver.stream_out().eof())
//         <<" ineof:"<<(_sender.stream_in().eof())
//    <<" byteinflight:"<<_sender.bytes_in_flight()<<endl;;


//    cout<<"recive seg segseqno:"<<
//         seg.header().seqno<<" ack:"<<(seg.header().ack)
//         <<" syn:"<<(seg.header().syn)
//         <<" fin:"<<(seg.header().fin)
//         <<" ackseqno:"<<seg.header().ackno<<endl;
    _receiver.segment_received(seg);
    if(_receiver.stream_out().eof()&&!_sender.stream_in().eof()){
        _linger_after_streams_finish=false;
    }
    //连接成功
    if(seg.header().syn){
        _sender.connect();
    }
    //设置win和ackno
    set_window_info();
    if(seg.header().ack){
        _sender.ack_received(seg.header().ackno,seg.header().win);
    }
    //确认ack
    if(seg.length_in_sequence_space()>0u){
        // cout<<"send_ack"<<endl;
         _sender.send_empty_segment();
         send();
    }
    // cout<<"id:"<<id;
    // cout<<"recive end outeof:"<<(_receiver.stream_out().eof())
    //     <<" ineof:"<<(_sender.stream_in().eof())
    // <<" byteinflight:"<<_sender.bytes_in_flight()<<endl;;
    last_recive_time=now_time;
 }

bool TCPConnection::active() const { 
     return !close;
}

size_t TCPConnection::write(const string &data) {
    size_t size=_sender.stream_in().write(data);
    set_window_info();
    _sender.fill_window();
    send();
    return size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    now_time+=ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    send();
    if(_sender.consecutive_retransmissions()>8){
        // cout<<"retrans to manny times badclose"<<endl;
        send_rst();
        close=true;
        _receiver.stream_out().error();
        _sender.stream_in().error();
    }
    // cout<<"outeof:"<<(_receiver.stream_out().eof())
    // <<" ineof:"<<(_sender.stream_in().eof())<<endl;;
    if(_receiver.stream_out().eof()&&_sender.stream_in().eof()&&_sender.bytes_in_flight()==0ull){
        // cout<<"both eof"<<endl;
      if(!_linger_after_streams_finish||now_time-last_recive_time>=10*_cfg.rt_timeout){
        //  cout<<"timeout close"
        //  <<"linerfinish:"<<(_linger_after_streams_finish)<<endl;
         close=true;
       }
    }
}

void TCPConnection::end_input_stream() {
    // cout<<"id:"<<id;
    // cout<<"endinput begin outeof:"<<(_receiver.stream_out().eof())
    //    <<" ineof:"<<(_sender.stream_in().eof())
//    <<" byteinflight:"<<_sender.bytes_in_flight()<<endl;;
    _sender.stream_in().end_input();
    //发送fin
    _sender.fill_window();
    send();
//     cout<<"id:"<<id;
//      cout<<"endinput end outeof:"<<(_receiver.stream_out().eof())
//     <<" ineof:"<<(_sender.stream_in().eof())
//    <<" byteinflight:"<<_sender.bytes_in_flight()<<endl;;
}

void TCPConnection::connect() {
    _sender.fill_window();
    send();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
           send_rst();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
