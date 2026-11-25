"""Ejecuta codigo Python dentro del Unreal Editor abierto (Python Remote Execution).

Uso:
    python Tools/ue_exec.py archivo.py
    python Tools/ue_exec.py -c "import unreal; print(unreal.SystemLibrary.get_engine_version())"
"""
import sys
import time

sys.path.append(r"E:/unreal_versions/UE_5.7/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python")
import remote_execution as remote  # noqa: E402


def run(code):
    config = remote.RemoteExecutionConfig()
    config.multicast_bind_address = "127.0.0.1"
    rex = remote.RemoteExecution(config)
    rex.start()
    try:
        deadline = time.time() + 15
        while not rex.remote_nodes and time.time() < deadline:
            time.sleep(0.2)
        if not rex.remote_nodes:
            print("ERROR: no se encontro ningun Unreal Editor con Remote Execution activo.")
            return 1
        rex.open_command_connection(rex.remote_nodes[0]["node_id"])
        result = rex.run_command(code, unattended=True, exec_mode=remote.MODE_EXEC_FILE)
        for entry in result.get("output", []):
            print(entry.get("output", "").rstrip())
        if not result.get("success", False):
            print("ERROR:", result.get("result"))
            return 1
        return 0
    finally:
        rex.stop()


if __name__ == "__main__":
    if len(sys.argv) >= 3 and sys.argv[1] == "-c":
        source = sys.argv[2]
    elif len(sys.argv) == 2:
        with open(sys.argv[1], encoding="utf-8") as f:
            source = f.read()
    else:
        print(__doc__)
        sys.exit(2)
    sys.exit(run(source))
