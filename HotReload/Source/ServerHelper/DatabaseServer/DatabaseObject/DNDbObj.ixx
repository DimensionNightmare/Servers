module;
#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"
#include "google/protobuf/descriptor.pb.h"
#include "DbExtend.pb.h"
#include "pqxx/transaction"
#include "pqxx/result"

#include <sstream>
#include <format>
export module DNDbObj;

import AfxCommon;

using namespace std;
using namespace google::protobuf;

#define DNPrint(code, level, fmt, ...) LoggerPrint(level, code, __FUNCTION__, fmt, ##__VA_ARGS__);

//statement
#define SSMBegin "(" 
#define SSMEnd ")"
#define SEnd ";"
#define SSplit ","
#define SSpace " "
#define SDefault "DEFAULT"
#define SCombo " AND "
#define SPrimaryKey "PRIMARY KEY"

enum class SqlOpType : char
{
	None,
	CreateTable,
	Insert,
	Query,
	Update,
	Delete,
};

const char* GetOpTypeBySqlOpType(SqlOpType eType)
{
	switch(eType)
	{
#define ONE(type, name) case type: return name;
		ONE(SqlOpType::CreateTable,	"CREATE TABLE "		)
		ONE(SqlOpType::Insert,		"INSERT INTO "		)
		ONE(SqlOpType::Query,		"SELECT "	)
		ONE(SqlOpType::Update,		"UPDATE "	)
		ONE(SqlOpType::Delete,		"DELETE FROM "	)
#undef ONE
	}

	return "";
}

const char* GetDbTypeByProtoType(FieldDescriptor::CppType pbType)
{
	// http://postgres.cn/docs/9.4/datatype-numeric.html#DATATYPE-INT
	switch(pbType)
	{
#define ONE(type, name) case type: return #name;
		ONE(FieldDescriptor::CPPTYPE_DOUBLE,		DOUBLE	)
		ONE(FieldDescriptor::CPPTYPE_FLOAT,			FLOAT	)
		ONE(FieldDescriptor::CPPTYPE_INT32,			INTEGER	)
		ONE(FieldDescriptor::CPPTYPE_UINT32,		INTEGER	)
		ONE(FieldDescriptor::CPPTYPE_INT64,			BIGINT	)
		ONE(FieldDescriptor::CPPTYPE_UINT64,		BIGINT	)
		ONE(FieldDescriptor::CPPTYPE_STRING, 		TEXT	)
		ONE(FieldDescriptor::CPPTYPE_ENUM, 			INTEGER	)
#undef ONE
		default:
			throw new exception("Please Regist DbType");
			break;
	}
	
	return "";
}

void InitFieldByProtoType(const FieldDescriptor* field, list<string>& out, list<string>& keys)
{
	const char* typeStr = GetDbTypeByProtoType(field->cpp_type());

	out.emplace_back(typeStr);

	if(!field->has_optional_keyword())
	{
		out.emplace_back("NOT NULL");
	}

	if(field->has_default_value())
	{
		out.emplace_back(format("{0} {1}", SDefault, field->default_value_string()));
	}

	if(field->options().GetExtension(primary_key))
	{
		keys.emplace_back(field->name());
	}

	auto res1 = field->options().GetExtension(len_limit);

	switch(field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_UINT32:
		case FieldDescriptor::CPPTYPE_UINT64:
			out.emplace_back(format("CHECK ({0} > -1)", field->name()));
			break;

	}
}

