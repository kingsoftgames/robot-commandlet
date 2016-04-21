echo "Runing CustomResource.sh for $1:"

GameName=$1
PackageDir="${1}RobotTest"
GamePath="$PackageDir/$GameName"

find $GamePath -name "Material" -type d | xargs rm -rf

find $GamePath -name "Materials" -type d | xargs rm -rf

find $GamePath -name "Textures" -type d | xargs rm -rf

find $GamePath -name "Texture" -type d | xargs rm -rf

