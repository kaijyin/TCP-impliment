#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.


using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) ,RTO(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno-cur_ack_seqno; }

void TCPSender::fill_window() {
    //全部数据都已近发送,无数据发送
    if(_next_seqno==fin_ack_seqno){
        return ;
    }
    //syn
    if(_next_seqno==0){
      TCPSegment syn_seg;
      syn_seg.header().syn=true;
      syn_seg.header().seqno=_isn;
      _segments_out.push(syn_seg);
      _next_seqno+=syn_seg.length_in_sequence_space();
      seg_buffer.push_back({syn_seg,_next_seqno});
      return ;
    }

    uint32_t max_size=1452;
    //fill_size:可发送的字节长度
    //这里非负性判断是因为可能发送了一字节的零窗口探测,实际窗口大小没变
    uint32_t fill_size=static_cast<uint32_t>(wd_right_edge>_next_seqno?wd_right_edge-_next_seqno:0);
    string total_data=_stream.read(fill_size);
    //优先发送data,data不能填满窗口的话,才发送fin
    bool fin_flg=_stream.input_ended()&&_stream.buffer_empty()&&total_data.size()<fill_size;
    //把data按照最大字节数1452拆分为小的seg发送
    uint32_t i=0;
    while(i+max_size<total_data.size()){
        TCPSegment seg;
        seg.header().seqno=wrap(_next_seqno,_isn);
        seg.payload()=Buffer(total_data.substr(i,max_size));
        _segments_out.push(seg);
        _next_seqno+=static_cast<uint64_t>(seg.length_in_sequence_space());
        seg_buffer.push_back({seg,_next_seqno});
        i+=max_size;
    }
    //处理最后部分
    if(i<total_data.size()||fin_flg){
       TCPSegment last_seg;
       if(fin_flg){last_seg.header().fin=true;}
       last_seg.header().seqno=wrap(_next_seqno,_isn);
       if(i<total_data.size())last_seg.payload()=Buffer(total_data.substr(i));
       _segments_out.push(last_seg);
        _next_seqno+=static_cast<uint64_t>(last_seg.length_in_sequence_space());
       seg_buffer.push_back({last_seg,_next_seqno});
       if(fin_flg)fin_ack_seqno=_next_seqno;
    }
}
//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t ack=unwrap(ackno,_isn,_next_seqno);
    //错误ack
    if(ack>_next_seqno){
        return ;
    }
    //去除已近确认的seg缓存
    bool new_ack_flg=false;
    while(!seg_buffer.empty()&&seg_buffer.front().ack_index<=ack){
        new_ack_flg=true;
        cur_ack_seqno=seg_buffer.front().ack_index;
        seg_buffer.pop_front();
    }
    //如果有新的seg被确认,刷新计时器及RTO
    if(new_ack_flg){
       RTO=_initial_retransmission_timeout;
       retrans_num=0;
       timer.flash();
    }
    uint64_t now_wd_right=ack+static_cast<uint64_t>(window_size);
    if(now_wd_right>wd_right_edge){
        wd_right_edge=now_wd_right;
    }
    //window_size=0,发送一字节的窗口探测包
    if(window_size==0)wd_right_edge++;
    fill_window();
    if(window_size==0)wd_right_edge--;
 }

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    //=?
    timer.tick(ms_since_last_tick);
    if(!timer.time_out(RTO)){
       return ;
    }
    if(!seg_buffer.empty()){
    //超时重传第一个未确认的seg
    _segments_out.push(seg_buffer.front().seg);
    uint64_t wind_size=wd_right_edge-cur_ack_seqno;
    if(wind_size!=0ull){
        retrans_num++;
        RTO=RTO*2u;
    }
    timer.flash();
    }
 }

unsigned int TCPSender::consecutive_retransmissions() const { 
    return retrans_num;
 }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno=wrap(_next_seqno,_isn);
    _segments_out.push(seg);
}
