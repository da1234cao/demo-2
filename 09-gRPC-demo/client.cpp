#include "calculator.grpc.pb.h"
#include <grpc++/grpc++.h>
#include <string>

class calcuClient {
public:
  calcuClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(math::calculator::NewStub(channel)) {}
  int sum(const int a, const int b) {
    math::request req;
    req.set_addend(a);
    req.set_additive_term(b);
    math::reply reply;
    grpc::ClientContext context;

    grpc::Status status = stub_->sum(&context, req, &reply);

    if (status.ok()) {
      return reply.result();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
  }

private:
  std::unique_ptr<math::calculator::Stub> stub_;
};

int main(int argc, char **argv) {
  std::string server_address("0.0.0.0:5000");
  calcuClient calcu(
      grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
  int result = calcu.sum(1, 2);
  std::cout << "calcu received: " << result << std::endl;

  return 0;
}