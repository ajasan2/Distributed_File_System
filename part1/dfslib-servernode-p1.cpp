#include <map>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <getopt.h>
#include <dirent.h>
#include <sys/stat.h>
#include <grpcpp/grpcpp.h>


using grpc::Status;
using grpc::Server;
using grpc::StatusCode;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::ServerContext;
using grpc::ServerBuilder;


class DFSServiceImpl final : public DFSService::Service {

public:

    DFSServiceImpl(const std::string &mount_path): mount_path(mount_path) {}

    ~DFSServiceImpl() {}

    Status UploadFile(ServerContext* context, ServerReader<FileContext>* reader, FileContext* response) override {
        if (context->IsCancelled()){
            dfs_log(LL_ERROR) << "Deadline expired";
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline expired");   
        }

        FileContext file_metadata;
        if (!reader->Read(&file_metadata)) {
            dfs_log(LL_ERROR) << "Metadata not received";
            return Status(StatusCode::INVALID_ARGUMENT, "Metadata not received");
        }

        string full_path = WrapPath(file_metadata.metadata().name()); 
        dfs_log(LL_SYSINFO) << "Storing file " << full_path << " of size " << file_metadata.metadata().size();

        ofstream ofs(full_path, ios::binary);
        if (!ofs.is_open()) {
            dfs_log(LL_ERROR) << "Failed to open file for writing: " << full_path;
            return Status(StatusCode::INTERNAL, "Failed to open file for writing");
        }

        // Redacted file storage

        ofs.close();
        return Status::OK;
    }

    Status DownloadFile(ServerContext* context, const FileContext* request, ServerWriter<FileContext>* writer) override {
        if (context->IsCancelled()){
            dfs_log(LL_ERROR) << "Deadline expired";
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline expired");
        }

        const string& full_path = WrapPath(request->metadata().name());
        struct stat file_stats;
        if (stat(full_path.c_str(), &file_stats) != 0){
            dfs_log(LL_ERROR) << "File does not exist:  " << full_path;
            return Status(StatusCode::NOT_FOUND, "Failed to open file");
        }

        dfs_log(LL_SYSINFO) << "Sending file " << full_path << " of size " << file_stats.st_size;

        // Redacted sending file chunks

        return Status::OK;
    }

    Status RemoveFile(ServerContext* context, const FileContext* request, Blank* response) override {
        if (context->IsCancelled()){
            dfs_log(LL_ERROR) << "Deadline expired";
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline expired");   
        }
        if (!request->has_metadata()) {
            dfs_log(LL_ERROR) << "Missing request metadata";
            return Status(StatusCode::INVALID_ARGUMENT, "Missing request metadata");
        }

        const string& filename = request->metadata().name();
        string full_path = WrapPath(filename);

        // Redacted file removal

        return Status::OK;
    }

    Status ListFiles(ServerContext* context, const Blank* request, FileCatalog* response) override {
        DIR *dir = opendir(mount_path.c_str());
        if (!dir) {
            dfs_log(LL_ERROR) << "Failed to open directory " << mount_path;
            return Status(StatusCode::INTERNAL, "Failed to open directory " + mount_path);
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            // Redacted file iteration
        }
        
        closedir(dir);
        return Status::OK;
    }

    Status GetFileStatus(ServerContext* context, const FileContext* request, FileContext* response) override {
        if (context->IsCancelled()) {
            dfs_log(LL_ERROR) << "Deadline expired";
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline expired");   
        }
        if (!request->has_metadata()) {
            dfs_log(LL_ERROR) << "Missing request metadata";
            return Status(StatusCode::INVALID_ARGUMENT, "Missing request metadata");
        }

        const string& filename = request->metadata().name();
        string full_path = WrapPath(filename);

        if (!(get_file_status(full_path, response))) {
            return Status(StatusCode::INTERNAL, "Error getting file stats");
        }

        return Status::OK;
    }

};