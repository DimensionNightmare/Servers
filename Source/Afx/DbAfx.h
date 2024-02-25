import DNDbObj;

#define DBSelect(obj, name) .Select(#name, obj.name())
#define DBSelectCond(obj, name, cond, splicing) .SelectCond(obj, #name, cond, splicing, obj.name())
#define DBUpdate(obj, name) .Update(obj, #name, obj.name())
#define DBUpdateCond(obj, name, cond, splicing) .UpdateCond(obj, #name, cond, splicing, obj.name())
#define DBDeleteCond(obj, name, cond, splicing) .DeleteCond(obj, #name, cond, splicing, obj.name())