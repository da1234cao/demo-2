#include "calculator.grpc.pb.h"
#include <grpc++/grpc++.h>
#include <string>

class CalcuServiceImpl final : public math::calculator::Service {
  grpc::Status sum(grpc::ServerContext *context, const math::request *request,
                   math::reply *reply) override {
    reply->set_result(request->addend() + request->additive_term());
    return grpc::Status::OK;
  }
};

void RunServer() {
  // 在windows会报错,不知道是不是防火墙的原因,没去细察
  // std::string server_address("0.0.0.0:5000");
  std::string server_address("127.0.0.1:5000");
  CalcuServiceImpl service;

  grpc::ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char **argv) {
  RunServer();
  return 0;
}