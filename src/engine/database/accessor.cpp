#include "accessor.h"
#include <utils/platform.h>
#include <net/scheduler.h>
#include <list>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <assert.h>

#ifdef PLATFORM_WIN32
#include <WinSock2.h>
#endif
#include <mysql.h>

using namespace mysql;
using namespace std::placeholders;

typedef std::shared_ptr<Query> QueryPtr;

/////////////////////////////////////////////////////////////////////////////
class Worker : public std::enable_shared_from_this<Worker>
{
public:
	typedef std::shared_ptr<Worker> Ptr;
	typedef std::function<void(Ptr)> QueryFinishedHandlerType;

	Worker(Accessor* mgr, const QueryFinishedHandlerType& query_finished, Serial& serial);
	~Worker();

	bool start(const ConnectParams& params);
	void stop();

	void post(const QueryPtr& query);

	void post_to_queue(const QueryPtr& query);

	unsigned int get_query_queue_size();

private:
	void run();

	void handle_query(const QueryPtr query);

	size_t getField(MYSQL_FIELD* field, MYSQL_ROW row, char* buff);

	size_t calcRowSize(MYSQL_RES* res);

	void handle_result(const QueryPtr query, Result result, std::shared_ptr<std::vector<char> > buff);

private:
	Accessor* parent_;

	MYSQL conn_;

	std::shared_ptr<std::thread> thread_;

	bool working_;

	typedef std::list<QueryPtr> QueryQueue;
	QueryQueue query_queue_;
	QueryPtr follower_query_;

	std::condition_variable_any cond_;
	std::mutex mutex_;

	QueryFinishedHandlerType on_query_finished;
	Serial& serial;
};

/////////////////////////////////////////////////////////////////////////////

Worker::Worker(Accessor* parent, const QueryFinishedHandlerType& query_finished, Serial& serial)
	: parent_(parent)
	, working_(false)
	, on_query_finished(query_finished)
	, serial(serial)
{
}

Worker::~Worker()
{
}

bool Worker::start(const ConnectParams& params)
{
	if (mysql_init(&conn_) == nullptr)
		return false;

	// mysql_options()
	if (mysql_real_connect(&conn_, params.host.c_str(), params.user.c_str(), params.password.c_str(), params.db.c_str(), params.port, nullptr, 0) == nullptr)
	{
		mysql_close(&conn_);
		return false;
	}

	if (mysql_set_character_set(&conn_, "utf8") != 0)
	{
		mysql_close(&conn_);
		return false;
	}

	working_ = true;

	thread_.reset(new std::thread(&Worker::run, shared_from_this()));
	return true;
}

void Worker::stop()
{
	working_ = false;

	if (thread_ != nullptr)
	{
		cond_.notify_one();
		thread_->join();
		thread_.reset();
	}

	mysql_close(&conn_);
}

void Worker::post(const QueryPtr& query)
{
	{
		std::lock_guard<std::mutex> guard(mutex_);
		assert(follower_query_ == nullptr);
		follower_query_ = query;
	}

	cond_.notify_one();
}

void Worker::post_to_queue(const QueryPtr& query)
{
	if (!working_) return;

	{
		std::lock_guard<std::mutex> guard(mutex_);
		query_queue_.push_back(query);
	}

	cond_.notify_one();
}

unsigned int Worker::get_query_queue_size()
{
	std::lock_guard<std::mutex> guard(mutex_);
	return query_queue_.size();
}

void Worker::run()
{
	QueryQueue query_queue;
	QueryPtr follower_query;

	while (working_)
	{
		assert(query_queue.empty());

		{
			std::unique_lock<std::mutex> lock(mutex_);
			if (!follower_query_ && query_queue_.empty())
			{
				cond_.wait(lock);
			}

			if (follower_query_)
			{
				follower_query = follower_query_;
				follower_query_.reset();
			}
			if (!query_queue_.empty())
			{
				query_queue_.swap(query_queue);
			}
		}

		while (!query_queue.empty())
		{
			handle_query(query_queue.front());
			query_queue.pop_front();
		}

		if (follower_query != nullptr)
		{
			handle_query(follower_query);
			follower_query.reset();

			on_query_finished(shared_from_this());
		}
	}
}