void InsertFieldByProtoType(const FieldDescriptor* field, const Reflection* reflection, Message& data, string& out)
{
	out.clear();

	const char* typeStr = GetDbTypeByProtoType(field->cpp_type());

	switch(field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_INT32:
			out = format("{}", reflection->GetInt32(data, field));
			break;
		case FieldDescriptor::CPPTYPE_UINT32:
			out = format("{}", reflection->GetUInt32(data, field));
			break;
		case FieldDescriptor::CPPTYPE_INT64:
			out = format("{}", reflection->GetInt64(data, field));
			break;
		case FieldDescriptor::CPPTYPE_UINT64:
			out = format("{}", reflection->GetUInt64(data, field));
			break;
		case FieldDescriptor::CPPTYPE_DOUBLE:
			out = format("{}", reflection->GetDouble(data, field));
			break;
		case FieldDescriptor::CPPTYPE_FLOAT:
			out = format("{}", reflection->GetFloat(data, field));
			break;
		case FieldDescriptor::CPPTYPE_STRING:
			out = format("'{}'", reflection->GetString(data, field));
			break;
		default:
			throw new exception("Please Regist InsertField::Type");
			break;
	}
	
}

void QueryFieldByProtoType(const FieldDescriptor* field, const Reflection* reflection, Message& data, pqxx::row::reference value)
{
	string typeStr = GetDbTypeByProtoType(field->cpp_type());

	switch(field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_INT32:
			reflection->SetInt32(&data, field, value.as<int32_t>());
			break;
		case FieldDescriptor::CPPTYPE_UINT32:
			reflection->SetUInt32(&data, field, value.as<uint32_t>());
			break;
		case FieldDescriptor::CPPTYPE_INT64:
			reflection->SetInt64(&data, field, value.as<int64_t>());
			break;
		case FieldDescriptor::CPPTYPE_UINT64:
			reflection->SetUInt64(&data, field, value.as<uint64_t>());
			break;
		case FieldDescriptor::CPPTYPE_DOUBLE:
			reflection->SetDouble(&data, field, value.as<double>());
			break;
		case FieldDescriptor::CPPTYPE_FLOAT:
			reflection->SetFloat(&data, field, value.as<float>());
			break;
		case FieldDescriptor::CPPTYPE_STRING:
			reflection->SetString(&data, field, value.as<string>());
			break;
		default:
			throw new exception("Please Regist InsertField::Type");
			break;
	}
	
}

export template <class TMessage = Message>
class DNDbObj
{
public:
	DNDbObj();
	DNDbObj(pqxx::transaction<>& work);
	~DNDbObj(){};

	const string& GetName(){return pMessage->GetDescriptor()->name();};
	
	bool Commit();
	//create table
	DNDbObj<TMessage>& InitTable();
	//insert
	DNDbObj<TMessage>& AddRecord(list<TMessage>& datas);
	//query
	DNDbObj<TMessage>& Query(int index = -2, int condIndex = -1, const char* condition = nullptr);

	vector<TMessage> Result(){ return mResult;}

	DNDbObj<TMessage>& Update(TMessage& updata, int condIndex = -1);

	DNDbObj<TMessage>& Delete(TMessage& updata, int condIndex = -1);

	bool IsSuccess(){return bExecResult;}

private:
	bool ChangeSqlType(SqlOpType type);

	void SetResult(int affectedRows);

	void BuildSqlStatement();

	DNDbObj<TMessage>& AddRecord(TMessage& data);

	void PaserQuery(pqxx::result& result);

private:
	TMessage* pMessage;

	vector<TMessage> mResult;

	SqlOpType eType;
	// create table, instert
	map<string, list<string>> mEles;

	pqxx::transaction<>* pWork;

	string sSqlStatement;

	bool bExecResult;
	int iExecResultCount;
};

module:private;

template <class TMessage>
DNDbObj<TMessage>::DNDbObj()
{
	pMessage = nullptr;
	mResult.clear();
	eType = SqlOpType::None;
	mEles.clear();
	pWork = nullptr;
	sSqlStatement.clear();
	bExecResult = false;
	iExecResultCount = 0;
}

template <class TMessage>
DNDbObj<TMessage>::DNDbObj(pqxx::transaction<> &work):DNDbObj()
{
	pWork = &work;
}

