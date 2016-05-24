#ifndef __DATABASE_SQL_BUILDER_HEADER__
#define __DATABASE_SQL_BUILDER_HEADER__

#include <string>
#include <sstream>
#include <type_traits>

namespace mysql
{
	// SQLBuilder, not orm
	class SQLBuilder
	{
	public:
		SQLBuilder() { reset(); }
		~SQLBuilder() {}

		void reset()
		{
			ss.str("");
			print_VALUES = false;
			print_END = false;
			first_CALLPARA = false;
		}

		std::string str()
		{
			if (!print_END)
			{
				print_END = true;
				ss << ";";
			}
			return ss.str();
		}

	private:
		SQLBuilder& PutVal(const char* v) { ss << "'" << v << "'"; return *this; }
		SQLBuilder& PutVal(const std::string& v) { ss << "'" << v << "'"; return *this; }
		SQLBuilder& PutVal(const signed char& v) { ss << (int)v; return *this; }
		SQLBuilder& PutVal(const unsigned char& v) { ss << (int)v; return *this; }
		SQLBuilder& PutVal(const signed short& v) { ss << v; return *this; }
		SQLBuilder& PutVal(const unsigned short& v) { ss << v; return *this; }
		SQLBuilder& PutVal(const signed int& v) { ss << v; return *this; }
		SQLBuilder& PutVal(const unsigned int& v) { ss << v; return *this; }
		SQLBuilder& PutVal(const signed long long& v) { ss << v; return *this; }
		SQLBuilder& PutVal(const unsigned long long& v) { ss << v; return *this; }
		SQLBuilder& PutVal(const float& v) { ss << v; return *this; }
		SQLBuilder& PutVal(const double& v) { ss << v; return *this; }
		SQLBuilder& PutVal(const long double& v) { ss << v; return *this; }

		template <typename T>
		typename std::enable_if<std::is_class<T>::value, SQLBuilder>::type& PutVal(const T& v)
		{
			static_assert(std::alignment_of<T>::value == 1, "align != 1");
			std::vector<char> buf(sizeof(v) * 2 + 1);
			int len = (int)db_accessor::EscapeString(&buf[0], (char*)&v, sizeof(v));
			ss << "'" << std::string(&buf[0], len) << "'";
			return *this;
		}

		template <typename T0, typename... T>
		SQLBuilder& AppendSelectFields(const T0& t0, T... t)
		{
			ss << "," << t0;
			return InsertReplaceFields(t...);
		}

		SQLBuilder& AppendSelectFields()
		{
			return *this;
		}

		template <typename T0, typename... T>
		SQLBuilder& AppendFrom(const T0& t0, T... t)
		{
			ss << "," << t0;
			return AppendFrom(t...);
		}

		SQLBuilder& AppendFrom()
		{
			return *this;
		}

		template <typename T0, typename... T>
		SQLBuilder& InsertReplaceFields(const T0& t0, T... t)
		{
			ss << t0;
			return InsertReplaceFields(t...);
		}

		SQLBuilder& InsertReplaceFields()
		{
			ss << ") VALUES";
			return *this;
		}

		template <typename T0, typename... T>
		SQLBuilder& AppendFields(const T0& t0, T... t)
		{
			ss << "," << t0;
			return AppendFields(t...);
		}

		SQLBuilder& AppendFields()
		{
			ss << ")";
			return *this;
		}

		template <typename T0, typename... T>
		SQLBuilder& AppendValues(const T0& t0, T... t)
		{
			ss << ",";
			PutVal(t0);
			return AppendValues(t...);
		}

		SQLBuilder& AppendValues()
		{
			ss << ")";
			return *this;
		}

		template <typename T0, typename... T>
		SQLBuilder& AppendGroupOrderBy(const T0& t0, T... t)
		{
			ss << "," << t0;
			return AppendGroupOrderBy(t...);
		}

		SQLBuilder& AppendGroupOrderBy()
		{
			return *this;
		}

		template <typename T0, typename... T>
		SQLBuilder& AppendIn(const T0& t0, T... t)
		{
			ss << ",";
			PutVal(t0);
			return AppendIn(t...);
		}

		SQLBuilder& AppendIn()
		{
			ss << ")";
			return *this;
		}

		template <typename T0, typename... T>
		SQLBuilder& AppendSet(const std::string& field, const T0& t0, T... t)
		{
			ss << "," << field << "=";
			PutVal(t0);
			return AppendSet(t...);
		}

		SQLBuilder& AppendSet()
		{
			return *this;
		}

		template <typename T0, typename... T>
		SQLBuilder& AppendCall(const T0& t0, T... t)
		{
			if (!first_CALLPARA) ss << ",";
			else  first_CALLPARA = false;
			PutVal(t0);
			return AppendCall(t...);
		}

		SQLBuilder& AppendCall()
		{
			ss << ")";
			return *this;
		}

