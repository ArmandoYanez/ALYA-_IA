"""Configura el jugador de WingShot dentro del editor.

  python Tools/ue_exec.py Tools/setup_player.py

- Crea IA_Sneak, IA_Binoculars e IA_FireNet y los mapea en IMC_Default
  (Ctrl izquierdo, clic derecho y clic izquierdo).
- Cambia el padre de BP_FirstPersonCharacter a AWingShotCharacter.
- Cambia el padre de BP_FirstPersonGameMode a AWingShotGameMode (HUD de WingShot).
"""
import unreal

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
ACTIONS_DIR = "/Game/WingShot/Input"


def make_action(name):
    path = f"{ACTIONS_DIR}/{name}"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    action = tools.create_asset(name, ACTIONS_DIR, unreal.InputAction, unreal.InputAction_Factory())
    action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    eal.save_loaded_asset(action)
    return action


def make_key(name):
    key = unreal.Key()
    key.import_text(name)
    return key


sneak = make_action("IA_Sneak")
binoculars = make_action("IA_Binoculars")
fire_net = make_action("IA_FireNet")

imc = unreal.load_asset("/Game/Input/IMC_Default")
for action, key_name in ((sneak, "LeftControl"), (binoculars, "RightMouseButton"), (fire_net, "LeftMouseButton")):
    imc.unmap_all_keys_from_action(action)
    imc.map_key(action, make_key(key_name))
eal.save_loaded_asset(imc)

# Personaje: nuevo padre C++ con sigilo, binoculares y red.
# (Las acciones de input se asignan en el constructor de AWingShotCharacter.)
bp = unreal.load_asset("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter")
unreal.BlueprintEditorLibrary.reparent_blueprint(bp, unreal.WingShotCharacter)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
eal.save_loaded_asset(bp, False)

# GameMode: nuevo padre C++ que define el HUD de WingShot.
gm_bp = unreal.load_asset("/Game/FirstPerson/Blueprints/BP_FirstPersonGameMode")
unreal.BlueprintEditorLibrary.reparent_blueprint(gm_bp, unreal.WingShotGameMode)
unreal.BlueprintEditorLibrary.compile_blueprint(gm_bp)
eal.save_loaded_asset(gm_bp, False)

print("OK jugador y GameMode configurados")
