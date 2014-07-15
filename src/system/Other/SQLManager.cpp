#include "SQLManager.h"
#include "uox3.h"

struct isEqual
{
    isEqual(std::string targetStr, unsigned int targetNum) : _targetStr(targetStr), _targetNum(targetNum) {};
    bool operator () (std::pair<std::pair<std::string, int>, std::string>& obj)
	{
		return obj.first.first.compare(_targetStr) == 0 && obj.first.second == _targetNum;
    };
    std::string _targetStr;
    unsigned int _targetNum;
};

namespace UOX
{
	const int ALLOC_NONE	= 0;
	const int ALLOC_ENV		= 1;
	const int ALLOC_DBC		= 2;
	const int ALLOC_CONNECT = 3;
	const int ALLOC_STMT	= 4;

    //---------------------------------------------------------------------------------------------
    template<> SQLManager * Singleton< SQLManager >::ms_Singleton = 0;
    SQLManager* SQLManager::getSingletonPtr( void )
    {
        return ms_Singleton;
    }
    SQLManager& SQLManager::getSingleton( void )
    {  
        assert( ms_Singleton );  return ( *ms_Singleton );  
    }
    //---------------------------------------------------------------------------------------------

	SQLManager::SQLManager() : inTransaction(false), ExecuteOrder(0)  {}
	SQLManager::~SQLManager()
	{
		Disconnect();
	}

	bool SQLManager::SetAddress( std::string newVal )
	{
		address	= newVal;
		lastState	= true;
		return lastState;
	}

	bool SQLManager::SetDatabase( std::string newVal )
	{
		database	= newVal;
		lastState	= true;
		return lastState;
	}

	bool SQLManager::SetUsername( std::string newVal )
	{
		username	= newVal;
		lastState	= true;
		return lastState;
	}

	bool SQLManager::SetPassword( std::string newVal )
	{
		password	= newVal;
		lastState	= true;
		return lastState;
	}

	bool SQLManager::BeginTransaction(void)
	{
		mysql_query(conn, "START TRANSACTION");
		inTransaction = true;
		lastState = true;
		return lastState;
	}

	bool SQLManager::QueryRelease(bool transaction)
	{
		lastState = true;
		if (state == SQL_STATE_EXECUTEQUERY_NORETURN)
			return lastState;

		if(transaction && !inTransaction)
			FinaliseTransaction(true);

		if (state == SQL_STATE_EXECUTEQUERY_RETURN && state != SQL_STATE_FETCHROW)
			--ExecuteOrder;

		if (statementList.size() > 0)
			statementList.erase(statementList.begin()+ExecuteOrder);

		state = SQL_STATE_RELEASEQUERY;
		return lastState;
	}

	bool SQLManager::FinaliseTransaction(bool commit)
	{
		lastState = true;
		if(commit)
			mysql_query(conn, "COMMIT");
		else
			mysql_query(conn, "ROLLBACK");

		if(inTransaction)
			QueryRelease();
		inTransaction = false;
		return lastState;
	}

	bool SQLManager::Connect(void)
	{
		lastState = true;

		conn = mysql_init((MYSQL*) 0);
		bool success = false;
		if (mysql_real_connect(conn, address.c_str(), username.c_str(), password.c_str(), database.c_str(), 0, NULL, 0))
			success = true;

		if (!success)
		{
			lastState = false;
			Console.Warning("SQLManager: Connection failure. Error: %s", mysql_error(conn));
			Shutdown(FATAL_UOX3_MYSQL_CONNECTION_FAIL);
		}

		return lastState;
	}

	bool SQLManager::Disconnect(void)
	{
		lastState = true;
		mysql_free_result(res);
		mysql_close(conn);
		return lastState;
	}

	bool SQLManager::ExecuteQuery(std::string sql, int *index, bool transaction)
	{
		lastState = false;
		if (sql.empty())
			return lastState;
		if (mysql_query(conn, const_cast<char*>(sql.c_str())))
			Console.Warning("MySQL query error : %s\n", mysql_error(conn));

		/*if (state == SQLSTATE_EXECUTEQUERY_RETURN || state == SQLSTATE_FETCHROW || state == SQLSTATE_GETCOLUMN)
			FinaliseTransaction(true);*/

		res = mysql_use_result(conn);
		if (res != NULL)
		{
			if (transaction && !inTransaction)
			{
				QueryRelease();
				Console.Warning("SQLManager: Forced release of query");
			}

			lastState = true;
			std::vector<std::vector<std::string>> rowsV;
			while ((row = mysql_fetch_row(res)))
			{
				std::vector<std::string> columnsV;
				for (size_t i = 0; i < mysql_num_fields(res); ++i)
					columnsV.push_back(row[i] ? row[i] : "");
				if (!columnsV.empty())
					rowsV.push_back(columnsV);
			}

			if (!rowsV.empty())
			{
				statementList.push_back(std::make_pair(0, rowsV));
				state = SQL_STATE_EXECUTEQUERY_RETURN;
				++ExecuteOrder;
				if(index != NULL && ExecuteOrder-1 > -1)
					*index = statementList[ExecuteOrder-1].second.size();
			}
			else
				state = SQL_STATE_EXECUTEQUERY_NORETURN;
		}
		return lastState;
	}

