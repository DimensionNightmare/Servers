#pragma once

import DNDbObj;

// obj.name is to verify that the name is a member of the obj
// lambda has no practical meaning 
#define DBSelect(obj, name) .Select(#name, [obj](){return obj.name();})
#define DBSelectCond(obj, name, cond, splicing) .SelectCond(obj, #name, cond, splicing, [obj](){return obj.name();})
#define DBUpdate(obj, name) .Update(obj, #name, [obj](){return obj.name();})
#define DBUpdateCond(obj, name, cond, splicing) .UpdateCond(obj, #name, cond, splicing, [obj](){return obj.name();})
#define DBDeleteCond(obj, name, cond, splicing) .DeleteCond(obj, #name, cond, splicing, [obj](){return obj.name();})
