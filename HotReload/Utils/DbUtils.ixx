module;
#include "Common/DbExtend.pb.h"
#include "StdMacro.h"

export module DbUtils;

import StrUtils;
import Logger;
import ThirdParty.PbGen;
import ThirdParty.Libpqxx;

using namespace std::chrono;

// statement
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

enum class SqlOpType : uint8_t
{
	None,
	CreateTable,
	Insert,
	Query,
	Update,
	Delete,
	UpdateTable,
};

const char* GetOpTypeBySqlOpType(SqlOpType eType)
{
	switch (eType)
	{
#define ONE(type, name) \
		case type: \
			return name;
		ONE(SqlOpType::CreateTable, "CREATE TABLE ");
		ONE(SqlOpType::Insert, "INSERT INTO ");
		ONE(SqlOpType::Query, "SELECT ");
		ONE(SqlOpType::Update, "UPDATE ");
		ONE(SqlOpType::Delete, "DELETE FROM ");
		ONE(SqlOpType::UpdateTable, "ALTER TABLE ");
#undef ONE
		default:
			throw invalid_argument("Please Check SqlOpType");
			break;
	}

	return "";
}


const string SqlTableColQuery = R"delim(SELECT
column_name
FROM information_schema.columns WHERE table_schema = 'public' AND table_name = '{}';)delim";

const char* GetDbTypeByProtoType(FieldDescriptor::CppType pbType)
{
	// http://postgres.cn/docs/9.4/datatype-numeric.html#DATATYPE-INT
	switch (pbType)
	{
#define ONE(type, name) \
		case type:          \
			return #name;
		ONE(FieldDescriptor::CPPTYPE_INT32, integer);
		ONE(FieldDescriptor::CPPTYPE_UINT32, integer);
		ONE(FieldDescriptor::CPPTYPE_INT64, bigint);
		ONE(FieldDescriptor::CPPTYPE_UINT64, bigint);
		ONE(FieldDescriptor::CPPTYPE_STRING, character varying);
		ONE(FieldDescriptor::CPPTYPE_MESSAGE, bytea);
#undef ONE
		default:
			throw invalid_argument("Please Check CppType");
			break;
	}

	return "";
}

void DirectFieldNameByProtoType(const FieldDescriptor* field, string& out)
{
	// if (field->options().HasExtension(ext_datetime))
	// {
	// 	out = format("EXTRACT(EPOCH FROM {} at time zone 'UTC') as {}", out, out);
	// }
}

// init table
void InitFieldByProtoType(const FieldDescriptor* field, list<string>& out, list<string>& keys)
{
	const char* typeStr = GetDbTypeByProtoType(field->cpp_type());

	out.emplace_back(typeStr);

	const FieldOptions& options = field->options();

	// Type First Modify !!!
	switch (field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_STRING:
		{
			if (int lenLimit = options.GetExtension(ext_len_limit))
			{
				out.back().append(format("({})", lenLimit));
			}
			// else
			// {
			// 	out.back().assign("text");
			// }
			break;
		}
		case FieldDescriptor::CPPTYPE_UINT32:
		case FieldDescriptor::CPPTYPE_UINT64:
		{
			if (int genIndex = field->options().GetExtension(ext_autogen))
			{
				out.emplace_back(format("GENERATED BY DEFAULT AS IDENTITY (START WITH {})", genIndex));
			}

			out.emplace_back(format("CHECK ({0} > -1)", field->name()));
			break;
		}
		default:
			break;
	}

	if (field->is_repeated())
	{
		out.back() += "[]";
	}

	if (!field->is_optional())
	{
		out.emplace_back(SNOTNULL);
	}

	if (const string& defaultStr = options.GetExtension(ext_default); !defaultStr.empty())
	{
		out.emplace_back(format("{0} {1}", SDefault, defaultStr));
	}

	if (options.GetExtension(ext_primary_key))
	{
		keys.emplace_back(field->name());
	}

	if (options.GetExtension(ext_unique))
	{
		out.emplace_back("UNIQUE");
	}
}

