#include "WingShot/WingShotGameMode.h"

#include "WingShot/WingShotHUD.h"

AWingShotGameMode::AWingShotGameMode()
{
	HUDClass = AWingShotHUD::StaticClass();
}
