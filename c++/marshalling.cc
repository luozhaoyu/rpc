// marshalling.cc : tests packing and unpacking various data types.
// by: allison morris

#include "clocker.h"
#include "measure.h"

// data object. contains message and data to pack in it.
struct message_data {
    big_message msg;
    int idata;
    float fdata;
    double ddata;
    std::string sdata;
    big_message::little ldata;
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
        data.msg.clear_sdata();
        data.msg.set_sdata(data.sdata);
    }
};

struct pack_little {
    void run(message_data& data) {
        data.msg.clear_ldata();
        data.msg.set_ldata(data.ldata);
    }
};

struct pack_little_full {
    void run(message_data& data) {
        data.msg.clear_ldata();
        data.msg.mutable_ldata()->set_first(data.idata);
        data.msg.mutable_ldata()->set_second(data.idata);
        data.msg.mutable_ldata()->set_third(data.sdata);
        data.msg.set_ldata(data.ldata);
    }
};

int main(int argc, char** argv) {
    message_data data;
    clocker::mode mode = clocker::beta_clock;
    int count = 100;
    
    // basic measurements.
    data.idata = 17;
    data.fdata = 135.68;
    data.ddata = 2125.1234;
    measure("packing int 17", pack_int32(), data, mode, count);
    measure("packing float 135.68", pack_float(), data, mode, count);
    measure("packing double 2125.1234", pack_double(), data, mode, count);
    
    // string measurements.
    data.sdata = "K";
    measure("packing 1 char string", pack_string(), data, mode, count);
    data.sdata = "01234567";
    measure("packing 8 char string", pack_string(), data, mode, count);
    data.sdata = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    measure("packing 64 char string", pack_string(), data, mode, count);
    data.sdata += data.sdata;
    data.sdata += data.sdata;
    measure("packing 256 char string", pack_string(), data, mode, count);
    
    // full inner measurements.
    data.sdata = "K";
    measure("packing inner with 1 char string from raw", pack_little_full(), data, mode, count);
    data.sdata = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    measure("packing inner with 64 char string from raw", pack_little_full(), data, mode, count);
    data.sdata += data.sdata;
    data.sdata += data.sdata;
    measure("packing inner with 256 char string from raw", pack_little_full(), data, mode, count);
    
    // simple inner measurements.
    data.msg.mutable_ldata->set_third("K");
    measure("packing inner with 1 char string from packed", pack_little(), data, mode, count);
    data.msg.mutable_ldata->set_third("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    measure("packing inner with 64 char string from packed", pack_little(), data, mode, count);
    data.msg.mutable_ldata->set_third(data.sdata);
    measure("packing inner with 256 char string from packed", pack_little(), data, mode, count);
    
    return 0;
}