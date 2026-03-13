#pragma once
#include "const.h"
#include <mysql/jdbc.h>
class SqlConnection {
public:
	SqlConnection(sql::Connection* con, int64_t lasttime) :_con(con), _last_oper_time(lasttime) {}
	std::unique_ptr<sql::Connection> _con;
	int64_t _last_oper_time;
};

class MySqlPool
{
public:
	MySqlPool(const std::string& url, const std::string& user, 
		const std::string& pass, const std::string& schema, int poolsize);
	std::unique_ptr<SqlConnection> getConnection();
	void returnConnection(std::unique_ptr<SqlConnection> con);
	void checkConnectionPro();
	void checkConnection();
	bool reconnect(long long timestamp);
	void Close();
	~MySqlPool()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		while (!pool_.empty()) {
			pool_.pop();
		}
	}
private:
	std::string url_;
	std::string user_;
	std::string pass_;
	std::string schema_;
	int poolsize_;
	std::queue<std::unique_ptr<SqlConnection>> pool_;
	std::mutex mutex_;
	std::condition_variable cond_;
	std::atomic<bool> b_stop_;
	std::thread _check_thread;
	std::atomic<int> _fail_count;
};


struct UserInfo
{
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
};

class MysqlDao {
public:
	MysqlDao();
	~MysqlDao() {
		pool_->Close();
	}
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& pwd);
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userinfo);

private:
	std::unique_ptr<MySqlPool> pool_;
};

