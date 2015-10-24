// basic_service.h : Implements the basic grpc roundtrip service.
// by: allison morris

#ifndef BASIC_SERVICE_H
#define BASIC_SERVICE_H

#include <grpc++/grpc++.h>
#include "grpc.pb.h"
#include "grpc.grpc.pb.h"

class basic_service_impl : public basic_service::Service {
public:
    static const int count = 100000;

    grpc::Status int_echo(grpc::ServerContext* context, const tiny_message* request, tiny_message* reply);

    grpc::Status pull_list(grpc::ServerContext* context, const tiny_message*
      request, grpc::ServerWriter<bulk_message>* writer);

};

#endif
