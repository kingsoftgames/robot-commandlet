GameName=`basename $PWD`
MyPackPath="${GameName}Robot"

GamePath="$MyPackPath/$GameName"
GameBinariesPath="$GamePath/Binaries"
GameBinariesLinuxPath="$GameBinariesPath/Linux"
GameConfigPath="$GamePath/Config"
GameContentPath="$GamePath/Content"
GameDerivedDataCachePath="$GamePath/DerivedDataCache"

UnrealEnginePath="$MyPackPath/UnrealEngine"
InnerEnginePath="$UnrealEnginePath/Engine"
EngineBinariesPath="$InnerEnginePath/Binaries"
EngineBinariesLinuxPath="$EngineBinariesPath/Linux"
EngineBinariesThirdPartyPath="$EngineBinariesPath/ThirdParty"
EngineConfigPath="$InnerEnginePath/Config"
EngineContentPath="$InnerEnginePath/Content"
EngineDerivedDataCachePath="$InnerEnginePath/DerivedDataCache"
EnginePluginsPath="$InnerEnginePath/Plugins"
EngineSavedPath="$InnerEnginePath/Saved"
EngineShadersPath="$InnerEnginePath/Shaders"

#To create Robot dir
if [ ! -d "$MyPackPath" ]; then
    mkdir "$MyPackPath"
else
    echo "The '$MyPackPath' dir has already exist!"
    #exit 1
    rm -rf $MyPackPath
    mkdir $MyPackPath
fi

mkdir $GamePath $GameBinariesPath $GameBinariesLinuxPath $GameConfigPath $GameContentPath $GameDerivedDataCachePath

mkdir $UnrealEnginePath $InnerEnginePath $EngineBinariesPath $EngineBinariesLinuxPath $EngineBinariesThirdPartyPath

mkdir $EngineConfigPath $EngineContentPath $EngineDerivedDataCachePath $EnginePluginsPath $EngineSavedPath $EngineShadersPath

#To deal with Game project files

#copy .uproject file 
cp ./*.uproject $GamePath

#copy Game Logic *.so files
cp -av ./Binaries/Linux/*.so $GameBinariesLinuxPath

#copy config files
cp -av ./Config/* $GameConfigPath

#copy Content files and remove the Paks file
cp -avr ./Saved/Cooked/LinuxNoEditor/$GameName/Content $GamePath

#copy all DerivedDataCache files
cp -avr ./DerivedDataCache $GamePath

#To deal with Unreal Engine files

#copy Engine Binaries files
OrientUnrealEngine="../UnrealEngine/Engine"

cp -avr $OrientUnrealEngine/Binaries/Linux/Linux $EngineBinariesLinuxPath
cp -av  $OrientUnrealEngine/Binaries/Linux/libUE4Editor-*.so $EngineBinariesLinuxPath
cp -av  $OrientUnrealEngine/Binaries/Linux/libmcpp.* $EngineBinariesLinuxPath
cp -av  $OrientUnrealEngine/Binaries/Linux/libnv* $EngineBinariesLinuxPath
cp -av  $OrientUnrealEngine/Binaries/Linux/libopenal.* $EngineBinariesLinuxPath
cp -av  $OrientUnrealEngine/Binaries/Linux/lib.* $EngineBinariesLinuxPath

cp -av  $OrientUnrealEngine/Binaries/Linux/UE4Editor $EngineBinariesLinuxPath

cp -avr $OrientUnrealEngine/Binaries/ThirdParty/ICU $EngineBinariesThirdPartyPath

cp -avr $OrientUnrealEngine/Config $InnerEnginePath

cp -avr $OrientUnrealEngine/Content $InnerEnginePath

cp -avr $OrientUnrealEngine/DerivedDataCache $InnerEnginePath

cp -avr $OrientUnrealEngine/Saved $InnerEnginePath

cp -avr $OrientUnrealEngine/Shaders $InnerEnginePath

#copy plugins files and remove same useless files
cp -avr $OrientUnrealEngine/Plugins $InnerEnginePath

find $InnerEnginePath -name "Intermediate" -type d | xargs rm -rf

echo "Customize this Project's resource."

#remove more resource files for the game project
#./CustomResource.sh $GameName

echo "Packaging all files! It will take some time..."

tar -czvf ${GameName}Robot.tar.gz $MyPackPath

echo "Robot Package End"

