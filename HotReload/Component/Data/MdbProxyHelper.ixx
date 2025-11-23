export module MdbProxyHelper;

import MdbProxy;
import FuncUtils;

export class MdbProxyHelper : public Helper<MdbProxyHelper, MdbProxy>
{
private:

	MdbProxyHelper() = delete;
	~MdbProxyHelper() = default;

public:

	[[nodiscard]]
	std::unique_ptr<sw::redis::Transaction> GetTransaction()
	{
		std::unique_ptr<sw::redis::Transaction> transaction;

		const auto& connection = GetConnection();
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

				}
				
				connection->ping();
				transaction = std::make_unique<sw::redis::Transaction>(connection->transaction(false, false));
				break;
			}
			catch(const sw::redis::IoError& e)
			{
				// lost connection. reconnect
				isReconnection = true;
			}
			catch(const std::exception& e)
			{
				break;
			}
		}
		while(true);

		return transaction;
	}


protected:

	std::shared_ptr<sw::redis::Redis> GetConnection()
	{
		if (pMdbProxys.contains(0))
		{
			return pMdbProxys[0];
		}

		return nullptr;
	}
	
};