	public:
		template <typename T0, typename... T>
		SQLBuilder& Select(const T0& t0, T... t)
		{
			reset();
			ss << "SELECT " << t0;
			return AppendSelectFields(t...);
		}

		template <typename T0, typename... T>
		SQLBuilder& From(const T0& t0, T... t)
		{
			ss << " FROM " << t0;
			return AppendFrom(t...);
		}

		SQLBuilder& InsertInto(const std::string& table)
		{
			reset();
			ss << "INSERT INTO " << table;
			return *this;
		}

		SQLBuilder& InsertIgnore(const std::string& table)
		{
			reset();
			ss << "INSERT IGNORE " << table;
			return *this;
		}

		SQLBuilder& ReplaceInto(const std::string& table)
		{
			reset();
			ss << "REPLACE INTO " << table;
			return *this;
		}

		template <typename T0, typename... T>
		SQLBuilder& Fields(const T0& t0, T... t)
		{
			ss << "(" << t0;
			return AppendFields(t...);
		}

		template <typename T0, typename... T>
		SQLBuilder& Values(const T0& t0, T... t)
		{
			if (!print_VALUES) { print_VALUES = true; ss << " VALUES("; }
			else { ss << ",("; }
			PutVal(t0);
			return AppendValues(t...);
		}

		SQLBuilder& Update(const std::string& table)
		{
			reset();
			ss << "UPDATE " << table;
			return *this;
		}

		template <typename T0, typename... T>
		SQLBuilder& Set(const std::string& field, const T0& val, T... t)
		{
			ss << " SET " << field << "=";
			PutVal(val);
			return AppendSet(t...);
		}

		SQLBuilder& Delete(const std::string& table)
		{
			reset();
			ss << "DELETE FROM " << table;
			return *this;
		}

		SQLBuilder& Where(const std::string& field) { ss << " WHERE " << field; return *this; }
		SQLBuilder& And(const std::string& field) { ss << " AND " << field; return *this; }
		SQLBuilder& Or(const std::string& field) { ss << " OR " << field; return *this; }
		SQLBuilder& Where() { ss << " WHERE"; return *this; }
		SQLBuilder& And() { ss << " AND"; return *this; }
		SQLBuilder& Or() { ss << " OR"; return *this; }

		template <typename Container>
		SQLBuilder& In(const Container& vals)
		{
			ss << " IN(";
			int n = 0;
			for (auto& v : vals)
			{
				if (n != 0) ss << ",";
				++n;
				PutVal(v);
			}
			ss << ")";
			return *this;
		}

		template <typename T0, typename... T>
		SQLBuilder& In(const T0& t0, T... t)
		{
			ss << " IN(";
			PutVal(t0);
			return AppendIn(t...);
		}

		SQLBuilder& In(SQLBuilder& selectSql)
		{
			ss << " IN(" << selectSql.str() << ")";
			return *this;
		}

		template <typename T0, typename... T>
		SQLBuilder& GroupBy(const T0& t0, T... t)
		{
			ss << " GROUP BY " << t0;
			return AppendGroupOrderBy(t...);
		}

		template <typename T0, typename... T>
		SQLBuilder& OrderBy(const T0& t0, T... t)
		{
			ss << " ORDER BY " << t0;
			return AppendGroupOrderBy(t...);
		}

		SQLBuilder& Limit(int l1)
		{
			ss << " LIMIT " << l1;
		}

		SQLBuilder& Limit(int l1, int l2)
		{
			ss << " LIMIT " << l1 << "," << l2;
		}

		template<typename T> SQLBuilder& Equal(const T& v) { ss << "="; PutVal(v); return *this; }
		template<typename T> SQLBuilder& NotEqual(const T& v) { ss << "!="; PutVal(v); return *this; }
		template<typename T> SQLBuilder& Less(const T& v) { ss << "<"; PutVal(v); return *this; }
		template<typename T> SQLBuilder& LessEqual(const T& v) { ss << "<="; PutVal(v); return *this; }
		template<typename T> SQLBuilder& Bigger(const T& v) { ss << ">="; PutVal(v); return *this; }
		template<typename T> SQLBuilder& BiggerEqual(const T& v) { ss << ">="; PutVal(v); return *this; }
		template<typename T> SQLBuilder& IsNull(const T& t) { ss << " ISNULL(" << t << ")"; return *this; }
		SQLBuilder& IsNull() { ss << " IS NULL"; return *this; }
		SQLBuilder& IsNotNull() { ss << " IS NOT NULL"; return *this; }

		template <typename T>
		SQLBuilder& IfNull(const std::string& field, const T& v)
		{
			ss << " IFNULL(" << field << ",";
			PutVal(v);
			return *this;
		}

		template <typename... T>
		SQLBuilder& Call(const std::string& p, T... t)
		{
			reset();
			ss << "CALL " << p << "(";
			first_CALLPARA = true;
			return AppendCall(t...);
		}

	private:
		std::stringstream ss;
		bool print_VALUES;
		bool print_END;
		bool first_CALLPARA;
	};
}

#endif