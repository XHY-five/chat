#pragma once
#include"Singleton.h"
#include<boost/asio.hpp>
#include<vector>


class AsioIOServicePool:public Singleton<AsioIOServicePool>
{
	friend Singleton<AsioIOServicePool>;
public:
	using IOService = boost::asio::io_context;
	using Work = boost::asio::executor_work_guard<IOService::executor_type>;
	using WorkPtr = std::unique_ptr<Work>;
	~AsioIOServicePool();
	AsioIOServicePool(const AsioIOServicePool&) = delete;
	AsioIOServicePool& operator = (const AsioIOServicePool&) = delete;
	AsioIOServicePool(const AsioIOServicePool&&) = delete;// 禁止移动构造
	AsioIOServicePool& operator = (const AsioIOServicePool&&) = delete;// 禁止移动赋值
	//使用round-robin 的方式返回一个io_context
	boost::asio::io_context& GetIOService();
	void Stop();

private:
	//hardware_concurrency获取并行的线程数
	AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency());
	std::vector<IOService> _ioServices;
	std::vector<WorkPtr> _works;
	std::vector<std::thread> _threads;

	std::size_t _nextIOService;
};

