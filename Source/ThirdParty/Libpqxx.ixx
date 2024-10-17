module;
#include "pqxx/transaction"
#include "pqxx/connection"
#include "pqxx/nontransaction"
#include "pqxx/result"
export module ThirdParty.Libpqxx;

using namespace pqxx;

export 
{
	using pq_connection = ::connection;
	using pq_field = ::field;
	using ::dbtransaction;
	using pq_result = result;
	using pq_row = row;
	using ::nontransaction;
	using pq_work = work;
	using ::read_transaction;
};