// get field data
void GetFieldValueByProtoType(const FieldDescriptor* field, const Reflection* reflection, Message& data, string& out)
{
	out.clear();

	if (field->is_optional() && !reflection->HasField(data, field))
	{
		out = SNULL;
		return;
	}

	bool isRepeat = field->is_repeated();

	const FieldOptions& options = field->options();

	switch (field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_INT32:
			if (isRepeat)
			{
				for (int i = 0; i < reflection->FieldSize(data, field); i++)
				{
					out += to_string(reflection->GetRepeatedInt32(data, field, i)) + ",";
				}
				out.pop_back();
				out = format("'{{ {} }}'", out);
			}
			else
			{
				out = to_string(reflection->GetInt32(data, field));
			}
			break;
		case FieldDescriptor::CPPTYPE_UINT32:
			if (isRepeat)
			{
				for (int i = 0; i < reflection->FieldSize(data, field); i++)
				{
					out += to_string(reflection->GetRepeatedUInt32(data, field, i)) + ",";
				}
				out.pop_back();
				out = format("'{{ {} }}'", out);
			}
			else
			{
				out = to_string(reflection->GetUInt32(data, field));
			}

			break;
		case FieldDescriptor::CPPTYPE_INT64:
			if (isRepeat)
			{
				for (int i = 0; i < reflection->FieldSize(data, field); i++)
				{
					out += to_string(reflection->GetRepeatedInt64(data, field, i)) + ",";
				}
				out.pop_back();
				out = format("'{{ {} }}'", out);
			}
			else
			{
				out = to_string(reflection->GetInt64(data, field));
			}
			break;
		case FieldDescriptor::CPPTYPE_UINT64:
			if (isRepeat)
			{
				for (int i = 0; i < reflection->FieldSize(data, field); i++)
				{
					out += to_string(reflection->GetRepeatedUInt64(data, field, i)) + ",";
				}
				out.pop_back();
				out = format("'{{ {} }}'", out);
			}
			else
			{
				out = to_string(reflection->GetUInt64(data, field));
			}
			break;
		case FieldDescriptor::CPPTYPE_STRING:
			if (isRepeat)
			{
				for (int i = 0; i < reflection->FieldSize(data, field); i++)
				{
					out = out + "'" + reflection->GetRepeatedString(data, field, i) + "'" + ",";
				}
				out.pop_back();
				out = format("'{{ {} }}'", out);
			}
			else
			{
				out = out + "'" + reflection->GetString(data, field) + "'";
			}
			break;
		case FieldDescriptor::CPPTYPE_MESSAGE:
			if (isRepeat)
			{
				throw invalid_argument("Please Regist InsertField::CppType");
			}
			else
			{
				const Message& msg = reflection->GetMessage(data, field);
				msg.SerializeToString(&out);
				BytesToHexString(out);
				out = format("E'\\\\x{}'", out);
			}
			break;
		default:
			throw invalid_argument("Please Regist InsertField::CppType");
			break;
	}

	switch (field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_STRING: // len_limit
		{
			if (int len_limit = options.GetExtension(ext_len_limit))
			{
				if (out.size() > len_limit)
				{
					throw invalid_argument(format("field {} lenth > {} limit !!!", field->name(), len_limit));
				}
			}
			break;
		}
		default:
			break;
	}
}

//set field data
void SetFieldValueByProtoType(const FieldDescriptor* field, const Reflection* reflection, Message& data, pq_field value, bool isQueryAll)
{
	// if (field->is_repeated())
	// {
	// 	array_parser arr = value.as_array();
	// 	pair<array_parser::juncture, string> elem;
	// 	int index = 0;
	// 	do
	// 	{
	// 		elem = arr.get_next();
	// 		if (elem.first == array_parser::juncture::string_value)
	// 		{
	// 			reflection->AddBool(&data, field, false);
	// 			reflection->SetRepeatedBool(&data, field, index, elem.second == "t");
	// 			index++;
	// 		}
	// 	} while (elem.first != array_parser::juncture::done);
	// }

	switch (field->cpp_type())
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
		case FieldDescriptor::CPPTYPE_STRING:
			reflection->SetString(&data, field, value.as<string>());
			break;
		case FieldDescriptor::CPPTYPE_MESSAGE:
		{
			if (field->is_repeated())
			{
				throw invalid_argument("Please Regist SelectField::Type");
			}
			else
			{
				Message* msg = reflection->MutableMessage(&data, field);
				string msgData = value.as<string>();
				msgData = msgData.substr(2);
				HexStringToBytes(msgData);
				msg->ParseFromString(msgData);
			}
		}
		break;
		default:
			throw invalid_argument("Please Regist SelectField::Type");
			break;
	}
}

