#include <glog/logging.h>
#include <gflags/gflags.h>

void SetupLog(const std::string& name) {
    std::string program_name = "dit";
    if (!name.empty()) {
        program_name = name;
    }
    if (FLAGS_log_dir.empty()){
        FLAGS_log_dir = ".";
    }
    std::string log_filename = FLAGS_log_dir + "/" + program_name + ".INFO.";
    std::string wf_filename = FLAGS_log_dir + "/" + program_name + ".WARNING.";
    std::string ev_filename = FLAGS_log_dir + "/" + program_name + ".EVENT.";
    google::SetLogDestination(google::INFO, log_filename.c_str());
    google::SetLogDestination(google::WARNING, wf_filename.c_str());
    google::SetLogDestination(google::ERROR, ev_filename.c_str());
    google::SetLogDestination(google::FATAL, "");

    google::SetLogSymlink(google::INFO, program_name.c_str());
    google::SetLogSymlink(google::WARNING, program_name.c_str());
    google::SetLogSymlink(google::ERROR,  program_name.c_str());
    google::SetLogSymlink(google::FATAL, "");
    google::InstallFailureSignalHandler();
}

int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    SetupLog("ditd");
    LOG(INFO) << "ditd start...";

    return 0;
}
