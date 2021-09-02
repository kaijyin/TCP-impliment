#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
        EthernetFrame frame;
        frame.header().src=_ethernet_address;
        frame.header().type=EthernetHeader::TYPE_IPv4;
        frame.payload()=dgram.serialize();
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    if(ip_address_map.count(next_hop_ip)){
        frame.header().dst=ip_address_map[next_hop_ip];
        _frames_out.push(frame);
    }else{
       unsend_frame[next_hop_ip].push_back(frame); 
       if(latest_arp_time.count(next_hop_ip)&&now_time-latest_arp_time[next_hop_ip]<5000){
           return ;
       }
       EthernetFrame arp_frame;
       arp_frame.header().src=_ethernet_address;
       arp_frame.header().dst=ETHERNET_BROADCAST;
       arp_frame.header().type=EthernetHeader::TYPE_ARP;
       ARPMessage arp;
       arp.opcode=ARPMessage::OPCODE_REQUEST;
       arp.sender_ip_address=_ip_address.ipv4_numeric();
       arp.sender_ethernet_address=_ethernet_address;
       arp.target_ethernet_address.fill(0);
       arp.target_ip_address=next_hop_ip;
       arp_frame.payload()=arp.serialize();
       _frames_out.push(arp_frame);
       latest_arp_time[next_hop_ip]=now_time;
    }
    
}
bool NetworkInterface::is_equal(const EthernetAddress& a,const EthernetAddress& b){
    for(size_t i=0;i<a.size();i++){
        if(a.at(i)!=b.at(i)){
            return false;
        }
    }
    return true;
}
//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if(!is_equal(frame.header().dst,_ethernet_address)&&!is_equal(frame.header().dst,ETHERNET_BROADCAST)){
        return {};
    }
    if(frame.header().type==EthernetHeader::TYPE_IPv4){
         InternetDatagram ipdata;
         if(ParseResult::NoError==ipdata.parse(frame.payload())){return ipdata;}
         else return {};
    }else{
        ARPMessage msg;
        if(ParseResult::NoError==msg.parse(frame.payload())){
            ip_address_map[msg.sender_ip_address]=msg.sender_ethernet_address;
            map_time[msg.sender_ip_address]=now_time;
            if(msg.opcode==ARPMessage::OPCODE_REQUEST&&msg.target_ip_address==_ip_address.ipv4_numeric()){
                EthernetFrame arp_rep;
                arp_rep.header().src=_ethernet_address;
                arp_rep.header().dst=msg.sender_ethernet_address;
                arp_rep.header().type=EthernetHeader::TYPE_ARP;
                ARPMessage arp;
                arp.sender_ip_address=_ip_address.ipv4_numeric();
                arp.sender_ethernet_address=_ethernet_address;
                arp.target_ip_address=msg.sender_ip_address;
                arp.target_ethernet_address=msg.sender_ethernet_address;
                arp.opcode=ARPMessage::OPCODE_REPLY;
                arp_rep.payload()=arp.serialize();
                _frames_out.push(arp_rep);
            }else{
                if(unsend_frame.count(msg.sender_ip_address)){
                    for(auto&x:unsend_frame[msg.sender_ip_address]){
                        x.header().dst=msg.sender_ethernet_address;
                        _frames_out.push(x);
                    }
                    unsend_frame.erase(msg.sender_ip_address);
                    latest_arp_time.erase(msg.sender_ip_address);
                }
            }
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 
    now_time+=ms_since_last_tick;
    for(auto&x:map_time){
        if(now_time-x.second>=30000){
            map_time.erase(x.first);
            ip_address_map.erase(x.first);
        }
    }
 }