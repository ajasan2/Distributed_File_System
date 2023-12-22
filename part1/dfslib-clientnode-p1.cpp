#include <regex>
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

using grpc::Status;
using grpc::Channel;
using grpc::StatusCode;
using grpc::ClientWriter;
using grpc::ClientReader;
using grpc::ClientContext;


DFSClientNodeP1::DFSClientNodeP1() : DFSClientNode() {}

DFSClientNodeP1::~DFSClientNodeP1() noexcept {}

StatusCode DFSClientNodeP1::Store(const std::string &filename) {

    const string& full_path = WrapPath(filename);
    struct stat file_stats;
    if (stat(full_path.c_str(), &file_stats) != 0){
        dfs_log(LL_ERROR) << "File does not exist:  " << full_path;
        return StatusCode::NOT_FOUND;
    }

    ClientContext context;
    context.set_deadline(system_clock::now() + milliseconds(deadline_timeout));

    FileContext response;
    unique_ptr<ClientWriter<FileContext>> writer = service_stub->UploadFile(&context, &response);

    FileContext file_metadata;
    file_metadata.mutable_metadata()->set_name(filename);
    file_metadata.mutable_metadata()->set_size(file_stats.st_size);
    dfs_log(LL_SYSINFO) << "Uploading file " << full_path << " of size " << file_stats.st_size;
    writer->Write(file_metadata);

    ifstream ifs(full_path, ios::binary);
    if (!ifs.is_open()) {
        dfs_log(LL_ERROR) << "Failed to open file";
        return StatusCode::CANCELLED;
    }

    // Redacted sending file in chunks

    if (!res.ok()) {
        dfs_log(LL_ERROR) << "UploadFile failed";
        if (res.error_code() == StatusCode::INTERNAL) {
            return StatusCode::CANCELLED;
        }
    }

    return res.error_code();
}

StatusCode DFSClientNodeP1::Fetch(const std::string &filename) {

    ClientContext context;
    context.set_deadline(system_clock::now() + milliseconds(deadline_timeout));

    FileContext request;
    request.mutable_metadata()->set_name(filename);

    unique_ptr<ClientReader<FileContext>> reader = service_stub->DownloadFile(&context, request);

    const string& full_path = WrapPath(filename);
    ofstream ofs(full_path, ios::binary);
    if (!ofs.is_open()) {
        dfs_log(LL_ERROR) << "Failed to open file for writing: " << full_path;
        return StatusCode::CANCELLED;
    }
    dfs_log(LL_SYSINFO) << "Downloading file " << full_path;

    // Redacted file reading
    
    if (!res.ok()) {
        dfs_log(LL_ERROR) << "Download failed";
        if (res.error_code() == StatusCode::INTERNAL) {
            return StatusCode::CANCELLED;
        }
    }
    ofs.close();
    return res.error_code();
}

StatusCode DFSClientNodeP1::Delete(const std::string& filename) {

    ClientContext context;
    context.set_deadline(system_clock::now() + milliseconds(deadline_timeout));
    
    FileContext request;
    request.mutable_metadata()->set_name(filename);
    Blank response;

    Status res = service_stub->RemoveFile(&context, request, &response);
    if (!res.ok()) {
        dfs_log(LL_ERROR) << "RemoveFile failed";
        if (res.error_code() == StatusCode::INTERNAL) {
            return StatusCode::CANCELLED;
        }
    }

    return res.error_code();
}

StatusCode DFSClientNodeP1::List(std::map<std::string,int>* file_map, bool display) {

    ClientContext context;
    context.set_deadline(system_clock::now() + milliseconds(deadline_timeout));

    Blank request;
    FileCatalog response;

    Status res = service_stub->ListFiles(&context, request, &response);
    if (!res.ok()) {
        dfs_log(LL_ERROR) << "Listing files failed";
        if (res.error_code() == StatusCode::INTERNAL) {
            return StatusCode::CANCELLED;
        }
    }

    dfs_log(LL_DEBUG3) << response.DebugString();
    for (const FileContext& fc : response.files()) {
        file_map->insert(pair<string,int>(fc.metadata().name(), fc.metadata().last_modified()));
        dfs_log(LL_SYSINFO) << "Adding " << fc.metadata().name() << ", mtime " << fc.metadata().last_modified();
    }

    return res.error_code();
}

StatusCode DFSClientNodeP1::Stat(const std::string &filename, void* file_status) {

    ClientContext context;
    context.set_deadline(system_clock::now() + milliseconds(deadline_timeout));

    FileContext request, response;
    request.mutable_metadata()->set_name(filename);

    Status res = service_stub->GetFileStatus(&context, request, &response);
    if (!res.ok()) {
        dfs_log(LL_ERROR) << "GetFileStatus failed";
        if (res.error_code() == StatusCode::INTERNAL) {
            return StatusCode::CANCELLED;
        }
    }

    dfs_log(LL_SYSINFO) << response.DebugString();
    return res.error_code();
}