// get default field data
void GetFieldDefaultValueByProtoType(const FieldDescriptor* field, string& out)
{
	out.clear();

	const FieldOptions& options = field->options();

	if (field->is_repeated())
	{
		out = "{}";
		return;
	}

	switch (field->cpp_type())
	{
		case FieldDescriptor::CPPTYPE_INT32:
		case FieldDescriptor::CPPTYPE_UINT32:
		case FieldDescriptor::CPPTYPE_INT64:
		case FieldDescriptor::CPPTYPE_UINT64:
		{
			out = "0";
		}
		break;
		case FieldDescriptor::CPPTYPE_STRING:
		{
			out = "''";
		}
		break;
		case FieldDescriptor::CPPTYPE_MESSAGE:
		{
			out = "E'\\\\x'";
		}
		break;
		default:
			throw invalid_argument("Please Regist InsertField::CppType");
			break;
	}

}

export class IDbSqlHelper
{
public:
	~IDbSqlHelper() = default;

	virtual bool IsExist() = 0;

	virtual IDbSqlHelper& CreateTable() = 0;

	virtual IDbSqlHelper& UpdateTable() = 0;

	virtual bool Commit() = 0;

	virtual const string& GetName() = 0;

	virtual string GetTableSchemaMd5() = 0;
};

export template <class TMessage = Message>
class DbSqlHelper : public IDbSqlHelper
{
public:
	DbSqlHelper(dbtransaction* work, TMessage* = nullptr);
	virtual ~DbSqlHelper();

	const string& GetName() { return pEntity->GetDescriptor()->name(); }

	const vector<TMessage*>& Result() { return mResult; }

	uint32_t ResultCount() { return iQueryCount; }

	bool Commit();
	// create table
	DbSqlHelper<TMessage>& CreateTable();

	DbSqlHelper<TMessage>& UpdateTable();

	// insert
	DbSqlHelper<TMessage>& Insert(bool bSetDefault = false);

	// query
	DbSqlHelper<TMessage>& SelectOne(const char* name, ...);

	DbSqlHelper<TMessage>& SelectByKey(const char* name, ...);

	DbSqlHelper<TMessage>& SelectAll(bool foreach = false, bool quertCount = false);

	DbSqlHelper<TMessage>& SelectCond(const char* name, const char* cond, const char* splicing, ...);

	DbSqlHelper<TMessage>& UpdateOne(const char* name, ...);

	DbSqlHelper<TMessage>& UpdateByKey(const char* name, ...);

	DbSqlHelper<TMessage>& UpdateCond(const char* name, const char* cond, const char* splicing, ...);

	DbSqlHelper<TMessage>& DeleteCond(const char* name, const char* cond, const char* splicing, ...);

	[[nodiscard]]
	bool IsSuccess()
	{
		bool success = bExecResult;
		bExecResult = false;
		return success;
	}

	bool IsExist();

	DbSqlHelper<TMessage>& Limit(uint32_t limit);

	string GetBuildSqlStatement();

	DbSqlHelper<TMessage>& InitEntity(TMessage& entity);

	string GetTableSchemaMd5();

private:
	bool ChangeSqlType(SqlOpType type);

	void SetResult(int affectedRows);

	void BuildSqlStatement();

	void PaserQuery(pq_result& result);

	void ReleaseResult();

private:
	vector<TMessage*> mResult;

	SqlOpType eType = SqlOpType::None;
	// create table, instert
	unordered_map<string, list<string>> mEles;

	dbtransaction* pWork = nullptr;

	string sSqlStatement;

	bool bExecResult = false;

	int iExecResultCount = 0;

	uint32_t iLimitCount = 0;

	uint32_t iQueryCount = 0;

	TMessage* pEntity = nullptr;
};

template <class TMessage>
DbSqlHelper<TMessage>::DbSqlHelper(dbtransaction* work, TMessage* entity)
{
	pWork = work;
	pEntity = entity;
}

template<class TMessage>
DbSqlHelper<TMessage>::~DbSqlHelper()
{
	ReleaseResult();
}

