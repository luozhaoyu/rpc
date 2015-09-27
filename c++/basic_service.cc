// basic_service.cc : Implements the basic grpc roundtrip service.
// by: allison morris

#include "basic_service.h"

grpc::Status basic_service_impl::int_echo(grpc::ServerContext* context, const tiny_message* request, tiny_message* reply) {
    reply->set_data(request->data() + 1);
    return grpc::Status::OK;
}
