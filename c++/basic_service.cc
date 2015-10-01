// basic_service.cc : Implements the basic grpc roundtrip service.
// by: allison morris

#include "basic_service.h"

grpc::Status basic_service_impl::int_echo(grpc::ServerContext* context, const tiny_message* request, tiny_message* reply) {
    reply->set_data(request->data() + 1);
    return grpc::Status::OK;
}


    std::string str = 
      "01234567890abcdef01234567890abcdef01234567890abcdeffedcba98765432100"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdeffedcba98765432100"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdeffedcba98765432100"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdeffedcba98765432100"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdef01234567890abcdef"
      "01234567890abcdef01234567890abcdef01234567890abcdefacdef0123456789"
      "fedcba9876543210";

std::string str4k = str + str;
std::string str8k = str4k + str4k;
std::string str32k = str8k + str8k + str8k + str8k;
std::string str128k = str32k + str32k + str32k + str32k;

grpc::Status basic_service_impl::pull_list(grpc::ServerContext* ctx,
  const tiny_message* req, grpc::ServerWriter<bulk_message>* writer) {
    bulk_message msg;
    msg.set_data(str8k);
    for (int i = 0; i < count; ++i) {
        writer->Write(msg);
    }
    return grpc::Status::OK;
}