size_t Worker::getField(MYSQL_FIELD* field, MYSQL_ROW row, char* buff)
{
	switch (field->type)
	{
	case MYSQL_TYPE_TINY:
		if ((field->flags & UNSIGNED_FLAG) != 0)
			*(uint8_t*)buff = static_cast<uint8_t>(*row != nullptr ? atoi(*row) : 0);
		else
			*(int8_t*)buff = static_cast<int8_t>(*row != nullptr ? atoi(*row) : 0);
		return 1;

	case MYSQL_TYPE_SHORT:
		if ((field->flags & UNSIGNED_FLAG) != 0)
			*(uint16_t*)buff = *row != nullptr ? (uint16_t)atoi(*row) : 0;
		else
			*(int16_t*)buff = *row != nullptr ? (int16_t)atoi(*row) : 0;
		return 2;

	case MYSQL_TYPE_LONG:
		if ((field->flags & UNSIGNED_FLAG) != 0)
			*(uint32_t*)buff = *row != nullptr ? (uint32_t)atoi(*row) : 0;
		else
			*(int32_t*)buff = *row != nullptr ? (uint32_t)atoi(*row) : 0;
		return 4;

	case MYSQL_TYPE_LONGLONG:
		
		if ((field->flags & UNSIGNED_FLAG) != 0)
			*(uint64_t*)buff = *row != nullptr ? (uint64_t)atoll(*row) : 0;
		else
			*(int64_t*)buff = *row != nullptr ? (int64_t)atoll(*row) : 0;
		return 8;

	case MYSQL_TYPE_FLOAT:
		*(float*)buff = *row != nullptr ? (float)atof(*row) : 0.0f;
		return 4;

	case MYSQL_TYPE_DOUBLE:
		*(double*)buff = *row != nullptr ? atof(*row) : 0.0;
		return 8;

	case MYSQL_TYPE_TIME:
		if (*row != nullptr)
			memcpy(buff, *row, sizeof(Time));
		else
			memset(buff, 0, sizeof(Time));
		return sizeof(Time);

	case MYSQL_TYPE_DATE:
		if (*row != nullptr)
			memcpy(buff, *row, sizeof(Time));
		else
			memset(buff, 0, sizeof(Time));
		return sizeof(Time);

	case MYSQL_TYPE_DATETIME:
		if (*row != nullptr)
			memcpy(buff, *row, sizeof(Time));
		else
			memset(buff, 0, sizeof(Time));
		return sizeof(Time);

	case MYSQL_TYPE_TIMESTAMP:
		if (*row != nullptr)
			memcpy(buff, *row, sizeof(Time));
		else
			memset(buff, 0, sizeof(Time));
		return sizeof(Time);

	case MYSQL_TYPE_STRING:
	case MYSQL_TYPE_VAR_STRING:
		// data length
		if ((field->flags & BINARY_FLAG) != 0)
		{
			if (*row != nullptr)
			{
				// max_length
				// 对结果集合的字段的最大宽度(对实际在结果集合中的行的最长字段值的长度)。
				// 如果你使用mysql_store_result() 或mysql_list_fields()，
				// 这包含字段最大长度。如果你使用mysql_use_result()，这个变量的值是零。
				memcpy(buff, *row, field->max_length);
				if (field->max_length < field->length)
				{
					memset(buff + field->max_length, 0, field->length - field->max_length);
				}
			}
			else
			{
				memset(buff, 0, field->length);
			}
			return field->length;
		}
		else
		{
			if (*row != nullptr)
			{
				memcpy(buff, *row, field->max_length);
				if (field->max_length < field->length)
				{
					memset(buff + field->max_length, 0, field->length / 3 + 1 - field->max_length);
				}
			}
			else
			{
				memset(buff, 0, (field->length / 3) + 1);
			}
			return (field->length / 3) + 1;
		}
		break;

	case MYSQL_TYPE_BLOB:
		// data_length
		if (*row != nullptr)
			memcpy(buff, *row, field->length);
		else
			memset(buff, 0, field->length);
		return field->length;
	}
	return 0;
}

