#include "router.h"
#include <glog/logging.h>

int main(int argc, char* argv[]) {

  // Assuming you launch router from /bin folder
  FLAGS_log_dir = "../logs";

  google::InitGoogleLogging(argv[0]);

  if (argc <= 2) {
    LOG(ERROR) << "You must provide serial port and addres";
    return 1;
  }

  PUT::CS::XbeeRouting::Router::run(argv[1], atoi(argv[2]));

  return 0;
}
