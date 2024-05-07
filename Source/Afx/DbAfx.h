#pragma once

import DNDbObj;

// obj.name is to verify that the name is a member of the obj
// lambda has no practical meaning 
#define DBSelect(obj, name) .Select(#name, [&](){return obj.name();})
#define DBSelectCond(obj, name, cond, splicing) .SelectCond(obj, #name, cond, splicing, [&](){return obj.name();})
#define DBUpdate(obj, name) .Update(obj, #name, [&](){return obj.name();})
#define DBUpdateCond(obj, name, cond, splicing) .UpdateCond(obj, #name, cond, splicing, [&](){return obj.name();})
#define DBDeleteCond(obj, name, cond, splicing) .DeleteCond(obj, #name, cond, splicing, [&](){return obj.name();})


// template
/*
list<Account> datas;
Account temp;
temp.set_accountid(1);
temp.set_createtime(123123);
temp.set_updatetime(324);
temp.set_lastlogouttime(324);
temp.set_authstring("2342");
temp.set_authname("zxcxfd");
// temp.set_email("iopi");

temp.clear_accountid();

datas.emplace_back(temp);
temp.set_email("iopi");
temp.set_authname("asds");
datas.emplace_back(temp);

temp.clear_email();
temp.set_authname("sadas1");
datas.emplace_back(temp);

// accountInfo.Inserts(datas);

accountInfo
	.SelectAll()
	// DBSelect(temp, email)
	DBSelectCond(temp, email,"=", "")
	// .SelectCond("createtime", "=", "", to_string(temp.createtime()))
	.Commit();

for(Account& msg : accountInfo.Result())
{
	DNPrint(0, LoggerLevel::Debug, "%s \n", msg.DebugString().c_str());
}

auto now = std::chrono::system_clock::now();

auto microseconds = std::chrono::time_point_cast<std::chrono::microseconds>(now);

double timestamp = static_cast<double>(microseconds.time_since_epoch().count());

temp.set_createtime(timestamp);
temp.set_createtime(temp.createtime() / 1000000);

accountInfo
	// .SelectAll()
	DBUpdate(temp, email)
	DBUpdateCond(temp, email,"=", "")
	// .SelectCond("createtime", "=", "", to_string(temp.createtime()))
	.Commit();

temp.Clear();
temp.set_accountid(1);
// accountInfo
// 	DBDeleteCond(temp, accountid, "=", "")
// 	.Commit();

*/