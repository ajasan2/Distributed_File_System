#include <regex>
#include <mutex>
#include <vector>
#include <string>
#include <thread>
#include <cstdio>
#include <chrono>
#include <errno.h>
#include <csignal>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <getopt.h>
#include <unistd.h>
#include <limits.h>
#include <sys/inotify.h>
#include <grpcpp/grpcpp.h>
#include <utime.h>

using grpc::Status;
using grpc::Channel;
using grpc::StatusCode;
using grpc::ClientWriter;
using grpc::ClientReader;
using grpc::ClientContext;

extern dfs_log_level_e DFS_LOG_LEVEL;

DFSClientNodeP2::DFSClientNodeP2() : DFSClientNode() {}
DFSClientNodeP2::~DFSClientNodeP2() {}

grpc::StatusCode DFSClientNodeP2::RequestWriteAccess(const std::string &filename) {

    ClientContext context;
    context.set_deadline(system_clock::now() + milliseconds(deadline_timeout));

    Blank response;
    FileContext request;
    request.mutable_metadata()->set_name(filename);
    request.mutable_metadata()->set_client_id(client_id);

    Status lock_result = service_stub->GetWriteLock(&context, request, &response);
    if (!lock_result.ok()) {
        dfs_log(LL_ERROR) << lock_result.error_message();
        return lock_result.error_code();
    }

    dfs_log(LL_DEBUG2) << "Client " << client_id << " successfully acquired the write lock for '" << filename << "'";
    return StatusCode::OK;
}

grpc::StatusCode DFSClientNodeP2::Store(const std::string &filename) {

    dfs_log(LL_DEBUG2) << "Entering Store";

    const string& full_path = WrapPath(filename);
    FileContext client_stats;
    if (!get_file_status(full_path, &client_stats)) {
        dfs_log(LL_ERROR) << "File '" << full_path << "' does not exist";
        return StatusCode::NOT_FOUND;
    }

    StatusCode lock_result = this->RequestWriteAccess(filename);
    if (lock_result != StatusCode::OK) {
        return lock_result;
    }

    FileContext response;
    unique_ptr<ClientWriter<FileContext>> writer = service_stub->UploadFile(&context, &response);
        
    Context client;
    client.mutable_metadata()->set_name(filename);
    client.mutable_metadata()->set_last_modified(client_stats.metadata().last_modified());

    dfs_log(LL_SYSINFO) << "Uploading file '" << full_path << "' with mtime " << client_stats.metadata().last_modified();

    if (!writer->Write(client)) {
        dfs_log(LL_ERROR) << "Could not send file request to server";
        return StatusCode::CANCELLED;
    }

    ifstream ifs(full_path, ios::binary);
    if (!ifs.is_open()) {
        this->CedeWriteAccess(filename);
        dfs_log(LL_ERROR) << "Failed to open file";
        return StatusCode::CANCELLED;
    }

    // Redacted storing file chunks

    if (!server_result.ok()) {
        dfs_log(LL_ERROR) << "Upload failed";
        if (server_result.error_code() == StatusCode::INTERNAL) {
            this->CedeWriteAccess(filename);
            return StatusCode::CANCELLED;
        }
    }
    return server_result.error_code();
}

grpc::StatusCode DFSClientNodeP2::Fetch(const std::string &filename) {

    dfs_log(LL_DEBUG2) << "Entering Fetch";
    ClientContext context;
    context.set_deadline(system_clock::now() + milliseconds(deadline_timeout));

    FileContext request;
    request.mutable_metadata()->set_name(filename);

    const string& full_path = WrapPath(filename);
    uint32_t client_crc = dfs_file_checksum(full_path, &this->crc_table);
    request.mutable_metadata()->set_crc(client_crc);

    unique_ptr<ClientReader<FileContext>> reader = service_stub->DownloadFile(&context, request);
    
    FileContext content;
    
    // Redacted receiving files

    if (!server_result.ok()) {
        dfs_log(LL_ERROR) << "Download failed";
        if (server_result.error_code() == StatusCode::INTERNAL) {
            return StatusCode::CANCELLED;
        }
    }
    return server_result.error_code();

}

