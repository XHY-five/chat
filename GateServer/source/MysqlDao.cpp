#include "MysqlDao.h"
#include "ConfigMgr.h"
#include <thread>




MySqlPool::MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolsize)
	:url_(url),
	user_(user),
	pass_(pass),
	schema_(schema),
	poolsize_(poolsize),
	b_stop_(false),
	_fail_count(0)
{
	try
	{
		for (int i = 0; i < poolsize_; ++i) {
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto* con=driver->connect(url_, user_, pass_);
			con->setSchema(schema_);
			auto currentTime = std::chrono::system_clock::now().time_since_epoch();

			long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();

			pool_.push(std::make_unique<SqlConnection>(con, timestamp));
		}

		_check_thread = std::thread([this]() {
			while (!b_stop_) {
				checkConnection();
				std::this_thread::sleep_for(std::chrono::seconds(60));
			}
		});
		_check_thread.detach();
	}
	catch (const sql::SQLException& e)
	{
		//处理异常
		std::cout << "Mysql pool init failed! error is "<<e.what() << std::endl;
	}
}
bool MySqlPool::reconnect(long long timestamp) {
	try {

		sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
		auto* con = driver->connect(url_, user_, pass_);
		con->setSchema(schema_);

		auto newCon = std::make_unique<SqlConnection>(con, timestamp);
		{
			std::lock_guard<std::mutex> guard(mutex_);
			pool_.push(std::move(newCon));
		}

		std::cout << "mysql connection reconnect success" << std::endl;
		return true;

	}
	catch (sql::SQLException& e) {
		std::cout << "Reconnect failed, error is " << e.what() << std::endl;
		return false;
	}
}

void MySqlPool::checkConnection() {
	std::lock_guard<std::mutex> guard(mutex_);
	int poolsize = pool_.size();
	// 获取当前时间戳
	auto currentTime = std::chrono::system_clock::now().time_since_epoch();
	// 将时间戳转换为秒
	long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
	for (int i = 0; i < poolsize; i++) {
		auto con = std::move(pool_.front());
		pool_.pop();
		Defer defer([this, &con]() {
			pool_.push(std::move(con));
			});

		if (timestamp - con->_last_oper_time < 5) {
			continue;
		}

		try {
			std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
			stmt->executeQuery("SELECT 1");
			con->_last_oper_time = timestamp;
			//std::cout << "execute timer alive query , cur is " << timestamp << std::endl;
		}
		catch (sql::SQLException& e) {
			std::cout << "Error keeping connection alive: " << e.what() << std::endl;
			// 重新创建连接并替换旧的连接
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto* newcon = driver->connect(url_, user_, pass_);
			newcon->setSchema(schema_);
			con->_con.reset(newcon);
			con->_last_oper_time = timestamp;
		}
	}
}
void MySqlPool::checkConnectionPro() {
	// 1)先读取“目标处理数”
	size_t targetCount;
	{
		std::lock_guard<std::mutex> guard(mutex_);
		targetCount = pool_.size();
	}

	//2 当前已经处理的数量
	size_t processed = 0;

	//3 时间戳
	auto now = std::chrono::system_clock::now().time_since_epoch();
	long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();

	while (processed < targetCount) {
		std::unique_ptr<SqlConnection> con;
		{
			std::lock_guard<std::mutex> guard(mutex_);
			if (pool_.empty()) {
				break;
			}
			con = std::move(pool_.front());
			pool_.pop();
		}

		bool healthy = true;
		//解锁后做检查/重连逻辑
		if (timestamp - con->_last_oper_time >= 5) {
			try {
				std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
				stmt->executeQuery("SELECT 1");
				con->_last_oper_time = timestamp;
			}
			catch (sql::SQLException& e) {
				std::cout << "Error keeping connection alive: " << e.what() << std::endl;
				healthy = false;
				_fail_count++;
			}

		}

		if (healthy)
		{
			std::lock_guard<std::mutex> guard(mutex_);
			pool_.push(std::move(con));
			cond_.notify_one();
		}

		++processed;
	}

	while (_fail_count > 0) {
		auto b_res = reconnect(timestamp);
		if (b_res) {
			_fail_count--;
		}
		else {
			break;
		}
	}
}

std::unique_ptr<SqlConnection> MySqlPool::getConnection()
{
	std::unique_lock<std::mutex> lock(mutex_);
	cond_.wait(lock, [this] {
		if (b_stop_) {
			return true;
		}
		return !pool_.empty();
	});
	if (b_stop_) {
		return nullptr;
	}
	std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
	pool_.pop();
	return con;

}

void MySqlPool::returnConnection(std::unique_ptr<SqlConnection> con)
{
	std::unique_lock<std::mutex> lock(mutex_);
	if (b_stop_) {
		return;
	}
	pool_.push(std::move(con));
	cond_.notify_one();
}

void MySqlPool::Close()
{
	b_stop_ = true;
	cond_.notify_all();
}

MysqlDao::MysqlDao()
{
	auto& cfg = ConfigMgr::Inst();
	const auto& host = cfg["Mysql"]["Host"];
	const auto& port = cfg["Mysql"]["Port"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& user = cfg["Mysql"]["User"];
	const auto& Schema = cfg["Mysql"]["Schema"];
	pool_.reset(new MySqlPool(host + ":" + port, user, pwd, Schema, 5));
}
//注册用户
int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}
		// 准备调用存储过程
		std::unique_ptr < sql::PreparedStatement > stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
		// 设置输入参数
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);

		// 由于PreparedStatement不直接支持注册输出参数，我们需要使用会话变量或其他方法来获取输出参数的值

		  // 执行存储过程
		stmt->execute();
		// 如果存储过程设置了会话变量或有其他方式获取输出参数的值，你可以在这里执行SELECT查询来获取它们
	   // 例如，如果存储过程设置了一个会话变量@result来存储输出结果，可以这样获取：
		std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
		if (res->next()) {
			int result = res->getInt("result");
			std::cout << "Result: " << result << std::endl;
			pool_->returnConnection(std::move(con));
			return result;
		}
		pool_->returnConnection(std::move(con));
		return -1;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email)
{
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			pool_->returnConnection(std::move(con));
			return false;
		}
		// 准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));
		// 绑定参数
		pstmt->setString(1, name);
		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		// 遍历结果集
		while (res->next()) {
			std::cout << "Check Email: " << res->getString("email") << std::endl;
			if (email != res->getString("email")) {
				pool_->returnConnection(std::move(con));
				return false;
			}
			pool_->returnConnection(std::move(con));
			return true;
		}
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd)
{
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			pool_->returnConnection(std::move(con));
			return false;
		}
		// 准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));
		// 绑定参数
		pstmt->setString(2, name);
		pstmt->setString(1, newpwd);
		// 执行更新
		int updateCount = pstmt->executeUpdate();
		std::cout << "Updated rows: " << updateCount << std::endl;
		pool_->returnConnection(std::move(con));
		return true;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userinfo)
{
	auto con = pool_->getConnection();
	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});
	try {
		if (con == nullptr) {
			return false;
		}
		// 准备SQL语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE email = ?"));
		pstmt->setString(1, name); // 将username替换为你要查询的用户名
		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::string origin_pwd = "";
		// 遍历结果集
		while (res->next()) {
			origin_pwd = res->getString("pwd");
			// 输出查询到的密码
			std::cout << "Password: " << origin_pwd << std::endl;
			break;
		}
		if (pwd != origin_pwd) {
			return false;
		}
		userinfo.name = name;
		userinfo.email = res->getString("email");
		userinfo.uid = res->getInt("uid");
		userinfo.pwd = origin_pwd;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}
