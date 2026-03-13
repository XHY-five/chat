#pragma once
#include<grpcpp/grpcpp.h>
#if defined(__linux__)
#include <message.grpc.pb.h>
#else
#include "message.grpc.pb.h"
#endif
#include "const.h"
#include "Singleton.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

class RPCconPool {
public:
	RPCconPool(std::size_t poolsize, std::string host, std::string port);
	~RPCconPool() {
		std::lock_guard<std::mutex> lock(mutex_);
		Close();
		while (!connections_.empty()) {
			connections_.pop();
		}
		
	}
	void Close();
	std::unique_ptr<VarifyService::Stub > getConnection();
	void returnConnection(std::unique_ptr<VarifyService::Stub> context);
private:
	std::atomic<bool> b_stop_;
	std::string host_;
	std::string port_;
	std::size_t poolsize_;
	std::queue<std::unique_ptr<VarifyService::Stub>> connections_;
	std::condition_variable cond_;
	std::mutex mutex_;

};

class VerifyGrpcClient:public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;
public:
	GetVarifyRsp GetVarifyCode(std::string email);
private:
	VerifyGrpcClient();
	std::unique_ptr<RPCconPool> pool_;
};
