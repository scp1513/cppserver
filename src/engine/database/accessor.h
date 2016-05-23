#ifndef __DB_MYSQL_ASYNC_MGR_HEADER__
#define __DB_MYSQL_ASYNC_MGR_HEADER__

#include <utils/typedef.h>
#include <string>
#include <functional>
#include <memory>

// TODO:
// 1. 自动重连
// 2. 编码
// 3. mysql_escape_string -> mysql_real_escape_string
namespace mysql
{
	enum { DEFAULT_PREFETCH_ROWS = 10 };

#pragma pack(push,1)
	// datetime type
	struct Time
	{
		unsigned int  year, month, day, hour, minute, second;
		unsigned long second_part;
		unsigned int  reserve1, reserve2;
	};
#pragma pack(pop)

	struct Result;
	typedef std::function<void(Result)> ResultHandler;

	enum class QueryType
	{
		Query,
		Execute,
		BatchQuery,
		NormalQuery, // include type Query and Execute
	};

	struct Query
	{
		ResultHandler handler;
		std::string   sql;
		QueryType     queryType;
		int           prefetchRows;    // number of rows per one COM_FETCH
		unsigned int  bindThread;      // bind worker thread
		bool          allThreadQuery; // 是否所有连接都执行当此查询

		Query()
		{
			queryType      = QueryType::NormalQuery;
			prefetchRows   = DEFAULT_PREFETCH_ROWS;
			bindThread     = 0;
			allThreadQuery = false;
		}
	};

	struct Result
	{
		int   ErrorCode()      { return error;        }
		int   SelectRowCount() { return rowCount;     }
		int   SelectRowSize()  { return rowSize;      }
		int   PrefetchRows()   { return prefetchRows; }
		int64 DataEffected()   { return effected;     }

		template<typename T>
		T* CastSelctData() { return reinterpret_cast<T*>(data); }

		int error; // 0:no error
		QueryType queryType;

		// Execute
		int64 effected;

		// Select
		int   rowCount;
		int   rowSize;
		void* data;
		void* fields;
		int   prefetchRows; // number of rows per one COM_FETCH
	};

	struct ConnectParams
	{
		std::string  host;
		std::string  user;
		std::string  password;
		std::string  db;
		int          port;
		size_t       workerNum;
	};

	class Accessor
	{
	public:
		Accessor();
		~Accessor();

		bool Init(const ConnectParams& params);
		void Release();
		void PostQuery(const Query& query);

		static ulong EscapeString(char *to, const char* from, unsigned long length);
		static std::string EscapeString(const char* from, ulong len);

		// get bind worker thread
		// return: failed:0
		uint GetWorkerThread();

	private:
		Accessor(const Accessor&) = delete;
		Accessor& operator=(const Accessor&) = delete;

	private:
		struct Core;
		std::shared_ptr<Core> mCore;
	};
};

#endif