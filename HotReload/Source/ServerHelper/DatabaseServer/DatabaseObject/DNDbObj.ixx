module;
#include "google/protobuf/Message.h"
#include "google/protobuf/reflection.h"
#include "google/protobuf/util/json_util.h"
#include "pqxx/transaction"
#include "pqxx/result"

#include <sstream>
#include <format>
export module DNDbObj;

using namespace std;
using namespace google::protobuf;

//statement
#define SSMBegin "(" 
#define SSMEnd ")"
#define SEnd ";"
#define SSplit ","
#define SSpace " "
#define SDefault "DEFAULT"

enum class SqlOpType : char
{
	None,
	CreateTable,
	Insert,
	Query,
};

const char* GetOpTypeBySqlOpType(SqlOpType eType)
{
	switch(eType)
	{
#define ONE(type, name, exp) case type: return name exp;
		ONE(SqlOpType::CreateTable,"CREATE TABLE"," ")
		ONE(SqlOpType::Insert,"INSERT INTO"," ")
		ONE(SqlOpType::Query,"SELECT %s FROM"," ")
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
			// printf("Please Regist Descriptor::Type %d \n\n", pbType);
			break;
	}
	
	return "";
}

void InitFieldByProtoType(const FieldDescriptor* field, list<string>& out)
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

private:
	bool ChangeSqlType(SqlOpType type);

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
	return true;
}

template <class TMessage>
void DNDbObj<TMessage>::BuildSqlStatement()
{
	if(eType == SqlOpType::None)
		return;

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
					ss << SSplit;
				else
					ss << SSMEnd;
			}
			ss << SEnd;

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
					ss << SSplit;
				else
					ss << SSMEnd;
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
						ss << SSplit;
				}
				ss << SSMEnd;

				if(lst + 1 != dataLen)
					ss << SSplit;
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
					selectElems += SSplit;
			}

			const char* fmtStr = GetOpTypeBySqlOpType(eType);
			static size_t fmtSLen = strlen(fmtStr) - 2; // delete %s
			
			string tempStr = selectElems;
			tempStr.resize(fmtSLen + tempStr.size());
		
			sprintf_s(tempStr.data(), tempStr.size() + 1, fmtStr, selectElems.c_str()); //size(), add '\0'

			ss << tempStr << GetName();

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
	}

	sSqlStatement = ss.str();
}

template <class TMessage>
bool DNDbObj<TMessage>::Commit()
{
	BuildSqlStatement();

	if(!pWork || sSqlStatement.empty())
		return false;

	printf("%s \n\n", sSqlStatement.c_str());

	switch(eType)
	{
		case SqlOpType::CreateTable:
		case SqlOpType::Insert:
	 		pWork->exec0(sSqlStatement);
			break;
		case SqlOpType::Query:
	 		pqxx::result result = pWork->exec(sSqlStatement);
			PaserQuery(result);
			break;
	}

	eType = SqlOpType::None;
	mEles.clear();

	return true;
}

template <class TMessage>
DNDbObj<TMessage>& DNDbObj<TMessage>::InitTable()
{
	ChangeSqlType(SqlOpType::CreateTable);

	const Descriptor* descriptor = this->pMessage->GetDescriptor();

	for(int i = 0; i < descriptor->field_count();i++)
	{
		FieldDescriptor* field = descriptor->field(i);
		list<string> params;
		InitFieldByProtoType(field, params);

		//unregist type
		if(params.size() == 0) 
			continue;

		mEles.emplace(make_pair(field->name(), params));
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
			AddRecord(one);
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
		FieldDescriptor* field = descriptor->field(i);
		
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