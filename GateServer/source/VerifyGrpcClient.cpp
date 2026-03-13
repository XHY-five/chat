#include "VerifyGrpcClient.h"
#include "ConfigMgr.h"

GetVarifyRsp VerifyGrpcClient::GetVarifyCode(std::string email)
{
	ClientContext context;
	GetVarifyRsp reply;
	GetVarifyReq request;
	request.set_email(email);
	auto stub = pool_->getConnection();
	Status status = stub ->GetVarifyCode(&context, request, &reply);
	if (status.ok()) {
		pool_->returnConnection(std::move(stub));
		return reply;
	}
	else {
		pool_->returnConnection(std::move(stub));
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

VerifyGrpcClient::VerifyGrpcClient()
{
	auto& gCfgMgr = ConfigMgr::Inst();
	std::string host = gCfgMgr["VarifyServer"]["Host"];
	std::string port = gCfgMgr["VarifyServer"]["Port"];
	pool_.reset(new RPCconPool(5, host, port));
}

RPCconPool::RPCconPool(std::size_t poolsize, std::string host, std::string port)
	:poolsize_(poolsize),
	host_(host),
	port_(port),
	b_stop_(false)
{
	for (std::size_t i = 0; i < poolsize_; ++i) {
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host_ + ":" + port_,
			grpc::InsecureChannelCredentials());
		connections_.push(VarifyService::NewStub(channel));
	}
}

void RPCconPool::Close()
{
	b_stop_ = true;
	cond_.notify_all();

}

std::unique_ptr<VarifyService::Stub > RPCconPool::getConnection()
{
	std::unique_lock<std::mutex> lock(mutex_);
	cond_.wait(lock, [this]() {
		if (b_stop_) {
			return true;
		}
		return !connections_.empty();
		});

	if (b_stop_) {
		return nullptr;
	}
	auto context = std::move(connections_.front());
	connections_.pop();
	return context;
}

void RPCconPool::returnConnection(std::unique_ptr<VarifyService::Stub> context)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (b_stop_) {
		return;
	}
	connections_.push(std::move(context));
	cond_.notify_one();
}
