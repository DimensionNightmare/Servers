module;
#include "StdAfx.h"
#include "DbExtend.pb.h"

#include "google/protobuf/message.h"
#include "google/protobuf/reflection.h"
#include "google/protobuf/descriptor.pb.h"
#include "pqxx/transaction"
#include "pqxx/result"
#include <sstream>
#include <format>
#include <ctime>
export module DNDbObj;

import Utils.StrUtils;

using namespace std;
using namespace google::protobuf;

//statement
#define SSMBegin "(" 
#define SSMEnd ")"
#define SEnd ";"
#define SSplit ","
#define SSpace " "
#define SDefault "DEFAULT"
#define SCombo " AND "
#define SNOT "NOT"
#define SPrimaryKey "PRIMARY KEY"
#define SNULL "NULL"
#define SNOTNULL "NOT NULL"
#define SISNULL "IS NULL"
#define SNOTISNULL SNOT SSpace SISNULL
#define SWHERE " WHERE "
#define SSELECTALL "*"
#define SUPDATE "UPDATE"
#define SUPDATECOND "UPDATECOND"

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

	// Type First Modify !!!
	switch (field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_STRING:
		{
			if(int lenLimit = field->options().GetExtension(ext_len_limit))
			{
				out.back().assign(format("VARCHAR({0})", lenLimit));
			}
			break;
		}
		case FieldDescriptor::CPPTYPE_UINT32:
		case FieldDescriptor::CPPTYPE_UINT64:
		{
			out.emplace_back(format("CHECK ({0} > -1)", field->name()));
			break;
		}
		case FieldDescriptor::CPPTYPE_DOUBLE:
		{
			if(field->options().GetExtension(ext_datetime))
			{
				out.back().assign("TIMESTAMP WITH TIME ZONE");
			}
		}
	}
	
	if(!field->has_optional_keyword())
	{
		out.emplace_back(SNOTNULL);
	}

	string defStr = field->options().GetExtension(ext_default);
	if(defStr.size())
	{
		out.emplace_back(format("{0} {1}", SDefault, defStr));
	}

	if(field->options().GetExtension(ext_primary_key))
	{
		keys.emplace_back(field->name());
	}

	if(field->options().GetExtension(ext_unique))
	{
		out.emplace_back("UNIQUE");
	}
}

/// @brief Get Message Field Value to String for *** INSERT *** Table
/// @param field 
/// @param reflection 
/// @param data 
/// @param out 
/// @return isNull
void InsertFieldByProtoType(const FieldDescriptor* field, const Reflection* reflection, Message& data, string& out)
{
	out.clear();

	switch(field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_INT32:
			out = to_string(reflection->GetInt32(data, field));
			break;
		case FieldDescriptor::CPPTYPE_UINT32:
			out = to_string(reflection->GetUInt32(data, field));
			break;
		case FieldDescriptor::CPPTYPE_INT64:
			out = to_string(reflection->GetInt64(data, field));
			break;
		case FieldDescriptor::CPPTYPE_UINT64:
			out = to_string(reflection->GetUInt64(data, field));
			break;
		case FieldDescriptor::CPPTYPE_DOUBLE:
			out = to_string(reflection->GetDouble(data, field));
			break;
		case FieldDescriptor::CPPTYPE_FLOAT:
			out = to_string(reflection->GetFloat(data, field));
			break;
		case FieldDescriptor::CPPTYPE_STRING:
			out = out + "'" + reflection->GetString(data, field) + "'";
			break;
		default:
			throw new exception("Please Regist InsertField::Type");
			break;
	}

	switch(field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_DOUBLE: 
		{
			if(field->options().GetExtension(ext_datetime))
			{
				out = format("TO_TIMESTAMP({})", out);
			}
			break;
		}
		case FieldDescriptor::CPPTYPE_STRING: // len_limit
		{
			if(int len = field->options().GetExtension(ext_len_limit))
			{
				if(out.size() > len)
				{
					out = format("field {} lenth > {} limit !!!", field->name(), len);
					throw new exception(out.c_str());
				}
			}
			break;
		}
	}
}

void SelectFieldNameByProtoType(const FieldDescriptor* field, string& out)
{
	if(field->options().GetExtension(ext_datetime))
	{
		out = format("EXTRACT(EPOCH FROM {} at time zone 'UTC') as {}", out, out);
	}
		
}

