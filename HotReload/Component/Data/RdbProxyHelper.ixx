export module RdbProxyHelper;

import RdbProxy;
import FuncUtils;
import Logger;
import StrUtils;

export class RdbProxyHelper : public Helper<RdbProxyHelper, RdbProxy>
{
private:

	RdbProxyHelper() = delete;
	~RdbProxyHelper() = default;

public:

	[[nodiscard]]
	std::unique_ptr<pqxx::transaction_base> GetTransaction(EMSqlDbNameEnum dbName, bool isReadOnly = true)
	{
		std::unique_ptr<pqxx::transaction_base> transaction;

		const auto& connection = GetConnection(dbName);
		if(!connection)
		{
			return nullptr;
		}

		int maxTryCount = 3;

		bool isReconnection = false;

		do
		{
			try
			{
				if(maxTryCount == 0)
				{
					return nullptr;
				}

				if(isReconnection)
				{
					--maxTryCount;

					auto newConnection = P_InstanceHolder->GetMemPool().Allocate<pqxx::connection>(connection->connection_string());
					connection->close();
					const_cast<std::shared_ptr<pqxx::connection>&>(connection).swap(newConnection);
				}

				if(isReadOnly)
				{
					transaction = std::make_unique<pqxx::read_transaction>(*connection);
				}
				else
				{
					transaction = std::make_unique<pqxx::work>(*connection);
				}

				break;
			}
			catch(pqxx::broken_connection& e)
			{
				// lost connection. reconnect
				LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Can Connect 'R' Database:{}, retest {}", EnumName(dbName), e.what());
				isReconnection = true;
			}
			catch(const std::exception& e)
			{
				LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Can Connect 'R' Database:{} broke, retest {}", EnumName(dbName), e.what());
				break;
			}
		}
		while(true);

		return transaction;
	}

	
private:

	const std::shared_ptr<pqxx::connection>& GetConnection(EMSqlDbNameEnum dbName)
	{
		static std::shared_ptr<pqxx::connection> None;
		if (pRdbProxys.contains(dbName))
		{
			return pRdbProxys[dbName];
		}
		
		return None;
	}
};