template <class TMessage>
bool DbSqlHelper<TMessage>::IsExist()
{
	return pWork->query_value<bool>(format("SELECT EXISTS ( SELECT 1 FROM pg_catalog.pg_tables WHERE schemaname = 'public' AND tablename = '{}');", GetName()));
}

template <class TMessage>
DbSqlHelper<TMessage>& DbSqlHelper<TMessage>::Limit(uint32_t limit)
{
	if (iQueryCount)
	{
		throw invalid_argument("Exist Limit Cant use count(*)");
	}

	iLimitCount = limit;

	return *this;
}

template<class TMessage>
string DbSqlHelper<TMessage>::GetBuildSqlStatement()
{
	BuildSqlStatement();
	return sSqlStatement;
}

template<class TMessage>
DbSqlHelper<TMessage>& DbSqlHelper<TMessage>::InitEntity(TMessage& entity)
{
	pEntity = &entity;
	return *this;
}

template<class TMessage>
string DbSqlHelper<TMessage>::GetTableSchemaMd5()
{
	const Descriptor* descriptor = pEntity->GetDescriptor();

	stringstream stream;
	list<string> out;
	list<string> primaryKey;
	stream << "";
	for (int i = 0; i < descriptor->field_count(); i++)
	{
		stream << descriptor->field(i)->name();
		InitFieldByProtoType(descriptor->field(i), out, primaryKey);
		for (const string& property : out)
		{
			stream << property;
		}
	}

	for (auto& key : primaryKey)
	{
		stream << key;
	}

	return Md5Hash(stream.str());
}

template <class TMessage>
bool DbSqlHelper<TMessage>::ChangeSqlType(SqlOpType type)
{
	if (eType != SqlOpType::None && eType != type)
	{
		throw invalid_argument("Not Commit to Change OpType");
		return false;
	}

	eType = type;
	bExecResult = false;
	iExecResultCount = 0;
	return true;
}

template <class TMessage>
void DbSqlHelper<TMessage>::SetResult(int affectedRows)
{
	switch (eType)
	{
		case SqlOpType::Update:
		case SqlOpType::Delete:
		case SqlOpType::Query:
		case SqlOpType::UpdateTable:
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
			throw invalid_argument("Please Imp SetResult Case!");
	}

	iExecResultCount = 0;
	eType = SqlOpType::None;
	mEles.clear();
}