	bool SQLManager::FetchRow(int *index)
	{
		lastState = false;
		if (ExecuteOrder-1 > -1)
		{
			statementList[ExecuteOrder-1].first = 1;
			if (*index > 0 && (size_t(*index) < statementList[ExecuteOrder-1].second.size()+1))
			{
				lastState = true;
				--*index;
			}
			else if (state != SQL_STATE_EXECUTEQUERY_NORETURN)
			{
				--ExecuteOrder;
				state = SQL_STATE_FETCHROW;
			}
		}
		return lastState;
	}

	bool SQLManager::GetColumn(int colNumber, UString& value, int *index)
	{
		lastState = false;
		bool NoFetch = false;
		if (statementList[ExecuteOrder-1].first == 0)
		{
			NoFetch = true;
			*index = 0;
			statementList[ExecuteOrder-1].first = 1;
		}

		if(size_t(*index) < statementList[ExecuteOrder-1].second.size()+1)
		{
			if (!statementList[ExecuteOrder-1].second[*index].empty() && !statementList[ExecuteOrder-1].second[*index][colNumber].empty())
			{
				lastState = true;
				value = statementList[ExecuteOrder-1].second[statementList[ExecuteOrder-1].second.size()-1-(*index)][colNumber];
			}
		}
		else
			Console.Warning("Invalid statement number %i (max %i)", *index, statementList[ExecuteOrder-1].second.size()+1);

		if (NoFetch && state != SQL_STATE_EXECUTEQUERY_NORETURN)
			--ExecuteOrder;
		
		state = SQL_STATE_GETCOLUMN;
		return lastState;
	}

	std::vector<std::string> SQLManager::Simplify(UString uStr, bool execute, int* index, bool transaction)
	{
		std::vector <std::pair<std::pair<std::string, int>, std::string> > Queries;
		std::istringstream issAll;
		issAll.str(uStr);
		uStr.clear();
		while (std::getline(issAll, uStr))
		{
			std::istringstream issSingle(uStr);
			int count = 0;
			std::string word;
			std::string table;
			std::string data;
			unsigned int type = 0;
			while (issSingle >> word)
			{
				++count;
				if (count == 1)
				{
					if (word.compare("INSERT") == 0)
						type = 1;
					else if (word.compare("REPLACE") == 0)
						type = 2;
					else
					{
						if (execute)
							SQLManager::getSingleton().ExecuteQuery(uStr, index, transaction);
						break;
					}
				}
				else
				{
					std::size_t first = word.find("`");
					if (first != std::string::npos)
					{
						std::size_t second = word.find("`", first+1, 1);
						if (second == std::string::npos)
							break;
                            
						table = word.substr(first+1, second-first-1);
					}
					else if (count == 3)
						table = word;
				}
            
				if (!table.empty())
				{
					std::size_t values = uStr.find("VALUES");
					if (values != std::string::npos)
					{
						data = uStr.substr(values+6, uStr.size()-values+6);
						while (data.at(data.size()-1) != ')')
							data.resize(data.size()-1);
					}

					if (!table.empty() && !data.empty())
					{
						auto itr = std::find_if(Queries.begin(), Queries.end(), isEqual(table, type));
						if (itr != Queries.end())
							itr->second += ","+data;
						else
							Queries.push_back(std::make_pair(std::make_pair(table, type), data));
					}

					break;
				}
			}
		}

		std::vector<std::string> eachTable;
		for (auto itr = Queries.begin(); itr != Queries.end(); ++itr)
		{
			std::string sql;
			switch (itr->first.second)
			{
			case 1:
				sql += "INSERT INTO `";
				break;
			case 2:
				sql += "REPLACE INTO `";
				break;
			}
			sql += itr->first.first+"` VALUES "+itr->second.c_str();
			eachTable.push_back(sql);
		}

		return eachTable;
	}

	bool SQLManager::SaveSettings( std::ofstream& target )
	{
		target << std::endl << "[MySQL]" << std::endl << "{" << std::endl;
		target << "MySQLDB=" << database << std::endl;
		target << "MySQLUSER=" << username << std::endl;
		target << "MySQLPASS=" << password << std::endl;
		target << "}" << std::endl;
		return true;
	}

	bool SQLManager::LastSucceeded( void )
	{
		return lastState;
	}
}