template <class TMessage>
bool DNDbObj<TMessage>::ChangeSqlType(SqlOpType type)
{
	if(eType != SqlOpType::None && eType != type)
	{
		throw new exception("Not Commit to Change OpType");
		return false;
	}

	eType = type;
	bExecResult = false;
	iExecResultCount = 0;
	return true;
}

template <class TMessage>
void DNDbObj<TMessage>::SetResult(int affectedRows)
{
	switch(eType)
	{
		case SqlOpType::Update:
		case SqlOpType::Delete:
		case SqlOpType::Query:
		{
			bExecResult = affectedRows > 0;
			break;
		}
		case SqlOpType::Insert:
		case SqlOpType::CreateTable:
		{
			bExecResult = affectedRows == iExecResultCount;
			break;
		}
		default:
			throw new exception("Please Imp SetResult Case!");
	}
	
	iExecResultCount = 0;
	eType = SqlOpType::None;
	mEles.clear();
}

template <class TMessage>
void DNDbObj<TMessage>::BuildSqlStatement()
{
	if(eType == SqlOpType::None)
	{
		return;
	}

	stringstream ss;
	
	switch(eType)
	{
		case SqlOpType::CreateTable:
		{
			ss << GetOpTypeBySqlOpType(eType) << GetName();

			if(!mEles.size())
			{
				// data null
				return;
			}

			ss << SSMBegin;
			auto it = mEles.begin();
			auto itEnd = mEles.end();
			for(;it != itEnd;it++)
			{
				ss << it->first;
				for(string& props : it->second)
				{
					ss << SSpace << props;
				}

				if(next(it) != itEnd)
				{
					ss << SSplit;
				}
				else
				{
					ss << SSMEnd;
				}
			}
			ss << SEnd;

			iExecResultCount = 1;
			break;
		}
		case SqlOpType::Insert:
		{
			ss << GetOpTypeBySqlOpType(eType) << GetName();

			if(!mEles.size())
			{
				// key null
				return;
			}

			size_t dataLen = mEles.begin()->second.size();
			if(!dataLen)
			{
				// value null
				return;
			}

			ss << SSMBegin;
			auto it = mEles.begin();
			auto itEnd = mEles.end();
			vector<list<string>> mapping;
			for(;it != itEnd;it++)
			{
				ss << it->first;

				mapping.push_back(it->second);

				if(next(it) != itEnd)
				{
					ss << SSplit;
				}
				else
				{
					ss << SSMEnd;
				}
			}

			ss << "VALUES";

			size_t mappingSize = mapping.size() - 1;
			for(int lst = 0;lst < dataLen; lst++)
			{
				ss << SSMBegin;
				for(int pos = 0; pos < mapping.size(); pos ++)
				{
					ss << mapping[pos].front();
					mapping[pos].pop_front();
					if(mappingSize != pos)
					{
						ss << SSplit;
					}
				}
				ss << SSMEnd;

				if(lst + 1 != dataLen)
				{
					ss << SSplit;
				}

				iExecResultCount = iExecResultCount + 1;
			}
			ss << SEnd;
			break;
		}
		case SqlOpType::Query:
		{
			string selectElems;
			auto it = mEles.begin();
			auto itEnd = mEles.end();
			for (;it != itEnd;it++)
			{
				selectElems.append(it->first);
				if(next(it) != itEnd)
				{
					selectElems += SSplit;
				}
			}

			ss << GetOpTypeBySqlOpType(eType) << selectElems << " FROM " << GetName();
			
			bool hasCondition = false;

			for (it = mEles.begin();it != itEnd;it++)
			{
				if(!hasCondition)
				{
					hasCondition = true;
					ss << " WHERE ";
				}
				
				for(string& cond : it->second)
				{
					ss << cond;
				}
			}

			ss << SEnd;
			break;
		}
		case SqlOpType::Update:
		{
			string selectElems;
			auto it = mEles.begin();
			auto itEnd = mEles.end();
			for (;it != itEnd;it++)
			{
				selectElems.append(format("{} = {}", it->first, it->second.front()));

				if(next(it) != itEnd)
				{
					selectElems += SSplit;
				}
			}

			ss << GetOpTypeBySqlOpType(eType) << GetName() << " SET "  << selectElems ;
			
			bool hasCondition = false;

			for (it = mEles.begin();it != itEnd;it++)
			{

				for(auto condIt = it->second.begin(); condIt != it->second.end(); condIt++)
				{
					if(condIt == it->second.begin())
					{
						continue;
					}

					if(!hasCondition)
					{
						hasCondition = true;
						ss << " WHERE ";
					}

					ss << *condIt;
				}
			}

			ss << SEnd;
			break;
		}
		case SqlOpType::Delete:
		{
			ss << GetOpTypeBySqlOpType(eType) << GetName();
			
			if(mEles.size())
			{
				ss << " WHERE ";
				string selectElems;
				auto it = mEles.begin();
				auto itEnd = mEles.end();
				for (;it != itEnd;it++)
				{
					selectElems.append(format("{} = {}", it->first, it->second.front()));

					if(next(it) != itEnd)
					{
						selectElems += SCombo;
					}
				}

				ss << selectElems;
			}


			ss << SEnd;
			break;
		}
		default:
			throw new exception("Please Imp BuildSqlStatement Case!");
	}

	sSqlStatement = ss.str();
}

