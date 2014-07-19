#ifndef __SQLManager_H__
#define __SQLManager_H__

#include "Prerequisites.h"
#include "Singleton.h"
#include "CDataList.h"
#include <sql.h>
#include <sqlext.h>
#include <mysql.h>

typedef std::vector< std::pair<int, std::vector<std::vector<std::string>>> > StatementType;

enum SQL_STATE
{
	SQL_STATE_EXECUTEQUERY_RETURN,
	SQL_STATE_EXECUTEQUERY_NORETURN,
	SQL_STATE_FETCHROW,
	SQL_STATE_GETCOLUMN,
	SQL_STATE_RELEASEQUERY
};

namespace UOX
{
	class SQLManager : public Singleton< SQLManager >
	{
	protected:
		MYSQL *conn;
		MYSQL_RES *res;
		MYSQL_ROW row;
		StatementType statementList;
		int ExecuteOrder;
		SQL_STATE state;
		bool lastState;
		bool inTransaction;
		std::string ip;
		unsigned int port;
		std::string	database;
		std::string	username;
		std::string	password;

	public:
		SQLManager();
		~SQLManager();

		void SetDatabaseInfo(UString dbinfo);

		std::string GetDatabase() const { return database; }

		bool Connect( void );
		bool Disconnect( void );
		bool ExecuteQuery(std::string sql, int *index = NULL, bool transaction = true);
		bool FetchRow( int *index );
		bool GetColumn(int colNumber, UString& value, int *index);
		bool QueryRelease(bool transaction = true);
		bool BeginTransaction( void );
		bool FinaliseTransaction( bool commit );
		bool LastSucceeded( void );
		std::vector<std::string> Simplify(UString uStr, bool execute = true, int* index = NULL, bool transaction = true);

		MYSQL_RES* GetMYSQLResult() { return res; }

        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static SQLManager& getSingleton( void );
        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static SQLManager * getSingletonPtr( void );
	};
}

#endif