void Worker::handle_query(QueryPtr query)
{
	Result result;
	result.queryType = query->queryType;

	result.error = mysql_real_query(&conn_, query->sql.c_str(), query->sql.length());
	if (result.error != 0)
	{
		result.error = mysql_errno(&conn_);
		// Errors
		// 2014: CR_COMMANDS_OUT_OF_SYNC
		// Commands were executed in an improper order.
		// 2006: CR_SERVER_GONE_ERROR
		// The MySQL server has gone away.
		// 2013: CR_SERVER_LOST
		// The connection to the server was lost during the query.
		// 2000: CR_UNKNOWN_ERROR
		// An unknown error occurred.

		if (query->handler)
			serial.Post(std::bind(query->handler, result));

		return;
	}

	if (mysql_field_count(&conn_) != 0)
	{
		// Select
		if (query->queryType != QueryType::BatchQuery)
		{
			do
			{
				auto* res = mysql_store_result(&conn_);
				if (res == nullptr) continue;

				result.fields = nullptr;
				result.prefetchRows = query->prefetchRows;
				result.rowCount = (int)mysql_num_rows(res);
				result.rowSize = calcRowSize(res);

				std::shared_ptr<std::vector<char> > buff_;
				size_t buffSize = result.rowCount * result.rowSize;
				if (buffSize != 0)
				{
					buff_.reset(new std::vector<char>(buffSize));
					memset(&(*buff_)[0], 0, buffSize);
					result.data = &(*buff_)[0];
				}

				int finishedRow = 0;
				size_t index = 0;
				while (auto row = mysql_fetch_row(res))
				{
					if (result.rowCount <= finishedRow)
						break;

					mysql_field_seek(res, 0);
					while (auto* field = mysql_fetch_field(res))
					{
						if (index >= buffSize)
						{
							assert(false);
							return;
						}
						index += getField(field, row, &(*buff_)[index]);
						++row;
					}

					++finishedRow;
				}

				serial.Post(std::bind(&Worker::handle_result, shared_from_this(), query, result, buff_));

				mysql_free_result(res);

				// more results? -1 = no, >0 = error, 0 = yes (keep looping)
			} while (mysql_next_result(&conn_) == 0);
		}
		else
		{
			do
			{
				auto* res = mysql_store_result(&conn_);
				if (res == nullptr) continue;

				if (query->prefetchRows == 0)
					query->prefetchRows = DEFAULT_PREFETCH_ROWS;

				int row_count = (int)mysql_num_rows(res);

				result.fields = nullptr;
				result.prefetchRows = query->prefetchRows;
				result.rowCount = row_count;
				result.rowSize = calcRowSize(res);

				if (result.rowCount > query->prefetchRows)
					result.rowCount = query->prefetchRows;

				//char* buff_ = nullptr;
				std::shared_ptr<std::vector<char> > buff_;
				size_t buffSize = result.rowCount * result.rowSize;
				if (buffSize != 0)
				{
					buff_.reset(new std::vector<char>(buffSize));
					memset(&(*buff_)[0], 0, buffSize);
					result.data = &(*buff_)[0];
				}

				int finishedRow = 0;
				size_t index = 0;
				while (auto row = mysql_fetch_row(res))
				{
					if (row_count <= finishedRow) break;

					mysql_field_seek(res, 0);
					while (auto* field = mysql_fetch_field(res))
					{
						if (index >= buffSize)
						{
							assert(false);
							return;
						}

						index += getField(field, row, &(*buff_)[index]);
						++row;
					}

					++finishedRow;
					if (finishedRow % query->prefetchRows == 0)
					{
						serial.Post(std::bind(&Worker::handle_result, shared_from_this(), query, result, buff_));

						assert(finishedRow <= row_count);
						int leftRow = row_count - finishedRow;
						result.rowCount = query->prefetchRows < leftRow ? query->prefetchRows : leftRow;
						size_t buffSize = result.rowCount * result.rowSize;
						if (buffSize != 0)
						{
							buff_.reset(new std::vector<char>(buffSize));
							memset(&(*buff_)[0], 0, buffSize);
							result.data = &(*buff_)[0];
						}
						index = 0;
					}
				}

				if (result.rowCount != 0 && finishedRow % query->prefetchRows != 0)
				{
					serial.Post(std::bind(&Worker::handle_result, shared_from_this(), query, result, buff_));
				}

				result.rowCount = 0;
				result.data = nullptr;
				serial.Post(std::bind(&Worker::handle_result, shared_from_this(), query, result, nullptr));

				mysql_free_result(res);

				// more results? -1 = no, >0 = error, 0 = yes (keep looping)
			} while (mysql_next_result(&conn_) == 0);
		}
	}
	else
	{
		// Update
		result.effected = mysql_affected_rows(&conn_);

		if (query->handler)
			serial.Post(std::bind(query->handler, result));
	}
}

size_t Worker::calcRowSize(MYSQL_RES* res)
{
	assert(res != nullptr);

	size_t rowSize = 0;

	mysql_field_seek(res, 0);
	while (MYSQL_FIELD* field = mysql_fetch_field(res))
	{
		switch (field->type)
		{
		case MYSQL_TYPE_TINY:
			rowSize += 1;
			break;

		case MYSQL_TYPE_SHORT:
			rowSize += 2;
			break;

		case MYSQL_TYPE_LONG:
		case MYSQL_TYPE_FLOAT:
			rowSize += 4;
			break;

		case MYSQL_TYPE_LONGLONG:
		case MYSQL_TYPE_DOUBLE:
			rowSize += 8;
			break;

		case MYSQL_TYPE_TIME:
		case MYSQL_TYPE_DATE:
		case MYSQL_TYPE_DATETIME:
		case MYSQL_TYPE_TIMESTAMP:
			rowSize += sizeof(Time);
			break;

		case MYSQL_TYPE_STRING:
		case MYSQL_TYPE_VAR_STRING:
			// data length
			if ((field->flags & BINARY_FLAG) != 0)
				rowSize += field->length;
			else
				rowSize += field->length / 3 + 1;
			break;

		case MYSQL_TYPE_BLOB:
			// data_length
			rowSize += field->length;
			break;

		default:
			break;
		}
	}
	return rowSize;
}