template <class TMessage>
bool DNDbObj<TMessage>::Commit()
{
	BuildSqlStatement();

	if(!pWork || sSqlStatement.empty())
	{
		return false;
	}

	DNPrint(-1, LoggerLevel::Debug, "%s \n\n", sSqlStatement.c_str());

	pqxx::result result = pWork->exec(sSqlStatement);

	if(eType == SqlOpType::Query)
	{
		PaserQuery(result);
	}

	SetResult(result.affected_rows());

	return true;
}

template <class TMessage>
DNDbObj<TMessage>& DNDbObj<TMessage>::InitTable()
{
	ChangeSqlType(SqlOpType::CreateTable);

	const Descriptor* descriptor = this->pMessage->GetDescriptor();

	list<string> primaryKey;

	for(int i = 0; i < descriptor->field_count();i++)
	{
		const FieldDescriptor* field = descriptor->field(i);
		list<string> params;
		InitFieldByProtoType(field, params, primaryKey);

		//unregist type
		if(params.size() == 0)
		{
			continue;
		} 

		mEles.emplace(make_pair(field->name(), params));
	}

	if (primaryKey.size())
	{
		string temp = SSMBegin;
		for (auto it = primaryKey.begin(); it != primaryKey.end(); it++)
		{
			temp += *it;

			if(next(it) != primaryKey.end())
			{
				temp += SSplit;
			}
		}

		temp += SSMEnd;

		primaryKey.clear();
		primaryKey.emplace_back(temp);
		
		mEles.emplace(make_pair(SPrimaryKey, primaryKey));
	}
	

	return *this;
}

template <class TMessage>
DNDbObj<TMessage>& DNDbObj<TMessage>::AddRecord(list<TMessage>& datas)
{
	ChangeSqlType(SqlOpType::Insert);

	if(datas.size())
	{
		for(TMessage& one : datas)
		{
			AddRecord(one);
		}
	}

	return *this;
}

template <class TMessage>
DNDbObj<TMessage>& DNDbObj<TMessage>::AddRecord(TMessage& data)
{
	const Descriptor* descriptor = this->pMessage->GetDescriptor();
	const Reflection* reflection = this->pMessage->GetReflection();

	string params;

	for(int i = 0; i < descriptor->field_count();i++)
	{
		const FieldDescriptor* field = descriptor->field(i);
		
		InsertFieldByProtoType(field, reflection, data, params);

		if(! params.size())
		{
			params = SDefault;
		}
		mEles[field->name()].emplace_back(params);
	}

	return *this;
}

