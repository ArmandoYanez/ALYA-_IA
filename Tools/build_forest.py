"""Construye el nivel prototipo de WingShot: /Game/WingShot/Maps/Lvl_BirdForest.

Se ejecuta dentro del editor:  python Tools/ue_exec.py Tools/build_forest.py
Es re-ejecutable: borra y vuelve a crear el nivel.

Puntos de interes que usara la IA (BirdPerch / BirdFeedSpot / TargetPoints con tags):
  Perch          rama donde un ave puede posarse
  OwlPerch       percha alta/oculta preferida por el buho (tambien tiene Perch)
  FeedSpot       zona del suelo donde las aves bajan a comer
  MousePath      puntos que recorren los ratones
  EagleWaypoint  ruta de vuelo del aguila sobre los arboles
"""
import math
import random
import unreal

random.seed(7)

ROOT = "/Game/WingShot"
MAP_PATH = ROOT + "/Maps/Lvl_BirdForest"
MAT_DIR = ROOT + "/Materials"

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assets = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube")
CYLINDER = unreal.load_asset("/Engine/BasicShapes/Cylinder")
SPHERE = unreal.load_asset("/Engine/BasicShapes/Sphere")
CONE = unreal.load_asset("/Engine/BasicShapes/Cone")
BASE_MAT = unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial")


# ---------- Materiales ----------
def make_material(name, rgb):
    path = f"{MAT_DIR}/{name}"
    if eal.does_asset_exist(path):
        mi = unreal.load_asset(path)
    else:
        mi = assets.create_asset(name, MAT_DIR, unreal.MaterialInstanceConstant,
                                 unreal.MaterialInstanceConstantFactoryNew())
        unreal.MaterialEditingLibrary.set_material_instance_parent(mi, BASE_MAT)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        mi, "Color", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    eal.save_loaded_asset(mi)
    return mi


MAT = {
    "Grass": make_material("MI_Grass", (0.05, 0.12, 0.02)),
    "Bark": make_material("MI_Bark", (0.09, 0.04, 0.015)),
    "Leaves": make_material("MI_Leaves", (0.02, 0.13, 0.02)),
    "LeavesDark": make_material("MI_LeavesDark", (0.01, 0.07, 0.015)),
    "Bush": make_material("MI_Bush", (0.03, 0.09, 0.012)),
    "Rock": make_material("MI_Rock", (0.12, 0.12, 0.11)),
    "Seeds": make_material("MI_FeedSpot", (0.55, 0.35, 0.06)),
    "Path": make_material("MI_DirtPath", (0.14, 0.08, 0.03)),
}


# ---------- Nivel nuevo ----------
if eal.does_asset_exist(MAP_PATH):
    levels.load_level("/Game/FirstPerson/Lvl_FirstPerson")
    eal.delete_asset(MAP_PATH)
levels.new_level(MAP_PATH)


def spawn_mesh(label, mesh, mat, loc, scale, rot=(0, 0, 0), folder="", tags=()):
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc),
                                      unreal.Rotator(roll=rot[0], pitch=rot[1], yaw=rot[2]))
    comp = a.static_mesh_component
    comp.set_static_mesh(mesh)
    comp.set_material(0, mat)
    a.set_actor_scale3d(unreal.Vector(*scale))
    a.set_actor_label(label)
    if folder:
        a.set_folder_path(folder)
    if tags:
        a.tags = [unreal.Name(t) for t in tags]
    return a


def spawn_point(label, loc, tags, folder, cls=unreal.TargetPoint):
    p = actors.spawn_actor_from_class(cls, unreal.Vector(*loc))
    p.set_actor_label(label)
    p.tags = [unreal.Name(t) for t in tags]
    p.set_folder_path(folder)
    return p


# ---------- Suelo y ambiente ----------
spawn_mesh("Ground", CUBE, MAT["Grass"], (0, 0, -50), (90, 90, 1), folder="Environment")

sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000),
                                    unreal.Rotator(roll=0, pitch=-40, yaw=35))
sun.set_actor_label("Sun")
sun.light_component.set_editor_property("atmosphere_sun_light", True)
sun.light_component.set_intensity(6.0)
sun.set_folder_path("Lighting")

