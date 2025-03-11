# set product names, allows mapping of (devel) name in Project to a more consistent name in release
$settings = lib/OGM-Common/scripts/build/OpenKNX-Build-Settings.ps1 $args[0] "AccessControl" "AccessControl"

$settings.sourceName="AccessControl"  
$settings.targetName="AccessControl"

Return $settings