void Worker::handle_result(QueryPtr query, Result result, std::shared_ptr<std::vector<char> > buff)
{
	if (query->handler)
		query->handler(result);

	buff.reset();
	//if (buff != nullptr)
	//	delete []buff;
}

/////////////////////////////////////////////////////////////////////////////
struct Accessor::Core : public std::enable_shared_from_this<Accessor::Core>
{
	typedef std::vector<Worker::Ptr> Workders;
	typedef std::list<Worker::Ptr> FollowersQueue;
	typedef std::list<QueryPtr> QueryQueue;

	Workders workers_;
	FollowersQueue followers_queue_;
	QueryQueue query_queue_;

	// 都上锁的时候约定先锁query_queue_mutex_，然后是worker_mutex_
	std::mutex query_queue_mutex_;
	std::mutex worker_mutex_;

	void on_query_finished(Worker::Ptr worker);

	void post_query_to_follower();

	Worker::QueryFinishedHandlerType GetQueryFinishedHandler();
};

/////////////////////////////////////////////////////////////////////////////

void Accessor::Core::on_query_finished(Worker::Ptr worker)
{
	{
		std::lock_guard<std::mutex> guard(worker_mutex_);
		followers_queue_.push_back(worker);
	}
	post_query_to_follower();
}

void Accessor::Core::post_query_to_follower()
{
	std::lock_guard<std::mutex> guard1(query_queue_mutex_);
	std::lock_guard<std::mutex> guard2(worker_mutex_);

	while (!followers_queue_.empty() && !query_queue_.empty())
	{
		auto worker = followers_queue_.front();
		auto query = query_queue_.front();
		followers_queue_.pop_front();
		query_queue_.pop_front();

		worker->post(query);
	}
}

inline Worker::QueryFinishedHandlerType Accessor::Core::GetQueryFinishedHandler()
{
	return std::bind(&Core::on_query_finished, shared_from_this(), _1);
}

/////////////////////////////////////////////////////////////////////////////
Accessor::Accessor()
	: mCore(new Accessor::Core())
{
}

Accessor::~Accessor()
{
}

bool Accessor::Init(const ConnectParams& params)
{
	bool ret = true;

	mCore->workers_.resize(params.workerNum);
	auto& serial = net::Scheduler::GetInstance().GetSerial();
	auto& handler = mCore->GetQueryFinishedHandler();
	for (size_t i = 0; i < mCore->workers_.size(); ++i)
	{
		Worker::Ptr worker(new Worker(this, handler, serial));
		if (!worker->start(params))
			ret = false;

		mCore->workers_[i] = worker;
	}

	if (!ret)
	{
		for (size_t i = 0; i < mCore->workers_.size(); ++i)
		{
			if (mCore->workers_[i] != nullptr)
				mCore->workers_[i]->stop();
		}

		mCore->workers_.clear();
	}
	else
	{
		for (size_t i = 0; i < mCore->workers_.size(); ++i)
		{
			mCore->followers_queue_.push_back(mCore->workers_[i]);
		}
	}

	return ret;
}

void Accessor::Release()
{
	for (size_t i = 0; i < mCore->workers_.size(); ++i)
	{
		mCore->workers_[i]->stop();
		mCore->workers_[i].reset();
	}

	mCore->workers_.clear();
	mCore->query_queue_.clear();
}

void Accessor::PostQuery(const Query& _query)
{
	QueryPtr query(new Query(_query));
	if (query->allThreadQuery)
	{
		for (size_t i = 0; i < mCore->workers_.size(); ++i)
			mCore->workers_[i]->post_to_queue(query);

		return;
	}

	if (query->bindThread != 0)
	{
		if (query->bindThread > mCore->workers_.size())
			return;

		mCore->workers_[query->bindThread - 1]->post_to_queue(query);

		return;
	}
	else
	{
		{
			std::lock_guard<std::mutex> guard(mCore->query_queue_mutex_);
			mCore->query_queue_.push_back(query);
		}
		mCore->post_query_to_follower();
	}
}

ulong Accessor::EscapeString(char *to, const char* from, unsigned long length)
{
	return mysql_escape_string(to, from, length);
}

std::string mysql::Accessor::EscapeString(const char * from, ulong len)
{
	std::vector<char> temp(len * 2 + 1);
	ulong res = mysql_escape_string(&temp[0], from, len);
	return std::string(&temp[0], res);
}

uint Accessor::GetWorkerThread()
{
	uint min = 0xffffFFFF;
	uint id = 0;
	for (size_t i = 0; i < mCore->workers_.size(); ++i)
	{
		size_t query_size = mCore->workers_[i]->get_query_queue_size();
		if (query_size < min)
		{
			id = i + 1;
			min = query_size;
		}
	}
	return id;
}