template <class TMessage>
void DbSqlHelper<TMessage>::BuildSqlStatement()
{
	if (eType == SqlOpType::None)
	{
		return;
	}

	stringstream ss;

	ss << "";

	string limit;

	if (iLimitCount)
	{
		limit = format(" limit {}", iLimitCount);
	}

	switch (eType)
	{
		case SqlOpType::CreateTable:
		{
			ss << GetOpTypeBySqlOpType(eType) << "\"" << GetName() << "\"";

			if (!mEles.size())
			{
				// data null
				return;
			}

			ss << SSMBegin;

			const Descriptor* descriptor = pEntity->GetDescriptor();

			for (int i = 0; i < descriptor->field_count(); i++)
			{
				const FieldDescriptor* field = descriptor->field(i);
				const string& fieldName = field->name();
				if (!mEles.contains(fieldName))
				{
					continue;
				}

				ss << fieldName;
				for (string& props : mEles[fieldName])
				{
					ss << SSpace << props;
				}

				ss << SSplit;

				mEles.erase(fieldName);
			}

			auto it = mEles.begin();
			auto itEnd = mEles.end();
			for (; it != itEnd; it++)
			{
				ss << it->first;
				for (string& props : it->second)
				{
					ss << SSpace << props;
				}

				if (next(it) != itEnd)
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
			ss << GetOpTypeBySqlOpType(eType) << "\"" << GetName() << "\"";

			if (!mEles.size())
			{
				// key null
				return;
			}

			size_t dataLen = mEles.begin()->second.size();
			if (!dataLen)
			{
				// value null
				return;
			}

			ss << SSMBegin;
			auto it = mEles.begin();
			auto itEnd = mEles.end();
			vector<list<string>> mapping;
			for (; it != itEnd; it++)
			{
				ss << it->first;

				mapping.emplace_back(it->second);

				if (next(it) != itEnd)
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
			for (int lst = 0; lst < dataLen; lst++)
			{
				ss << SSMBegin;
				for (int pos = 0; pos < mapping.size(); pos++)
				{
					ss << mapping[pos].front();
					mapping[pos].pop_front();
					if (mappingSize != pos)
					{
						ss << SSplit;
					}
				}
				ss << SSMEnd;

				if (lst + 1 != dataLen)
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
			for (; it != itEnd; it++)
			{
				if (!iQueryCount)
				{
					selectElems.append(it->first);
					if (next(it) != itEnd)
					{
						selectElems += SSplit;
					}
				}
				else
				{
					selectElems = "COUNT(*)";
					break;
				}
			}

			ss << GetOpTypeBySqlOpType(eType) << selectElems << " FROM " << "\"" << GetName() << "\"";

			bool hasCondition = false;

			for (it = mEles.begin(); it != itEnd; it++)
			{
				for (string& cond : it->second)
				{
					if (!hasCondition)
					{
						hasCondition = true;
						ss << SWHERE;
					}

					ss << cond;
				}
			}

			ss << limit;

			ss << SEnd;
			break;
		}
		case SqlOpType::Update:
		{
			auto& updates = mEles[SUPDATE];
			if (!updates.size())
			{
				throw invalid_argument("NOT SET UPDATE PROPERTY !");
			}

			string selectElems;
			auto it = updates.begin();
			auto itEnd = updates.end();
			for (; it != itEnd; it++)
			{
				selectElems.append(*it);

				if (next(it) != itEnd)
				{
					selectElems += SSplit;
				}
			}

			ss << GetOpTypeBySqlOpType(eType) << "\"" << GetName() << "\"" << " SET " << selectElems;

			auto& updateConds = mEles[SUPDATECOND];

			bool hasCondition = false;

			for (auto condIt = updateConds.begin(); condIt != updateConds.end(); condIt++)
			{
				if (!hasCondition)
				{
					hasCondition = true;
					ss << SWHERE;
				}

				ss << *condIt;
			}

			ss << limit;

			ss << SEnd;
			break;
		}
		case SqlOpType::Delete:
		{
			ss << GetOpTypeBySqlOpType(eType) << "\"" << GetName() << "\"";

			bool hasCondition = false;

			if (mEles.size())
			{
				auto it = mEles.begin();
				auto itEnd = mEles.end();

				for (it = mEles.begin(); it != itEnd; it++)
				{
					for (string& cond : it->second)
					{
						if (!hasCondition)
						{
							hasCondition = true;
							ss << SWHERE;
						}

						ss << cond;
					}
				}
			}

			ss << limit;

			ss << SEnd;
			break;
		}
		case SqlOpType::UpdateTable:
		{
			for (auto& [k, items] : mEles)
			{
				for (auto& statement : items)
				{
					ss << statement;
				}
			}
			break;
		}
		default:
			throw invalid_argument("Please Imp BuildSqlStatement Case!");
	}

	sSqlStatement = ss.str();
}

template <class TMessage>
bool DbSqlHelper<TMessage>::Commit()
{
	BuildSqlStatement();

	if (!pWork || sSqlStatement.empty())
	{
		return false;
	}

	DNPrint(0, LoggerLevel::Debug, "%s ", sSqlStatement.c_str());

	pq_result result = pWork->exec(sSqlStatement);

	if (eType == SqlOpType::Query)
	{
		if (!iQueryCount)
		{
			PaserQuery(result);
		}
		else
		{
			iQueryCount = result[0][0].as<uint32_t>();
		}

		iLimitCount = 0;
	}

	SetResult(result.affected_rows());

	return true;
}

template <class TMessage>
DbSqlHelper<TMessage>& DbSqlHelper<TMessage>::CreateTable()
{
	ChangeSqlType(SqlOpType::CreateTable);

	const Descriptor* descriptor = pEntity->GetDescriptor();

	list<string> primaryKey;

	for (int i = 0; i < descriptor->field_count(); i++)
	{
		const FieldDescriptor* field = descriptor->field(i);

		// should init table
		if (field->is_repeated() && field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE)
		{
			throw runtime_error("Not imp!!!");
		}

		list<string> params;
		InitFieldByProtoType(field, params, primaryKey);

		// unregist type
		if (params.size() == 0)
		{
			continue;
		}

		mEles.emplace(make_pair(field->name(), params));
	}

	if (mEles.contains(SPrimaryKey))
	{
		throw invalid_argument("Not Allow Exist 'PRIMARY KEY' map key!");
	}

	if (primaryKey.size())
	{
		string temp = SSMBegin;
		for (auto it = primaryKey.begin(); it != primaryKey.end(); it++)
		{
			temp += *it;

			if (next(it) != primaryKey.end())
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

template<class TMessage>
DbSqlHelper<TMessage>& DbSqlHelper<TMessage>::UpdateTable()
{
	ChangeSqlType(SqlOpType::UpdateTable);

	pq_result result = pWork->exec(vformat(SqlTableColQuery, make_format_args(GetName())));

	unordered_map<string, int> sqlColInfo;

	for (int row = 0; row < result.size(); row++)
	{
		const string& name = result[row][0].as<string>();

		sqlColInfo[name] = row;

	}

	const Descriptor* descriptor = pEntity->GetDescriptor();

	// clear CONSTRAINT
	// result = pWork->exec( format("SELECT conname FROM pg_constraint WHERE conrelid = '{}'::regclass;", GetName()) );
	// for (int row = 0; row < result.size(); row++)
	// {
	// 	pWork->exec(format("ALTER TABLE {} DROP CONSTRAINT {};", GetName(), result[0][0].as<string>()));
	// }

	list<string> primaryKey;
	string tempstr;
	string opTypeStr = GetOpTypeBySqlOpType(eType);

	// check col
	for (int i = 0; i < descriptor->field_count(); i++)
	{
		const string& colName = descriptor->field(i)->name();
		list<string> params;

		const FieldDescriptor* field = descriptor->field(i);

		// update col
		if (sqlColInfo.contains(colName))
		{
			InitFieldByProtoType(field, params, primaryKey);

			// can not null
			if (!field->is_optional())
			{
				params.remove(SNOTNULL);
			}

			// change type
			for (string& param : params)
			{
				tempstr += param + " ";
			}

			mEles[""].emplace_back(format("{0}\"{1}\" ADD COLUMN new_{2} {3};\nUPDATE \"{1}\" SET new_{2} = {2};\n{0}\"{1}\" DROP COLUMN {2};\n{0}\"{1}\" RENAME COLUMN new_{2} TO {2};\n", opTypeStr, GetName(), colName, tempstr));

			cout << mEles[""].back() << endl;

			if (!field->is_optional())
			{
				mEles[""].emplace_back(format("{0}\"{1}\" ALTER COLUMN {2} SET NOT NULL;\n", opTypeStr, GetName(), colName));

				cout << mEles[""].back() << endl;
			}

			tempstr.clear();

			// already deal remove 
			sqlColInfo.erase(colName);
		}
		// create col
		else
		{
			InitFieldByProtoType(descriptor->field(i), params, primaryKey);
			for (string& param : params)
			{
				tempstr += param + " ";
			}
			// ALTER TABLE table_name ADD COLUMN column_name data_type [column_constraint];
			mEles[""].emplace_back(format("{}\"{}\" ADD COLUMN {} {};\n", opTypeStr, GetName(), colName, tempstr));
			tempstr.clear();
		}

	}

	// remove col
	for (auto& iter : sqlColInfo)
	{
		mEles[""].emplace_back(format("{}\"{}\" DROP COLUMN {};\n", opTypeStr, GetName(), iter.first));
	}

	return *this;
}

template <class TMessage>
DbSqlHelper<TMessage>& DbSqlHelper<TMessage>::Insert(bool bSetDefault)
{
	ChangeSqlType(SqlOpType::Insert);

	const Descriptor* descriptor = pEntity->GetDescriptor();
	const Reflection* reflection = pEntity->GetReflection();

	string value;

	string nowTime = to_string(time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch().count());

	for (int i = 0; i < descriptor->field_count(); i++)
	{
		const FieldDescriptor* field = descriptor->field(i);

		const FieldOptions& options = field->options();

		// if primary_key and not other ext. throw
		if (options.GetExtension(ext_primary_key) && !options.GetExtension(ext_autogen) && !reflection->HasField(*pEntity, field))
		{
			throw invalid_argument(format("Insert <{}> Data Not Set primary_key <{}> Data!", GetName(), field->name()));
		}

		value.clear();

		if (reflection->HasField(*pEntity, field))
		{
			GetFieldValueByProtoType(field, reflection, *pEntity, value);
		}

		if (!value.empty())
		{

		}
		// can not null
		else if (value.empty() && !field->is_optional())
		{
			if (const string& defaultStr = options.GetExtension(ext_default); !defaultStr.empty())
			{
				value = defaultStr;
			}
			else if (bSetDefault)
			{
				if (options.GetExtension(ext_datetime))
				{
					value = nowTime;
				}
				else
				{
					GetFieldDefaultValueByProtoType(field, value);
				}
			}
			else
			{
				continue;
			}
		}
		else
		{
			continue;
		}


		mEles[field->name()].emplace_back(value);
	}

	return *this;
}

template <class TMessage>
DbSqlHelper<TMessage>& DbSqlHelper<TMessage>::SelectOne(const char* name, ...)
{
	ChangeSqlType(SqlOpType::Query);

	const Descriptor* descriptor = pEntity->GetDescriptor();
	const Reflection* reflection = pEntity->GetReflection();

	if (mEles.contains(SSELECTALL))
	{
		throw invalid_argument("exist other select statement!!");
	}

	if (const FieldDescriptor* field = descriptor->FindFieldByLowercaseName(name))
	{
		string name = field->name();
		DirectFieldNameByProtoType(field, name);
		mEles[name];
	}

	return *this;
}

template<class TMessage>
DbSqlHelper<TMessage>& DbSqlHelper<TMessage>::SelectByKey(const char* name, ...)
{
	ChangeSqlType(SqlOpType::Query);
	const Descriptor* descriptor = pEntity->GetDescriptor();
	const Reflection* reflection = pEntity->GetReflection();

	const FieldDescriptor* field = descriptor->FindFieldByLowercaseName(name);
	if (!field)
	{
		return *this;
	}

	const FieldOptions& options = field->options();
	if (!options.GetExtension(ext_primary_key))
	{
		throw invalid_argument(format(" {} is not table {} key!", name, GetName()));
		return *this;
	}

	string value;
	GetFieldValueByProtoType(field, reflection, *pEntity, value);

	if (value.empty())
	{
		throw invalid_argument(format(" table {} key is not set!", GetName()));
		return *this;
	}

	mEles[SSELECTALL].emplace_back(format("{}={}", field->name(), value));

	return *this;
}

template <class TMessage>
DbSqlHelper<TMessage>& DbSqlHelper<TMessage>::SelectAll(bool foreach, bool quertCount)
{
	ChangeSqlType(SqlOpType::Query);

	// not default
	if (foreach)
	{
		const Descriptor* descriptor = pEntity->GetDescriptor();
		for (int i = 0; i < descriptor->field_count(); i++)
		{
			SelectOne(descriptor->field(i)->lowercase_name().c_str());
		}
		return *this;
	}

	// not default
	iQueryCount = quertCount;

	mEles[SSELECTALL];

	if (mEles.size() > 1)
	{
		throw invalid_argument("exist other select statement!!");
	}

	return *this;
}

template <class TMessage>
DbSqlHelper<TMessage>& DbSqlHelper<TMessage>::SelectCond(const char* name, const char* cond, const char* splicing, ...)
{
	const Descriptor* descriptor = pEntity->GetDescriptor();
	const Reflection* reflection = pEntity->GetReflection();

	const FieldDescriptor* field = descriptor->FindFieldByLowercaseName(name);
	if (!field)
	{
		return *this;
	}

	string value;
	GetFieldValueByProtoType(field, reflection, *pEntity, value);

	value = string() + splicing + name + cond + value;

	if (mEles.contains(SSELECTALL))
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
void DbSqlHelper<TMessage>::PaserQuery(pq_result& result)
{
	ReleaseResult();

	vector<string> keys;
	for (int col = 0; col < result.columns(); ++col)
	{
		keys.emplace_back(result.column_name(col));
	}

	const Descriptor* descriptor = pEntity->GetDescriptor();
	const Reflection* reflection = pEntity->GetReflection();

	bool isQueryAll = mEles.contains(SSELECTALL);

	for (int row = 0; row < result.size(); row++)
	{
		TMessage* gen = pEntity->New();

		pq_row rowInfo = result[row];
		for (int col = 0; col < rowInfo.size(); col++)
		{
			if (rowInfo[col].is_null())
			{
				continue;
			}

			if (const FieldDescriptor* field = descriptor->FindFieldByLowercaseName(keys[col]))
			{
				SetFieldValueByProtoType(field, reflection, *gen, rowInfo[col], isQueryAll);
			}
		}
		mResult.emplace_back(gen);
	}
}

template<class TMessage>
void DbSqlHelper<TMessage>::ReleaseResult()
{
	for (auto& it : mResult)
	{
		delete it;
	}
	mResult.clear();
}

template <class TMessage>
DbSqlHelper<TMessage>& DbSqlHelper<TMessage>::UpdateOne(const char* name, ...)
{
	ChangeSqlType(SqlOpType::Update);

	const Descriptor* descriptor = pEntity->GetDescriptor();
	const Reflection* reflection = pEntity->GetReflection();

	const FieldDescriptor* field = descriptor->FindFieldByLowercaseName(name);
	if (!field)
	{
		return *this;
	}

	string value;
	GetFieldValueByProtoType(field, reflection, *pEntity, value);

	mEles[SUPDATE].emplace_back(format("{}={}", field->name(), value));

	return *this;
}

template<class TMessage>
DbSqlHelper<TMessage>& DbSqlHelper<TMessage>::UpdateByKey(const char* name, ...)
{
	ChangeSqlType(SqlOpType::Update);
	const Descriptor* descriptor = pEntity->GetDescriptor();
	const Reflection* reflection = pEntity->GetReflection();

	const FieldDescriptor* field = descriptor->FindFieldByLowercaseName(name);
	if (!field)
	{
		return *this;
	}

	const FieldOptions& options = field->options();
	if (!options.GetExtension(ext_primary_key))
	{
		throw invalid_argument(format(" {} is not table {} key!", name, GetName()));
		return *this;
	}

	string value;
	GetFieldValueByProtoType(field, reflection, *pEntity, value);

	if (value.empty())
	{
		throw invalid_argument(format(" table {} key is not set!", GetName()));
		return *this;
	}

	mEles[SUPDATECOND].emplace_back(format("{}={}", field->name(), value));

	for (int i = 0; i < descriptor->field_count(); i++)
	{
		if (field != descriptor->field(i) && reflection->HasField(*pEntity, field))
		{
			UpdateOne(descriptor->field(i)->lowercase_name().c_str());
		}
	}

	if (mEles[SUPDATE].empty())
	{
		throw invalid_argument(format(" table {} Not Update Data!", GetName()));
	}

	return *this;
}

template <class TMessage>
DbSqlHelper<TMessage>& DbSqlHelper<TMessage>::UpdateCond(const char* name, const char* cond, const char* splicing, ...)
{
	ChangeSqlType(SqlOpType::Update);

	const Descriptor* descriptor = pEntity->GetDescriptor();
	const Reflection* reflection = pEntity->GetReflection();

	const FieldDescriptor* field = descriptor->FindFieldByLowercaseName(name);
	if (!field)
	{
		return *this;
	}

	string value;
	GetFieldValueByProtoType(field, reflection, *pEntity, value);
	value = string() + splicing + name + cond + value;

	mEles[SUPDATECOND].emplace_back(value);

	return *this;
}

template <class TMessage>
DbSqlHelper<TMessage>& DbSqlHelper<TMessage>::DeleteCond(const char* name, const char* cond, const char* splicing, ...)
{
	ChangeSqlType(SqlOpType::Delete);

	const Descriptor* descriptor = pEntity->GetDescriptor();
	const Reflection* reflection = pEntity->GetReflection();

	const FieldDescriptor* field = descriptor->FindFieldByLowercaseName(name);
	if (!field)
	{
		return *this;
	}

	string value;

	GetFieldValueByProtoType(field, reflection, *pEntity, value);

	value = string() + splicing + name + cond + value;

	mEles[SUPDATECOND].emplace_back(value);

	return *this;
}