void SelectFieldByProtoType(const FieldDescriptor* field, const Reflection* reflection, Message& data, pqxx::row::reference value, bool isQueryAll)
{
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
		{
			if( isQueryAll && field->options().GetExtension(ext_datetime) )
			{
				double timestamp = StringToTimestamp(value.as<string>());

				reflection->SetDouble(&data, field,  timestamp);
				break;
			}
			reflection->SetDouble(&data, field, value.as<double>());
			break;
		}
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

void SelectFieldCondByProtoType(const FieldDescriptor* field, const Reflection* reflection, Message& data, string& out)
{
	out.clear();

	if( field->has_optional_keyword() && !reflection->HasField(data, field))
	{
		out = SNULL;
		return;
	}

	switch(field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_INT32:
			out = to_string(reflection->GetInt32(data, field));
			break;
		case FieldDescriptor::CPPTYPE_UINT32:
			out = to_string(reflection->GetUInt32(data, field));
			break;
		case FieldDescriptor::CPPTYPE_INT64:
			out = to_string(reflection->GetInt64(data, field));
			break;
		case FieldDescriptor::CPPTYPE_UINT64:
			out = to_string(reflection->GetUInt64(data, field));
			break;
		case FieldDescriptor::CPPTYPE_DOUBLE:
			out = to_string(reflection->GetDouble(data, field));
			break;
		case FieldDescriptor::CPPTYPE_FLOAT:
			out = to_string(reflection->GetFloat(data, field));
			break;
		case FieldDescriptor::CPPTYPE_STRING:
			out = format("'{}'", reflection->GetString(data, field));
			break;
		default:
			throw new exception("Please Regist InsertField::Type");
			break;
	}

	switch(field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_DOUBLE: 
		{
			if(field->options().GetExtension(ext_datetime))
			{
				out = format("TO_TIMESTAMP({})", out);
			}
			break;
		}
		case FieldDescriptor::CPPTYPE_STRING: // len_limit
		{
			if(int len = field->options().GetExtension(ext_len_limit))
			{
				if(out.size() > len)
				{
					out = format("field {} lenth > {} limit !!!", field->name(), len);
					throw new exception(out.c_str());
				}
			}
			break;
		}
	}
	
}

bool UpdateFieldByProtoType(const FieldDescriptor* field, const Reflection* reflection, Message& data, string& out)
{
	out.clear();

	if( field->has_optional_keyword() && !reflection->HasField(data, field))
	{
		out = SNULL;
		return true;
	}

	switch(field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_INT32:
			out = to_string(reflection->GetInt32(data, field));
			break;
		case FieldDescriptor::CPPTYPE_UINT32:
			out = to_string(reflection->GetUInt32(data, field));
			break;
		case FieldDescriptor::CPPTYPE_INT64:
			out = to_string(reflection->GetInt64(data, field));
			break;
		case FieldDescriptor::CPPTYPE_UINT64:
			out = to_string(reflection->GetUInt64(data, field));
			break;
		case FieldDescriptor::CPPTYPE_DOUBLE:
			out = to_string(reflection->GetDouble(data, field));
			break;
		case FieldDescriptor::CPPTYPE_FLOAT:
			out = to_string(reflection->GetFloat(data, field));
			break;
		case FieldDescriptor::CPPTYPE_STRING:
			out = format("'{}'", reflection->GetString(data, field));
			break;
		default:
			throw new exception("Please Regist InsertField::Type");
			break;
	}

	switch(field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_DOUBLE: 
		{
			if(field->options().GetExtension(ext_datetime))
			{
				out = format("TO_TIMESTAMP({})", out);
			}
			break;
		}
		case FieldDescriptor::CPPTYPE_STRING: // len_limit
		{
			if(int len = field->options().GetExtension(ext_len_limit))
			{
				if(out.size() > len)
				{
					out = format("field {} lenth > {} limit !!!", field->name(), len);
					throw new exception(out.c_str());
				}
			}
			break;
		}
	}
	
	return true;
}

void UpdateFieldCondByProtoType(const FieldDescriptor* field, const Reflection* reflection, Message& data, string& out)
{
	out.clear();

	if( field->has_optional_keyword() && !reflection->HasField(data, field))
	{
		out = SNULL;
		return;
	}

	switch(field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_INT32:
			out = to_string(reflection->GetInt32(data, field));
			break;
		case FieldDescriptor::CPPTYPE_UINT32:
			out = to_string(reflection->GetUInt32(data, field));
			break;
		case FieldDescriptor::CPPTYPE_INT64:
			out = to_string(reflection->GetInt64(data, field));
			break;
		case FieldDescriptor::CPPTYPE_UINT64:
			out = to_string(reflection->GetUInt64(data, field));
			break;
		case FieldDescriptor::CPPTYPE_DOUBLE:
			out = to_string(reflection->GetDouble(data, field));
			break;
		case FieldDescriptor::CPPTYPE_FLOAT:
			out = to_string(reflection->GetFloat(data, field));
			break;
		case FieldDescriptor::CPPTYPE_STRING:
			out = format("'{}'", reflection->GetString(data, field));
			break;
		default:
			throw new exception("Please Regist InsertField::Type");
			break;
	}

	switch(field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_DOUBLE: 
		{
			if(field->options().GetExtension(ext_datetime))
			{
				out = format("TO_TIMESTAMP({})", out);
			}
			break;
		}
		case FieldDescriptor::CPPTYPE_STRING: // len_limit
		{
			if(int len = field->options().GetExtension(ext_len_limit))
			{
				if(out.size() > len)
				{
					out = format("field {} lenth > {} limit !!!", field->name(), len);
					throw new exception(out.c_str());
				}
			}
			break;
		}
	}
}

export template <class TMessage = Message>
class DNDbObj
{
public:
	DNDbObj(pqxx::transaction<>* work);
	~DNDbObj(){};

	const string& GetName(){return pMessage->GetDescriptor()->name();};
	
	vector<TMessage>& Result(){ return mResult;}

	bool Commit();
	//create table
	DNDbObj<TMessage>& InitTable();
	//insert
	void Inserts(list<TMessage>& inObjs);
	//query
	DNDbObj<TMessage>& Select(const char* name, ...);

	DNDbObj<TMessage>& SelectAll(bool foreach = false);

	DNDbObj<TMessage>& SelectCond(TMessage& selObj, const char* name, const char* cond, const char* splicing, ...);

	DNDbObj<TMessage>& Update(TMessage& upObj, const char* name, ...);

	DNDbObj<TMessage>& UpdateCond(TMessage& upObj, const char* name, const char* cond, const char* splicing, ...);

	DNDbObj<TMessage>& DeleteCond(TMessage &outObj, const char* name, const char* cond, const char* splicing, ...);

	bool IsSuccess(){return bExecResult;}

	bool IsExist();

private:
	DNDbObj();

	bool ChangeSqlType(SqlOpType type);

	void SetResult(int affectedRows);

	void BuildSqlStatement();

	DNDbObj<TMessage>& Insert(TMessage& inObj);

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
DNDbObj<TMessage>::DNDbObj(pqxx::transaction<> *work):DNDbObj()
{
	pWork = work;
}

template <class TMessage>
bool DNDbObj<TMessage>::IsExist()
{
	return pWork->query_value<bool>(format("SELECT EXISTS ( SELECT 1 FROM information_schema.tables WHERE table_schema = 'public' AND table_name = lower('{}'));", GetName()));
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
				for(string& cond : it->second)
				{
					if(!hasCondition)
					{
						hasCondition = true;
						ss << SWHERE;
					}

					ss << cond;
				}
			}

			ss << SEnd;
			break;
		}
		case SqlOpType::Update:
		{
			auto& updates = mEles[SUPDATE];
			if(!updates.size())
			{
				throw new exception("NOT SET UPDATE PROPERTY !");
			}

			string selectElems;
			auto it = updates.begin();
			auto itEnd = updates.end();
			for (;it != itEnd;it++)
			{
				selectElems.append(*it);

				if(next(it) != itEnd)
				{
					selectElems += SSplit;
				}
			}

			ss << GetOpTypeBySqlOpType(eType) << GetName() << " SET "  << selectElems ;

			auto& updateConds = mEles[SUPDATECOND];
			
			bool hasCondition = false;

			for(auto condIt = updateConds.begin(); condIt != updateConds.end(); condIt++)
			{
				if(!hasCondition)
				{
					hasCondition = true;
					ss << SWHERE;
				}

				ss << *condIt;
			}
			

			ss << SEnd;
			break;
		}
		case SqlOpType::Delete:
		{
			ss << GetOpTypeBySqlOpType(eType) << GetName();

			bool hasCondition = false;
			
			if(mEles.size())
			{
				auto it = mEles.begin();
				auto itEnd = mEles.end();

				for (it = mEles.begin();it != itEnd;it++)
				{
					for(string& cond : it->second)
					{
						if(!hasCondition)
						{
							hasCondition = true;
							ss << SWHERE;
						}

						ss << cond;
					}
				}
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

	if(mEles.contains(SPrimaryKey))
	{
		throw new exception("Not Allow Exist 'PRIMARY KEY' map key!");
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
void DNDbObj<TMessage>::Inserts(list<TMessage>& inObjs)
{
	if(inObjs.size())
	{
		for(TMessage& one : inObjs)
		{
			Insert(one);
			Commit();
		}
	}
}

template <class TMessage>
DNDbObj<TMessage>& DNDbObj<TMessage>::Insert(TMessage& inObj)
{
	ChangeSqlType(SqlOpType::Insert);

	const Descriptor* descriptor = this->pMessage->GetDescriptor();
	const Reflection* reflection = this->pMessage->GetReflection();

	string params;

	for(int i = 0; i < descriptor->field_count();i++)
	{
		const FieldDescriptor* field = descriptor->field(i);

		if(!reflection->HasField(inObj, field))
		{
			continue;
		}
		
		InsertFieldByProtoType(field, reflection, inObj, params);

		if(! params.size())
		{
			params = SDefault;
		}
		mEles[field->name()].emplace_back(params);
	}

	return *this;
}

template <class TMessage>
DNDbObj<TMessage> &DNDbObj<TMessage>::Select(const char* name, ...)
{
	ChangeSqlType(SqlOpType::Query);

	const Descriptor* descriptor = this->pMessage->GetDescriptor();
	const Reflection* reflection = this->pMessage->GetReflection();

	if(mEles.contains(SSELECTALL))
	{
		throw new exception("exist other select statement!!");
	}

	if(const FieldDescriptor* field = descriptor->FindFieldByLowercaseName(name))
	{
		string name = field->name();
		SelectFieldNameByProtoType(field, name);
		mEles[name];
	}

	return *this;
}

template <class TMessage>
DNDbObj<TMessage> &DNDbObj<TMessage>::SelectAll(bool foreach)
{
	ChangeSqlType(SqlOpType::Query);

	if(foreach)
	{
		const Descriptor* descriptor = this->pMessage->GetDescriptor();
		for(int i = 0; i < descriptor->field_count(); i++)
		{
			Select(descriptor->field(i)->lowercase_name().c_str());
		}
		return *this;
	}

	mEles[SSELECTALL];

	if(mEles.size() > 1)
	{
		throw new exception("exist other select statement!!");
	}

	return *this;
}

template <class TMessage>
DNDbObj<TMessage> &DNDbObj<TMessage>::SelectCond(TMessage& selObj, const char* name, const char* cond, const char* splicing, ...)
{
	const Descriptor* descriptor = this->pMessage->GetDescriptor();
	const Reflection* reflection = this->pMessage->GetReflection();

	const FieldDescriptor* field = descriptor->FindFieldByLowercaseName(name);
	if(!field)
	{
		return *this;
	}

	string value;
	SelectFieldCondByProtoType(field, reflection, selObj, value);

	value = format(" {} {} {} {}", splicing, name, cond, value);

	if(mEles.contains(SSELECTALL))
	{
		mEles[SSELECTALL].emplace_back(value);
	}
	else
	{
		mEles.begin()->second.emplace_back(value);
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

	bool isQueryAll = mEles.contains(SSELECTALL);

	for(int row = 0;row < result.size(); row++)
	{
		TMessage gen;

		pqxx::row rowInfo = result[row];
		for(int col = 0; col < rowInfo.size(); col++)
		{
			if(rowInfo[col].is_null())
			{
				continue;
			}
			SelectFieldByProtoType(descriptor->FindFieldByLowercaseName(keys[col]), reflection, gen, rowInfo[col], isQueryAll);
		}
		mResult.push_back(gen);
	}
}

template <class TMessage>
DNDbObj<TMessage> &DNDbObj<TMessage>::Update(TMessage &upObj, const char *name, ...)
{
	ChangeSqlType(SqlOpType::Update);

	const Descriptor* descriptor = this->pMessage->GetDescriptor();
	const Reflection* reflection = this->pMessage->GetReflection();

	const FieldDescriptor* field = descriptor->FindFieldByLowercaseName(name);
	if(!field)
	{
		return *this;
	}

	string value;
	UpdateFieldByProtoType(field, reflection, upObj, value);

	mEles[SUPDATE].emplace_back(format("{} = {}", field->name(), value));
	
	return *this;
}

template <class TMessage>
DNDbObj<TMessage> &DNDbObj<TMessage>::UpdateCond(TMessage& upObj, const char *name, const char *cond, const char *splicing, ...)
{
	ChangeSqlType(SqlOpType::Update);

	const Descriptor* descriptor = this->pMessage->GetDescriptor();
	const Reflection* reflection = this->pMessage->GetReflection();

	const FieldDescriptor* field = descriptor->FindFieldByLowercaseName(name);
	if(!field)
	{
		return *this;
	}

	string value;
	UpdateFieldCondByProtoType(field, reflection, upObj, value);
	value = format("{} {} {} {}", splicing, name, cond, value);

	mEles[SUPDATECOND].emplace_back(value);
	
	return *this;
}

template <class TMessage>
DNDbObj<TMessage> &DNDbObj<TMessage>::DeleteCond(TMessage &outObj, const char *name, const char *cond, const char *splicing, ...)
{
	ChangeSqlType(SqlOpType::Delete);

	const Descriptor* descriptor = this->pMessage->GetDescriptor();
	const Reflection* reflection = this->pMessage->GetReflection();

	const FieldDescriptor* field = descriptor->FindFieldByLowercaseName(name);
	if(!field)
	{
		return *this;
	}

	string condStr;

	UpdateFieldCondByProtoType(field, reflection, outObj, condStr);

	condStr = format("{} {} {} {}", splicing, name, cond, condStr);

	mEles[SUPDATECOND].emplace_back(condStr);

	return *this;
}