grpc::StatusCode DFSClientNodeP2::Delete(const std::string &filename) {

    dfs_log(LL_DEBUG2) << "Entering Delete";
    ClientContext context;
    context.set_deadline(system_clock::now() + milliseconds(deadline_timeout));
    
    FileContext request;
    request.mutable_metadata()->set_name(filename);
    Blank response;

    StatusCode lock_result = this->RequestWriteAccess(filename);
    if (lock_result != StatusCode::OK) {
        return lock_result;
    }

    Status server_result = service_stub->RemoveFile(&context, request, &response);
    if (!server_result.ok()) {
        dfs_log(LL_ERROR) << "RemoveFile failed";
        if (server_result.error_code() == StatusCode::INTERNAL) {
            this->CedeWriteAccess(filename);
            return StatusCode::CANCELLED;
        }
    }
    return server_result.error_code();
}

grpc::StatusCode DFSClientNodeP2::List(std::map<std::string,int>* file_map, bool display) {

    dfs_log(LL_DEBUG2) << "Entering List";
    ClientContext context;
    context.set_deadline(system_clock::now() + milliseconds(deadline_timeout));

    Blank request;
    FileCatalog response;

    Status server_result = service_stub->ListFiles(&context, request, &response);
    if (!server_result.ok()) {
        dfs_log(LL_ERROR) << "Listing files failed";
        if (server_result.error_code() == StatusCode::INTERNAL) {
            return StatusCode::CANCELLED;
        }
    }

    for (const FileContext& fc : response.files()) {
        file_map->insert(pair<string,int>(fc.metadata().name(), fc.metadata().last_modified()));
        dfs_log(LL_DEBUG2) << "Adding " << fc.metadata().name() << ", mtime " << fc.metadata().last_modified();
    }

    return server_result.error_code();
}

grpc::StatusCode DFSClientNodeP2::Stat(const std::string &filename, void* file_status) {

    ClientContext context;
    context.set_deadline(system_clock::now() + milliseconds(deadline_timeout));

    FileContext request, response;
    request.mutable_metadata()->set_name(filename);

    Status server_result = service_stub->GetFileStatus(&context, request, &response);
    if (!server_result.ok()) {
        dfs_log(LL_ERROR) << "GetFileStatus failed";
        if (server_result.error_code() == StatusCode::INTERNAL) {
            return StatusCode::CANCELLED;
        }
    }
    return server_result.error_code();
}


void DFSClientNodeP2::HandleCallbackList() {

    void* tag;
    bool ok = false;

    while (completion_queue.Next(&tag, &ok)) {
        {

            AsyncClientData<FileListResponseType> *call_data = static_cast<AsyncClientData<FileListResponseType> *>(tag);

            dfs_log(LL_DEBUG3) << "Received completion queue callback";
            if (!ok) {
                dfs_log(LL_ERROR) << "Completion queue callback not ok.";
            }

            if (ok && call_data->status.ok()) {

                dfs_log(LL_DEBUG2) << "Handling async callback ";

                ClientContext context;
                FileContext request, response;

                // Redacted file deletion broadcast

                for (const FileContext& server_file : call_data->reply.files()) {
                    FileContext local_file;
                    StatusCode server_result = StatusCode::OK;
                    const string& local_path = WrapPath(server_file.metadata().name());

                    dfs_log(LL_DEBUG2) << "Checking on file '" << server_file.metadata().name() << "'";
                    get_file_status(local_path, &local_file);

                    // Redacted file synchronization logic
                    if (client_mtime != server_mtime) {
                        // Check if file does not exist locally
                        if (client_mtime == 0) {}
                        // Check if local file is up-to-date
                        else if (client_mtime < server_mtime) {}
                        // Check if file on server needs to be updated
                        else if (client_mtime > server_mtime) {}
                    }
                }
            }
        }
    }
}


grpc::StatusCode DFSClientNodeP2::CedeWriteAccess(const std::string &filename) {
    Blank response;
    FileContext request;
    request.mutable_metadata()->set_name(filename);
    request.mutable_metadata()->set_client_id(client_id);
    
    // Redacted setting pre-conditions

    if (!unlock_result.ok()) {
        dfs_log(LL_ERROR) << unlock_result.error_message();
        return unlock_result.error_code();
    }

    dfs_log(LL_DEBUG2) << "Client " << client_id << " successfully gave up write lock for '" << filename << "'";
    return StatusCode::OK;
}