template <class TMessage>
DNDbObj<TMessage> &DNDbObj<TMessage>::Query(int index, int condIndex, const char* condition)
{
	ChangeSqlType(SqlOpType::Query);

	const Descriptor* descriptor = this->pMessage->GetDescriptor();
	const Reflection* reflection = this->pMessage->GetReflection();

	// range *=-1
	if(index < -2 || index > descriptor->field_count())
	{
		return *this;
	}
	
	bool isExistAll = mEles.count("*");

	// query * not need other
	if(isExistAll && index > -1)
	{
		return *this;
	}

	if(index == -1)
	{
		mEles["*"];

		if(mEles.size() > 1)
		{
			throw new exception("exist other select statement!!");
		}

		isExistAll = true;
	}
	else if(index > 0)
	{
		if(isExistAll)
		{
			throw new exception("exist * select statement!!");
		}

		const FieldDescriptor* field = descriptor->field(index);
		mEles[field->name()];
	}

	if(condIndex > -1 && condition)
	{
		const FieldDescriptor* field = descriptor->field(condIndex);
		string name = field->name();

		string condStr;

		condStr.resize(strlen(condition) - 2 + name.size());
		sprintf_s(condStr.data(), condStr.size() +1, condition, name.c_str());

		if(isExistAll)
		{
			mEles["*"].emplace_back(condStr);
		}
		else
		{
			mEles[name].emplace_back(condStr);
		}
	}

	return *this;
}

template <class TMessage>
void DNDbObj<TMessage>::PaserQuery(pqxx::result &result)
{
	mResult.clear();

	vector<string> keys;
	for (int col = 0; col < result.columns(); ++col)
	{
		keys.push_back(result.column_name(col));
	}

	const Descriptor* descriptor = this->pMessage->GetDescriptor();
	const Reflection* reflection = this->pMessage->GetReflection();

	for(int row = 0;row < result.size(); row++)
	{
		TMessage gen;

		pqxx::row rowInfo = result[row];
		for(int col = 0; col < rowInfo.size(); col++)
		{
			QueryFieldByProtoType(descriptor->FindFieldByLowercaseName(keys[col]), reflection, gen, rowInfo[col]);
		}
		mResult.push_back(gen);
	}
}

template <class TMessage>
DNDbObj<TMessage> &DNDbObj<TMessage>::Update(TMessage& updata, int condIndex)
{
	ChangeSqlType(SqlOpType::Update);

	const Descriptor* descriptor = this->pMessage->GetDescriptor();
	const Reflection* reflection = this->pMessage->GetReflection();

	string sData;

	for(int i = 0; i < descriptor->field_count(); i++)
	{
		bool isKey = condIndex == i;
		const FieldDescriptor* field = descriptor->field(i);
		if (!reflection->HasField(updata, field)) 
		{
			if(isKey)
			{
				throw new exception("update key not set");
			}
			continue;
        } 

		InsertFieldByProtoType(field, reflection, updata, sData);
        mEles[field->name()].emplace_back(sData);

		if(isKey)
		{
			mEles.begin()->second.emplace_back( format("{} = {}", field->name(), sData));
		}
	}

	return *this;
}

template <class TMessage>
DNDbObj<TMessage> &DNDbObj<TMessage>::Delete(TMessage &updata, int condIndex)
{
	ChangeSqlType(SqlOpType::Delete);

	const Descriptor* descriptor = this->pMessage->GetDescriptor();
	const Reflection* reflection = this->pMessage->GetReflection();

	string sData;

	for(int i = 0; i < descriptor->field_count(); i++)
	{
		bool isKey = condIndex == i;
		const FieldDescriptor* field = descriptor->field(i);
		if (!reflection->HasField(updata, field)) 
		{
			if(isKey)
			{
				throw new exception("Delete key not set");
			}
			continue;
        } 

		InsertFieldByProtoType(field, reflection, updata, sData);
        mEles[field->name()].emplace_back(sData);

		if(isKey)
		{
			mEles.begin()->second.emplace_back( format("{} = {}", field->name(), sData));
		}
	}

	return *this;
}
