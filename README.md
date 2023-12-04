# Servers
Windows 下 C++ HotReload 热更， c++ 20 Module and coroutine 模块和协程，

环境配置 visual studio 2022 or Upper : 组件 c++，cmake。
编辑器我的选择是 visual studio code。

准备好项目运行所需的框架，libhv，protobuf。

全部布置完成之后 可在void DimensionNightmare::InitCmdHandle() 查询相关的控制台命令
启动参数可参考vscode文件夹中launch.json
例如，在项目启动成功之后，可以在控制台输入reload，即可重载c++热更部分代码, 
你可以更改不影响Nightmare执行文件项目的其他代码，重新编译成dll后，输入reload指令后， 即可运行新的hotreload代码逻辑