for cls, label in ((unreal.SkyAtmosphere, "SkyAtmosphere"),
                   (unreal.ExponentialHeightFog, "HeightFog"),
                   (unreal.VolumetricCloud, "Clouds")):
    a = actors.spawn_actor_from_class(cls, unreal.Vector(0, 0, 0))
    a.set_actor_label(label)
    a.set_folder_path("Lighting")

sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
sky.set_actor_label("SkyLight")
sky.light_component.set_editor_property("real_time_capture", True)
sky.set_folder_path("Lighting")

# ---------- Arboles ----------
# Anillo de bosque alrededor de un claro central donde aparece el jugador.
tree_positions = []
for i in range(24):
    ang = (i / 24.0) * 2 * math.pi + random.uniform(-0.15, 0.15)
    radius = random.uniform(1700, 3900)
    tree_positions.append((math.cos(ang) * radius, math.sin(ang) * radius))
for _ in range(6):  # algunos arboles mas adentro para dar cobertura
    ang = random.uniform(0, 2 * math.pi)
    radius = random.uniform(900, 1400)
    tree_positions.append((math.cos(ang) * radius, math.sin(ang) * radius))

perch_count = 0
for t, (x, y) in enumerate(tree_positions, start=1):
    folder = f"Trees/Tree_{t:02d}"
    height = random.uniform(650, 950)
    trunk_w = random.uniform(0.45, 0.7)
    spawn_mesh(f"Tree_{t:02d}_Trunk", CYLINDER, MAT["Bark"], (x, y, height / 2),
               (trunk_w, trunk_w, height / 100), folder=folder, tags=("Tree",))
    leaves = MAT["Leaves"] if t % 2 else MAT["LeavesDark"]
    canopy = random.uniform(4.0, 5.5)
    spawn_mesh(f"Tree_{t:02d}_Canopy", SPHERE, leaves, (x, y, height + canopy * 30),
               (canopy, canopy, canopy * 0.8), folder=folder)

    # Ramas horizontales con una percha en la punta.
    n_branches = random.randint(2, 4)
    base_yaw = random.uniform(0, 360)
    for b in range(n_branches):
        yaw = base_yaw + b * (360.0 / n_branches) + random.uniform(-20, 20)
        z = random.uniform(height * 0.55, height * 0.85)
        length = random.uniform(180, 260)
        rad = math.radians(yaw)
        dx, dy = math.cos(rad), math.sin(rad)
        center = (x + dx * length / 2, y + dy * length / 2, z)
        # Pitch 90 acuesta el cilindro; el yaw lo orienta hacia afuera del tronco.
        spawn_mesh(f"Tree_{t:02d}_Branch_{b + 1}", CYLINDER, MAT["Bark"], center,
                   (0.14, 0.14, length / 100), rot=(0, 90, yaw), folder=folder)
        tip = (x + dx * (length - 25), y + dy * (length - 25), z + 12)
        tags = ["Perch"]
        if z > height * 0.75 and t % 3 == 0:
            tags.append("OwlPerch")
        perch_count += 1
        perch = spawn_point(f"Perch_{t:02d}_{b + 1}", tip, tags, f"{folder}/Perches", cls=unreal.BirdPerch)
        perch.set_editor_property("owl_perch", "OwlPerch" in tags)

# ---------- Arbustos (cobertura para acercarse sigilosamente) ----------
for i in range(18):
    ang = random.uniform(0, 2 * math.pi)
    radius = random.uniform(600, 3000)
    s = random.uniform(1.5, 2.8)
    spawn_mesh(f"Bush_{i + 1:02d}", SPHERE, MAT["Bush"],
               (math.cos(ang) * radius, math.sin(ang) * radius, s * 30),
               (s, s, s * 0.6), folder="Environment/Bushes")

# ---------- Rocas ----------
for i in range(10):
    ang = random.uniform(0, 2 * math.pi)
    radius = random.uniform(800, 3200)
    s = random.uniform(0.8, 2.0)
    spawn_mesh(f"Rock_{i + 1:02d}", CUBE, MAT["Rock"],
               (math.cos(ang) * radius, math.sin(ang) * radius, s * 35),
               (s * 1.3, s, s * 0.7), rot=(random.uniform(-8, 8), random.uniform(-8, 8), random.uniform(0, 360)),
               folder="Environment/Rocks")

