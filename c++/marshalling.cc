// marshalling.cc : tests packing and unpacking various data types.
// by: allison morris

#include "grpc.pb.h"
#include "clocker.h"
#include "measure.h"

// data object. contains message and data to pack in it.
struct message_data {
    big_message msg;
    str_msg smsg;
    complex_message cdata;
    int idata;
    float fdata;
    double ddata;
    long ldata;
    bool bdata;
    std::string sdata;
    //big_message::little ldata;
};

// predicate functions. these are executed by the runner.

struct pack_int32 {
    void run(message_data& data) {
        data.msg.clear_idata();
        data.msg.set_idata(data.idata);
    }
};

struct pack_float {
    void run(message_data& data) {
        data.msg.clear_fdata();
        data.msg.set_fdata(data.fdata);
    }
};

struct pack_double {
    void run(message_data& data) {
        data.msg.clear_ddata();
        data.msg.set_ddata(data.ddata);
    }
};

struct pack_string {
    void run(message_data& data) {
        data.smsg.clear_sdata();
        data.smsg.set_sdata(data.sdata);
    }
};

/*struct pack_complex {
    void run( message_data& data) {
        data.cdata.Clear();
        *data.msg.mutable_cdata() = data.cdata;
    }
};*/

struct pack_complex {
    void run(message_data& data) {
        data.cdata.Clear();
        data.cdata.mutable_top()->mutable_wrap()->set_ldata(data.ldata); 
        data.cdata.mutable_top()->mutable_wrap()->set_bdata(data.bdata);
        data.cdata.mutable_bot()->mutable_wrap()->set_idata(data.idata);
        data.cdata.mutable_bot()->mutable_wrap()->set_bdata(data.bdata); 
    }
};

/*struct pack_little {
    void run(message_data& data) {
        data.msg.clear_ldata();
        *data.msg.mutable_ldata() = (data.ldata);
    }
};

struct pack_little_full {
    void run(message_data& data) {
        data.msg.clear_ldata();
        data.msg.mutable_ldata()->set_first(data.idata);
        data.msg.mutable_ldata()->set_second(data.idata);
        data.msg.mutable_ldata()->set_third(data.sdata);
        *data.msg.mutable_ldata() = (data.ldata);
    }
};*/

std::string operator*(std::string str, int times) {
    std::string ret = str;
    for (int i = 0; i < times - 1; ++i) {
        for (int j = 0; j < str.length(); ++j) {
            ret += (char)(((ret[ret.length() - str.length()] - 32) + 7) % 95) + 32;
        }    
    //ret += str;
    }
    return ret;
}

int main(int argc, char** argv) {
    message_data data;
    clocker::mode mode = clocker::clock_gettime;
    int count = 1;
    
    // basic measurements.
    data.idata = 17;
    data.fdata = 135.68;
    data.ddata = 2125.1234;
    data.ldata = 123456789098;
    data.bdata = true;
    measure("packing int 17", pack_int32(), data, mode, count);
    measure("packing float 135.68", pack_float(), data, mode, count);
    measure("packing double 2125.1234", pack_double(), data, mode, count);
    
    // string measurements.
    data.sdata = " ";
    measure("packing 1 char string", pack_string(), data, mode, count);
    //data.sdata = "01234567";
    data.sdata = data.sdata * 10;
    measure("packing 10 char string", pack_string(), data, mode, count);
    //data.sdata = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    data.sdata = data.sdata * 10;
    measure("packing 100 char string", pack_string(), data, mode, count);
    data.sdata = data.sdata * 10;
    measure("packing 1000 char string", pack_string(), data, mode, count);
    data.sdata = data.sdata * 10;
    measure("packing 10000 char string", pack_string(), data, mode, count);
    data.sdata = data.sdata * 10;
    measure("packing 100000 char string", pack_string(), data, mode, count);
    // full inner measurements.
 /*   data.sdata = "K";
    measure("packing inner with 1 char string from raw", pack_little_full(), data, mode, count);
    data.sdata = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    measure("packing inner with 64 char string from raw", pack_little_full(), data, mode, count);
    data.sdata += data.sdata;
    data.sdata += data.sdata;
    measure("packing inner with 256 char string from raw", pack_little_full(), data, mode, count);
    
    // simple inner measurements.
    data.msg.mutable_ldata()->set_third("K");
    measure("packing inner with 1 char string from packed", pack_little(), data, mode, count);
    data.msg.mutable_ldata()->set_third("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    measure("packing inner with 64 char string from packed", pack_little(), data, mode, count);
    data.msg.mutable_ldata()->set_third(data.sdata);
    measure("packing inner with 256 char string from packed", pack_little(), data, mode, count);
 */  
    // complex measurements.
    measure("complex straight-up", pack_complex(), data, mode, count); 
    return 0;
}
