module;
#include "pqxx/transaction"
#include "pqxx/connection"
#include "pqxx/nontransaction"
#include "pqxx/result"
export module ThirdParty.Libpqxx;


export 
{
	using namespace pqxx;
	
	using ::dbtransaction;
	using ::nontransaction;
	using ::read_transaction;
	
	using pq_connection = connection;
	using pq_field = field;
	using pq_result = result;
	using pq_row = row;
	using pq_work = work;
};