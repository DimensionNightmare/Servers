#include "pqxx/transaction"
#include "pqxx/connection"
#include "pqxx/nontransaction"
#include "pqxx/result"
export module ThirdParty.Libpqxx;


export namespace pqxx
{
	using pqxx::dbtransaction;
	using pqxx::nontransaction;
	using pqxx::read_transaction;
	
	using pqxx::connection;
	using pqxx::field;
	using pqxx::result;
	using pqxx::row;
	using pqxx::work;
}