# ---------- Zonas de alimentacion ----------
feed_count = 0
for i in range(8):
    tx, ty = tree_positions[i * 2]
    ang = random.uniform(0, 2 * math.pi)
    fx, fy = tx + math.cos(ang) * 350, ty + math.sin(ang) * 350
    spawn_mesh(f"FeedSpot_{i + 1:02d}_Seeds", CYLINDER, MAT["Seeds"], (fx, fy, 1),
               (1.6, 1.6, 0.02), folder="AI/FeedSpots")
    spawn_point(f"FeedSpot_{i + 1:02d}", (fx, fy, 0), ["FeedSpot"], "AI/FeedSpots", cls=unreal.BirdFeedSpot)
    feed_count += 1

# ---------- Rutas de ratones (pares A-B) ----------
mouse_count = 0
for i in range(4):
    ang = (i / 4.0) * 2 * math.pi + 0.4
    a = (math.cos(ang) * 1200, math.sin(ang) * 1200)
    b = (math.cos(ang + 0.9) * 2600, math.sin(ang + 0.9) * 2600)
    for tag_pt, (px, py) in (("A", a), ("B", b)):
        spawn_mesh(f"MouseHole_{i + 1}{tag_pt}", CYLINDER, MAT["Path"], (px, py, 1),
                   (0.7, 0.7, 0.02), folder="AI/MousePaths")
        point = spawn_point(f"MousePath_{i + 1}{tag_pt}", (px, py, 20), ["MousePath", f"MouseRoute{i + 1}"],
                            "AI/MousePaths", cls=unreal.MousePathPoint)
        point.set_editor_property("route_id", i + 1)
        mouse_count += 1

# ---------- Ruta de vuelo del aguila ----------
for i in range(8):
    ang = (i / 8.0) * 2 * math.pi
    spawn_point(f"EagleWaypoint_{i + 1}", (math.cos(ang) * 2500, math.sin(ang) * 2500, 2200),
                ["EagleWaypoint"], "AI/EagleRoute", cls=unreal.EagleWaypoint)

# ---------- Criaturas (spawners) ----------
for label, pawn_class, spawn_point_type, count, respawn in (
        ("Spawner_CommonBird", unreal.CommonBird, unreal.CreatureSpawnPoint.PERCH, 16, 25.0),
        ("Spawner_Owl", unreal.OwlBird, unreal.CreatureSpawnPoint.OWL_PERCH, 3, 40.0),
        ("Spawner_Eagle", unreal.EagleBird, unreal.CreatureSpawnPoint.SKY, 1, 45.0),
        ("Spawner_Mouse", unreal.MouseCritter, unreal.CreatureSpawnPoint.MOUSE_ROUTE, 4, 15.0)):
    spawner = actors.spawn_actor_from_class(unreal.BirdSpawner, unreal.Vector(0, 0, 300))
    spawner.set_actor_label(label)
    spawner.set_folder_path("AI/Spawners")
    spawner.set_editor_property("pawn_class", pawn_class)
    spawner.set_editor_property("spawn_point", spawn_point_type)
    spawner.set_editor_property("count", count)
    spawner.set_editor_property("respawn_delay", respawn)

# ---------- Navegacion y jugador ----------
nav = actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0, 0, 0))
nav.set_actor_label("NavMeshBounds")
nav.set_actor_scale3d(unreal.Vector(45, 45, 6))
nav.set_folder_path("AI")

# NavMesh dinamico: se genera al iniciar el juego (no depende de guardarlo construido).
for recast in [a for a in actors.get_all_level_actors() if isinstance(a, unreal.RecastNavMesh)]:
    recast.set_editor_property("runtime_generation", unreal.RuntimeGenerationType.DYNAMIC)

start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 120))
start.set_actor_label("PlayerStart")

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
gm_class = eal.load_blueprint_class("/Game/FirstPerson/Blueprints/BP_FirstPersonGameMode")
world.get_world_settings().set_editor_property("default_game_mode", gm_class)

levels.save_current_level()
print(f"OK: {len(tree_positions)} arboles, {perch_count} perchas, {feed_count} zonas de comida, "
      f"{mouse_count} puntos de raton, nivel guardado en {MAP_PATH}")
