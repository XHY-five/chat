#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include <iostream>
#include "Singleton.h"
#include <functional>
#include <map>
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <hiredis.h>
#if defined(__linux__)
#include <message.pb.h>
#include <message.grpc.pb.h>
#else
#include "message.pb.h"
#include "message.grpc.pb.h"
#endif
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <atomic>
namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;


enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,
	RPCFailed = 1002,
	VerifyExpired = 1003,
	VerifyCodeErr = 1004,
	UserExist = 1005,
	PasswdErr = 1006,
	EmailNotMatch = 1007, 
	PasswdUpFailed = 1008,  
	PasswdInvalid = 1009,   
	TokenInvalid = 1010,  
	UidInvalid = 1011,  

};



// Defer类
class Defer {
public:
	// 接受一个lambda表达式或者函数指针
	Defer(std::function<void()> func) : func_(func) {}

	// 析构函数中执行传入的函数
	~Defer() {
		func_();
	}

private:
	std::function<void()> func_;
};


#define CODEPREFIX "code_"


