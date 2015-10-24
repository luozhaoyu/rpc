// sup.h : simple udp-based protocol.
// by: allison morris

// note: this is for linux only.

#include <netinet/ip.h>

// class that wraps protocol functionality.
class sup {
public:
    enum type { MSG, ACK };
    
    // class to represent packet header.
    class header {
    public:
        header(unsigned int n, type t) : _number(n), _flags(0) {
            if (t == ACK) { _flags = 0x80000000; }
        }
        
        unsigned int data_size() const { return _flags & 0x7fffffff; }
        
        void data_size(unsigned int s) {
            _flags &= 0x80000000;
            s &= 0x7fffffff;
            _flags |= s;
        }
        
        bool is_ack() const { return (_flags & 0x80000000) != 0; }
        bool is_msg() const { return (_flags & 0x80000000) == 0; }
    private:
        unsigned int _number;
        unsigned int _flags;
    };
    
    static const unsigned int max_packet_size = 512;
    static const unsigned int max_data_length = max_packet_size - sizeof(header);
    
    // class to represent full packet.
    class packet {
    public:
        packet(unsigned int sn, type t) : _info(sn, t) { }
        
        unsigned int data_size() const { return _info.data_size(); }
        
        char* data() { return &_data[0]; }
    private:
        header _info;
        char _data[max_data_length];
    };
    
    // handles client side.
    class client {
    public:
        client(unsigned int addr, unsigned int port);
        
        void create_msg();
        
        char* get_data() { return _buf.data(); }
        
        void send_msg();
        
    private:
        unsigned int _addr;
        unsigned int _port;
        unsigned int _seqnum;
        int sock;
        
        packet _buf;
    };
    
    // handles server side.
    class server {
    public:
        server(unsigned int addr, unsigned int port, unsigned int rate);
        
        void listen();
        
    private:
        unsigned int _addr;
        unsigned int _port;
        unsigned int _rate;
        int sock;
        
        packet _buf;
    };
private:
};
