# `UE4` 通用机器人插件 `NetcodeRobot`

## 简介

本文是参考 `Unreal Engine` 提供的单元测试插件 `NetcodeUnitTest` 源码的基础上编写的一个能模拟客户端登录 `Unreal Engine` 的 `Dedicated Server` 的一个 `Commandlet` 插件。 允许用户通过使用控制台命令用 `UE4Editor`  程序启动这个插件，从而运行一个无渲染窗口的客户端游戏进程实例并登录到参数指定的服务器中。本插件目前只处理到登录场景并生成角色，并没有实现其他的机器人的移动或攻击的行为模拟，但相关模块的功能引用都是可行的。后面可以参照单元测试插件 `NetcodeUnitTest` 的 `NUTUnrealEngine4` 工程将机器人行为的模拟功能另做成一个或多个 `Module` ，用本插件去调用这些行为模拟模块的行为模拟函数。

## 编译

**VS2015 环境** 

将插件目录放置到引擎或游戏的Plugins目录下， 其中如果放到引擎`Plugins`目录下则需要打开游戏工程编辑器，打开 `Edit` > `Plugins`。 在界面中找到你的插件信息，并设置插件的激活 `Enable` 选择。如果是游戏目录的 `Plugins` 则是默认激活的。

运行引擎目录下的`GenerateProjectFiles.bat`重新生成引擎工程文件，右击游戏工程`.uproject` 文件重新生成游戏工程文件。 插件工程会加入到对应的工程文件中，用 **VS2015** 打开工程文件编译。

**Linux 环境**

在引擎的根目录下运行 `GenerateProjectFiles.sh` 重新生成游戏和引擎工程。如果引擎工程不需要重新生成则不需要加 `-engine`

	$./GenerateProjectFiles.sh -project="$GAME_UPROJECT" -game -engine

进入引擎目录里的 `$UE4_ROOT/Engine/Build/BatchFiles` 目录运行 `RunUAT.sh` 脚本

编译 Linux Client

	$./RunUAT.sh BuildCookRun -project="$GAME_UPROJECT" -nop4 -build -cook -compressed -stage -platform=Linux -clientconfig=Development -pak -archive -archivedirectory="$ARCHIVED_DIR" -utf8output


其中 `$GAME_UPROJECT` 是游戏工程路径，例如： `/home/user/xxx/.../xxx/MyProject/MyProject.uproject` 。`$ARCHIVED_DIR` 是编译文件输出路径， 填入你想要的输出目录即可。

## 运行

参照 `Unreal Engine` 的 `Commandlet` 运行方式运行本插件：打开游戏工程的编辑器的 `Edit` > `Plugins` 的窗口，在`Networking` 分支下确认本插件的 `Enable` 被选择上。进入`UnrealEngine/Engine/Binaries` 中当前平台的可执行程序目录（如 `Linux`, `Win32` 和 `Win64` 等），然后输入下面的命令：

	UE4Editor.exe "$GameProject" -run=NetcodeRobot.RobotCommandlet -RobotServer=$ServerIp:$Port -RobotMap=$Map

第一个参数用引号将需要测试的游戏工程 `.uproject` 文件所在的绝对路径填入其中，第二个参数是表示选择本插件，第三个参数 `RobotServer` 是将需要连接的服务的地址和端口传入插件， 最后一个参数是指定服务器的运行地图（格式按配置文件中地图文件的填写方式）。

## 注意事项

1. 需要关闭游戏工程的 **UI** 功能调用,  否则可能会造成运行的崩溃
2. 要确保系统运行初始化是按客户端模式进行初始化的，因为 `Commandlet` 在初始化时 `IsClient` 、`IsServer` 和 `IsEditor` 都是设置成 `false` ，游戏工程代码很容易进入服务器模式的初始化。
3. 在引擎升级成4.11版本后， 一些老版本的 'blueprint' 对象需要同步的属性会在初始化后被垃圾收集器清空。 这个不兼容应该是 **UE4.10** 之前版本的造成的，用 **UE4.10** 和 **UE4.11** 新创建的工程没有这个问题。
4. 因为未解决的运行 `Commandlet` 时将第一个工程路径参数当成 `Map` 信息的错误提示，在 **Windows** 下选择按默认地图启动即可，而 **Linux** 下不会提示这个警告而直接进入退出的分支逻辑。 需要修改引擎代码让引擎在 `Commandlet` 模式下发生警告时默认进入默认地图运行， 相关代码在 `void UGameInstance::StartGameInstance()` 函数中：

```
UE_LOG(LogLoad, Error, TEXT("%s"), *FString::Printf(TEXT("Failed to enter %s: %s. Please check the log for errors."), *URL.Map, *Error));

// the map specified on the command-line couldn't be loaded.  ask the user if we should load the default map instead
if (FCString::Stricmp(*PackageName, *DefaultMap) != 0)
{
	const FText Message = FText::Format(NSLOCTEXT("Engine", "MapNotFound", "The map specified on the commandline '{0}' could not be found. Would you like to load the default map instead?"), FText::FromString(URL.Map));

	if (   FCString::Stricmp(*URL.Map, *DefaultMap) != 0  
		&& FMessageDialog::Open(EAppMsgType::OkCancel, Message) != EAppReturnType::Ok)
	{
		// user canceled (maybe a typo while attempting to run a commandlet)
		FPlatformMisc::RequestExit(false);
		return;
	}
	else
	{
		BrowseRet = Engine->Browse(*WorldContext, FURL(&DefaultURL, *(DefaultMap + GameMapsSettings->LocalMapOptions), TRAVEL_Partial), Error);
	}
}
else
{
	const FText Message = FText::Format(NSLOCTEXT("Engine", "MapNotFoundNoFallback", "The map specified on the commandline '{0}' could not be found. Exiting."), FText::FromString(URL.Map));
	FMessageDialog::Open(EAppMsgType::Ok, Message);
	FPlatformMisc::RequestExit(false);
	return;
}
```

其中第二个判断代码需改成


	if (   FCString::Stricmp(*URL.Map, *DefaultMap) != 0  
		&& FMessageDialog::Open(EAppMsgType::OkCancel, Message) != EAppReturnType::Ok
		&& !PRIVATE_GIsRunningCommandlet)

## 文件列表

- [src]()
	- [NetcodeRobot]()
- [patch]() 
	- [GameInstance.cpp.patch]()
	- [KMGameInstance.cpp.patch]()
- [PackagingRobot.sh]()
- [CustomResource.sh]()
- [README.md]()

**`NetcodeRobot`** 目录是插件的源码和工程目录，应用时将整个目录拷贝到对应的 `Plugins` 目录中。

`PackagingRobot.sh` 是 **Linux** 下的机器人程序和资源的打包脚本，需要将它放到游戏工程根目录中运行（`.uproject` 文件所在的目录），它会将机器人所需要的可执行程序、动态链接库和必要的资源文件另外拷贝整理并做打包处理。

如果觉得打包文件太大，可以将 `CustomResource.sh` 脚本拷贝到相同目录下并将 `PackagingRobot.sh` 脚本代码对 `CustomResource.sh` 脚本执行的注释取消，使用 `CustomResource.sh` 将资源文件中的和渲染相关的材质和纹理文件删除。 删除后运行过程中会有资源载入的错误提示，但不影响程序的逻辑运行。

`GameInstance.cpp.patch` 是上面注意事项中第4项的修改的 `patch` 文件，可以在引擎根目录下应用这个 `patch` 文件产生修改，或者参照上面文档描述直接代码中修改。

`KMGameInstance.cpp.patch` 是针对本测试插件的目标游戏工程的修改，其他游戏工程不需要关心。
