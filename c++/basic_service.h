// basic_service.h : Implements the basic grpc roundtrip service.
// by: allison morris

#ifndef BASIC_SERVICE_H
#define BASIC_SERVICE_H

#include <grpc++/grpc++.h>
#include "grpc.pb.h"
#include "grpc.grpc.pb.h"

class basic_service_impl : public basic_service::Service {
public:
    grpc::Status int_echo(grpc::ServerContext* context, const tiny_message* request, tiny_message* reply);

};

#endif
