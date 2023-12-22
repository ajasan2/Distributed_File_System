#include <map>
#include <mutex>
#include <shared_mutex>
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

using dfs_service::DFSService;


extern dfs_log_level_e DFS_LOG_LEVEL;

class DFSServiceImpl final :
    public DFSService::WithAsyncMethod_CallbackList<DFSService::Service>,
        public DFSCallDataManager<FileRequestType , FileListResponseType> {

private:
    /** Locked file names to client ID **/
    map<string, string> locked_files;

    /** Main mutex for all file locks ID **/
    mutex master_m;

    /** Mutex for accessing directory file list **/
    mutex directory_m;

public:

    ~DFSServiceImpl() {
        this->runner.Shutdown();
    }

    Status GetWriteLock(ServerContext* context, const FileContext* request, Blank* response) override {
        if (context->IsCancelled()){
            dfs_log(LL_ERROR) << "Deadline expired";
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline expired");   
        }

        // Redacted lock checkout

        dfs_log(LL_DEBUG2) << "Client " << request->metadata().client_id() << " locked file '" << request->metadata().name() << "'";
        locked_files[request->metadata().name()] = request->metadata().client_id();
        return Status::OK;
    }

    Status ReleaseWriteLock(ServerContext* context, const FileContext* request, Blank* response) override {
        
        // Redacted lock return

        return Status(StatusCode::FAILED_PRECONDITION, "Trying to unlock file that client does not have access to");
    }

    Status UploadFile(ServerContext* context, ServerReader<FileContext>* reader, FileContext* response) override {
        dfs_log(LL_DEBUG2) << "Entering UploadFile";
        if (context->IsCancelled()){
            dfs_log(LL_ERROR) << "Deadline expired";
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline expired");   
        }

        FileContext client_file;
        if (!reader->Read(&client_file)) {
            dfs_log(LL_ERROR) << "Metadata not received";
            return Status(StatusCode::INVALID_ARGUMENT, "Metadata not received");
        }

        // Redacted pre-condition validation

        dfs_log(LL_SYSINFO) << "Storing file '" << client_file.metadata().name() << "'";
        ofstream ofs(full_path, ios::binary);
        if (!ofs.is_open()) {
            dfs_log(LL_ERROR) << "Failed to open file '" << full_path << "' for writing";
            return Status(StatusCode::INTERNAL, "Failed to open file for writing");
        }

        // Redacted file storage

        ofs.close();
        return Status::OK;
    }

    Status DownloadFile(ServerContext* context, const FileContext* request, ServerWriter<FileContext>* writer) override {
        dfs_log(LL_DEBUG2) << "Entering DownloadFile";
        if (context->IsCancelled()){
            dfs_log(LL_ERROR) << "Deadline expired";
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline expired");
        }

        const string& full_path = WrapPath(request->metadata().name());
        FileContext server_stats;
        if (!get_file_status(full_path, &server_stats)) {
            return Status(StatusCode::NOT_FOUND, "File does not exist");
        }

        // Redacted pre-condition validation

        dfs_log(LL_SYSINFO) << "Sending file '" << full_path << "'";

        ifstream ifs(full_path, ios::binary);
        if (!ifs.is_open()) {
            dfs_log(LL_ERROR) << "Failed to open file '" << full_path << "' for reading";
            return Status(StatusCode::INTERNAL, "Failed to open file");
        }

        // Redacted sending file chunks

        ifs.close();
        return Status::OK;
    }

    Status ListFiles(ServerContext* context, const Blank* request, FileCatalog* response) override {
        dfs_log(LL_DEBUG2) << "Listing files";
        
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

        response->mutable_metadata()->set_name(request->metadata().name());
        const string& full_path = WrapPath(request->metadata().name());
        if (!get_file_status(full_path, response)) {
            return Status(StatusCode::NOT_FOUND, "File does not exist");
        }
        
        return Status::OK;
    }

    Status RemoveFile(ServerContext* context, const FileContext* request, Blank* response) override {
        dfs_log(LL_DEBUG2) << "Entering RemoveFile";
        if (context->IsCancelled()) {
            dfs_log(LL_ERROR) << "Deadline expired";
            return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline expired");   
        } 
        if (!request->has_metadata()) {
            dfs_log(LL_ERROR) << "Missing request metadata";
            return Status(StatusCode::INVALID_ARGUMENT, "Missing request metadata");
        }

        string full_path = WrapPath(request->metadata().name());
        
        // Redacted file removal
        
        return Status::OK;
    }

};