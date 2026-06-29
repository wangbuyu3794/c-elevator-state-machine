import ctypes
import json
import sys
import threading
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


HOST = "127.0.0.1"
PORT = 8765
MIN_FLOOR = -1
MAX_FLOOR = 34
TOTAL_FLOOR_COUNT = 35
DOOR_OPEN = 1
ELEVATOR_EVENT_OK = 1

IntArray = ctypes.c_int * TOTAL_FLOOR_COUNT

NAME_ZH = {
    "Idle": "空闲",
    "Moving": "运行中",
    "Door opening": "开门中",
    "Door holding": "开门保持",
    "Door closing": "关门中",
    "Fault": "故障",
    "Paused": "管理员暂停",
    "Power off": "断电",
    "Recovering": "恢复中",
    "None": "无",
    "Up": "上行",
    "Down": "下行",
    "Open": "打开",
    "Closed": "关闭",
    "Door fault": "门故障",
    "Motor fault": "电机故障",
    "Sensor fault": "传感器故障",
    "Unknown fault": "未知故障",
    "Invalid fault": "无效故障",
    "OK": "成功",
    "Null elevator": "电梯对象为空",
    "Invalid floor": "楼层无效",
    "Request already exists": "请求已存在",
    "Power is off": "电源已关闭",
    "Door is not aligned with floor": "电梯未对齐楼层",
    "Door close blocked": "安全条件阻止关门",
    "Already active": "已经处于该状态",
    "No recovery needed": "当前不需要恢复",
    "Backup power unavailable": "备用电源不可用",
    "Not between floors": "电梯不在两层之间",
    "Recovery blocked": "恢复被安全条件阻止",
    "Unknown event result": "未知事件结果",
}


class Elevator(ctypes.Structure):
    _fields_ = [
        ("currentFloor", ctypes.c_int),
        ("targetFloor", ctypes.c_int),
        ("totalTimeSeconds", ctypes.c_int),
        ("idleTimeSeconds", ctypes.c_int),
        ("state", ctypes.c_int),
        ("direction", ctypes.c_int),
        ("door", ctypes.c_int),
        ("carDoor", ctypes.c_int),
        ("landingDoors", IntArray),
        ("landingDoorLocked", IntArray),
        ("isAlignedWithFloor", ctypes.c_int),
        ("fault", ctypes.c_int),
        ("currentLoadKg", ctypes.c_int),
        ("isOverloaded", ctypes.c_int),
        ("isDoorBlocked", ctypes.c_int),
        ("isDoorOpenButtonHeld", ctypes.c_int),
        ("isEmergencyCallActive", ctypes.c_int),
        ("isAdminPaused", ctypes.c_int),
        ("isPowerOff", ctypes.c_int),
        ("isMainPowerOn", ctypes.c_int),
        ("isBackupPowerAvailable", ctypes.c_int),
        ("isBackupPowerActive", ctypes.c_int),
        ("isPowerFailure", ctypes.c_int),
        ("isRecovering", ctypes.c_int),
        ("isBetweenFloors", ctypes.c_int),
        ("safeFloor", ctypes.c_int),
        ("rescueFloor", ctypes.c_int),
        ("directionBeforePowerFailure", ctypes.c_int),
        ("hallUpRequests", IntArray),
        ("hallDownRequests", IntArray),
        ("carFloorRequests", IntArray),
        ("hallUpRequestCreatedAt", IntArray),
        ("hallDownRequestCreatedAt", IntArray),
        ("carRequestCreatedAt", IntArray),
        ("completedRequestCount", ctypes.c_int),
        ("totalWaitTimeSeconds", ctypes.c_int),
        ("longestWaitTimeSeconds", ctypes.c_int),
    ]


class ElevatorSnapshot(ctypes.Structure):
    _fields_ = [
        ("currentFloor", ctypes.c_int),
        ("targetFloor", ctypes.c_int),
        ("currentFloorIndex", ctypes.c_int),
        ("targetFloorIndex", ctypes.c_int),
        ("totalTimeSeconds", ctypes.c_int),
        ("idleTimeSeconds", ctypes.c_int),
        ("state", ctypes.c_int),
        ("direction", ctypes.c_int),
        ("door", ctypes.c_int),
        ("carDoor", ctypes.c_int),
        ("landingDoors", IntArray),
        ("landingDoorLocked", IntArray),
        ("currentLandingDoor", ctypes.c_int),
        ("currentLandingDoorLocked", ctypes.c_int),
        ("areAllLandingDoorsLocked", ctypes.c_int),
        ("isAlignedWithFloor", ctypes.c_int),
        ("fault", ctypes.c_int),
        ("currentLoadKg", ctypes.c_int),
        ("isOverloaded", ctypes.c_int),
        ("isDoorBlocked", ctypes.c_int),
        ("isDoorOpenButtonHeld", ctypes.c_int),
        ("isEmergencyCallActive", ctypes.c_int),
        ("isAdminPaused", ctypes.c_int),
        ("isPowerOff", ctypes.c_int),
        ("isMainPowerOn", ctypes.c_int),
        ("isBackupPowerAvailable", ctypes.c_int),
        ("isBackupPowerActive", ctypes.c_int),
        ("isPowerFailure", ctypes.c_int),
        ("isRecovering", ctypes.c_int),
        ("isBetweenFloors", ctypes.c_int),
        ("safeFloor", ctypes.c_int),
        ("rescueFloor", ctypes.c_int),
        ("directionBeforePowerFailure", ctypes.c_int),
        ("canMove", ctypes.c_int),
        ("canCloseDoor", ctypes.c_int),
        ("hallUpRequests", IntArray),
        ("hallDownRequests", IntArray),
        ("carFloorRequests", IntArray),
        ("hasAnyRequest", ctypes.c_int),
        ("completedRequestCount", ctypes.c_int),
        ("totalWaitTimeSeconds", ctypes.c_int),
        ("longestWaitTimeSeconds", ctypes.c_int),
        ("averageWaitTimeSeconds", ctypes.c_double),
    ]


def floors_descending():
    floors = list(range(MAX_FLOOR, 0, -1))
    floors.append(MIN_FLOOR)
    return floors


def floor_to_index(floor):
    return 0 if floor == MIN_FLOOR else floor


class ElevatorBridge:
    def __init__(self, dll_path):
        self.lock = threading.Lock()
        self.lib = ctypes.CDLL(dll_path)
        self.elevator = Elevator()
        self.configure_api()
        self.lib.Elevator_Init(ctypes.byref(self.elevator))

    def configure_api(self):
        e_ptr = ctypes.POINTER(Elevator)
        s_ptr = ctypes.POINTER(ElevatorSnapshot)
        self.lib.Elevator_Init.argtypes = [e_ptr]
        self.lib.Elevator_GetSnapshot.argtypes = [e_ptr, s_ptr]

        one_int_events = [
            "Elevator_PressHallUpButton",
            "Elevator_PressHallDownButton",
            "Elevator_PressCarFloorButton",
            "Elevator_SetLoad",
            "Elevator_SetDoorBlocked",
            "Elevator_SetBackupPowerAvailable",
            "Elevator_SetBetweenFloors",
            "Elevator_SetFault",
        ]
        for name in one_int_events:
            func = getattr(self.lib, name)
            func.argtypes = [e_ptr, ctypes.c_int]
            func.restype = ctypes.c_int

        no_arg_events = [
            "Elevator_PressDoorOpenButton",
            "Elevator_ReleaseDoorOpenButton",
            "Elevator_PressDoorCloseButton",
            "Elevator_PressEmergencyCallButton",
            "Elevator_ClearEmergencyCall",
            "Elevator_AdminPause",
            "Elevator_AdminResume",
            "Elevator_PowerOff",
            "Elevator_RestorePower",
            "Elevator_SimulatePowerFailure",
            "Elevator_RunBackupRescue",
            "Elevator_RunRecovery",
            "Elevator_ClearFault",
        ]
        for name in no_arg_events:
            func = getattr(self.lib, name)
            func.argtypes = [e_ptr]
            func.restype = ctypes.c_int

        self.lib.Elevator_RunOneStep.argtypes = [e_ptr]

        for name in [
            "Elevator_GetStateName",
            "Elevator_GetDirectionName",
            "Elevator_GetDoorName",
            "Elevator_GetFaultName",
            "Elevator_GetEventResultName",
        ]:
            getattr(self.lib, name).restype = ctypes.c_char_p

    def c_name(self, func, value):
        result = func(value)
        name = result.decode("utf-8") if result else "Unknown"
        return NAME_ZH.get(name, name)

    def snapshot(self):
        with self.lock:
            snap = ElevatorSnapshot()
            self.lib.Elevator_GetSnapshot(ctypes.byref(self.elevator), ctypes.byref(snap))
            return self.snapshot_to_dict(snap)

    def snapshot_to_dict(self, snap):
        floors = []
        for floor in floors_descending():
            index = floor_to_index(floor)
            floors.append(
                {
                    "floor": floor,
                    "isCurrent": floor == snap.currentFloor and not snap.isBetweenFloors,
                    "hallUp": bool(snap.hallUpRequests[index]),
                    "hallDown": bool(snap.hallDownRequests[index]),
                    "car": bool(snap.carFloorRequests[index]),
                    "landingDoorOpen": snap.landingDoors[index] == DOOR_OPEN,
                    "landingDoorLocked": bool(snap.landingDoorLocked[index]),
                }
            )

        return {
            "currentFloor": snap.currentFloor,
            "targetFloor": snap.targetFloor,
            "totalTimeSeconds": snap.totalTimeSeconds,
            "idleTimeSeconds": snap.idleTimeSeconds,
            "state": self.c_name(self.lib.Elevator_GetStateName, snap.state),
            "direction": self.c_name(self.lib.Elevator_GetDirectionName, snap.direction),
            "carDoor": self.c_name(self.lib.Elevator_GetDoorName, snap.carDoor),
            "currentLandingDoor": self.c_name(self.lib.Elevator_GetDoorName, snap.currentLandingDoor),
            "currentLandingDoorLocked": bool(snap.currentLandingDoorLocked),
            "areAllLandingDoorsLocked": bool(snap.areAllLandingDoorsLocked),
            "isAlignedWithFloor": bool(snap.isAlignedWithFloor),
            "fault": self.c_name(self.lib.Elevator_GetFaultName, snap.fault),
            "currentLoadKg": snap.currentLoadKg,
            "isOverloaded": bool(snap.isOverloaded),
            "isDoorBlocked": bool(snap.isDoorBlocked),
            "isDoorOpenButtonHeld": bool(snap.isDoorOpenButtonHeld),
            "isEmergencyCallActive": bool(snap.isEmergencyCallActive),
            "isAdminPaused": bool(snap.isAdminPaused),
            "isMainPowerOn": bool(snap.isMainPowerOn),
            "isBackupPowerAvailable": bool(snap.isBackupPowerAvailable),
            "isBackupPowerActive": bool(snap.isBackupPowerActive),
            "isPowerFailure": bool(snap.isPowerFailure),
            "isRecovering": bool(snap.isRecovering),
            "isBetweenFloors": bool(snap.isBetweenFloors),
            "safeFloor": snap.safeFloor,
            "rescueFloor": snap.rescueFloor,
            "canMove": bool(snap.canMove),
            "canCloseDoor": bool(snap.canCloseDoor),
            "hasAnyRequest": bool(snap.hasAnyRequest),
            "completedRequestCount": snap.completedRequestCount,
            "totalWaitTimeSeconds": snap.totalWaitTimeSeconds,
            "longestWaitTimeSeconds": snap.longestWaitTimeSeconds,
            "averageWaitTimeSeconds": round(snap.averageWaitTimeSeconds, 2),
            "floors": floors,
        }

    def event_result(self, result):
        return {
            "ok": result == ELEVATOR_EVENT_OK,
            "result": result,
            "message": self.c_name(self.lib.Elevator_GetEventResultName, result),
            "snapshot": self.snapshot(),
        }

    def run_step(self):
        with self.lock:
            self.lib.Elevator_RunOneStep(ctypes.byref(self.elevator))
        return {"ok": True, "message": "运行一步完成。", "snapshot": self.snapshot()}

    def reset(self):
        with self.lock:
            self.lib.Elevator_Init(ctypes.byref(self.elevator))
        return {"ok": True, "message": "电梯已重置。", "snapshot": self.snapshot()}

    def call_event(self, action, value=None):
        with self.lock:
            e_ptr = ctypes.byref(self.elevator)
            if action == "hall_up":
                result = self.lib.Elevator_PressHallUpButton(e_ptr, int(value))
            elif action == "hall_down":
                result = self.lib.Elevator_PressHallDownButton(e_ptr, int(value))
            elif action == "car":
                result = self.lib.Elevator_PressCarFloorButton(e_ptr, int(value))
            elif action == "load":
                result = self.lib.Elevator_SetLoad(e_ptr, int(value))
            elif action == "door_blocked":
                result = self.lib.Elevator_SetDoorBlocked(e_ptr, int(value))
            elif action == "backup_available":
                result = self.lib.Elevator_SetBackupPowerAvailable(e_ptr, int(value))
            elif action == "between_floors":
                result = self.lib.Elevator_SetBetweenFloors(e_ptr, int(value))
            elif action == "fault":
                result = self.lib.Elevator_SetFault(e_ptr, int(value))
            elif action == "door_open_hold":
                result = self.lib.Elevator_PressDoorOpenButton(e_ptr)
            elif action == "door_open_release":
                result = self.lib.Elevator_ReleaseDoorOpenButton(e_ptr)
            elif action == "door_close":
                result = self.lib.Elevator_PressDoorCloseButton(e_ptr)
            elif action == "emergency":
                result = self.lib.Elevator_PressEmergencyCallButton(e_ptr)
            elif action == "clear_emergency":
                result = self.lib.Elevator_ClearEmergencyCall(e_ptr)
            elif action == "admin_pause":
                result = self.lib.Elevator_AdminPause(e_ptr)
            elif action == "admin_resume":
                result = self.lib.Elevator_AdminResume(e_ptr)
            elif action == "power_off":
                result = self.lib.Elevator_PowerOff(e_ptr)
            elif action == "restore_power":
                result = self.lib.Elevator_RestorePower(e_ptr)
            elif action == "power_failure":
                result = self.lib.Elevator_SimulatePowerFailure(e_ptr)
            elif action == "backup_rescue":
                result = self.lib.Elevator_RunBackupRescue(e_ptr)
            elif action == "recovery":
                result = self.lib.Elevator_RunRecovery(e_ptr)
            elif action == "clear_fault":
                result = self.lib.Elevator_ClearFault(e_ptr)
            else:
                return {"ok": False, "message": "未知操作。", "snapshot": self.snapshot()}
        return self.event_result(result)


HTML = r"""<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>C 语言电梯状态机</title>
  <style>
    :root { --bg:#eef2f6; --panel:#fff; --ink:#17212b; --muted:#667085; --line:#d7dde5; --accent:#1f6aa5; --danger:#b42318; --ok:#067647; --warn:#b54708; }
    * { box-sizing: border-box; }
    body { margin:0; font-family:"Segoe UI", Arial, sans-serif; background:var(--bg); color:var(--ink); }
    header { display:flex; align-items:center; justify-content:space-between; gap:16px; padding:12px 18px; border-bottom:1px solid var(--line); background:var(--panel); }
    h1 { margin:0; font-size:20px; letter-spacing:0; }
    main { display:grid; grid-template-columns:280px minmax(360px,1fr) 320px; gap:12px; padding:12px; height:calc(100vh - 57px); }
    section { min-height:0; background:var(--panel); border:1px solid var(--line); border-radius:8px; padding:12px; overflow:auto; }
    h2 { margin:0 0 10px; font-size:15px; }
    button { min-height:32px; border:1px solid var(--line); border-radius:6px; background:#f8fafc; color:var(--ink); cursor:pointer; }
    button:hover { background:#edf4fb; border-color:#a8c7df; }
    button:disabled { color:#98a2b3; cursor:default; background:#f2f4f7; }
    input, select { height:32px; border:1px solid var(--line); border-radius:6px; padding:4px 8px; width:100%; }
    .status-grid { display:grid; grid-template-columns:repeat(4,minmax(120px,1fr)); gap:8px; }
    .metric { border:1px solid var(--line); border-radius:6px; padding:8px; background:#fbfcfe; }
    .metric span { display:block; color:var(--muted); font-size:12px; }
    .metric strong { display:block; font-size:15px; margin-top:2px; }
    .hall-row,.shaft-row { display:grid; align-items:center; gap:6px; border-bottom:1px solid #edf0f3; padding:4px 0; }
    .hall-row { grid-template-columns:44px 1fr 1fr; }
    .shaft-row { grid-template-columns:44px 70px 90px 1fr; }
    .floor { text-align:right; font-variant-numeric:tabular-nums; color:var(--muted); }
    .car { text-align:center; border-radius:4px; padding:4px; background:#f2f4f7; }
    .current .floor,.current .car { background:var(--accent); color:white; font-weight:700; }
    .requests { color:var(--warn); font-weight:700; }
    .door { color:var(--muted); font-size:12px; }
    .cabin-grid { display:grid; grid-template-columns:repeat(4,1fr); gap:6px; }
    .controls { display:grid; grid-template-columns:1fr 1fr; gap:8px; margin-top:12px; }
    .wide { grid-column:1 / -1; }
    .inline { display:grid; grid-template-columns:1fr 90px; gap:8px; }
    #log { height:140px; overflow:auto; border:1px solid var(--line); border-radius:6px; background:#101828; color:#f9fafb; padding:8px; font-family:Consolas,monospace; font-size:12px; margin-top:10px; white-space:pre-wrap; }
    .bad { color:var(--danger); } .good { color:var(--ok); }
    @media (max-width:980px) { main { grid-template-columns:1fr; height:auto; } .status-grid { grid-template-columns:repeat(2,1fr); } }
  </style>
</head>
<body>
  <header><h1>C 语言电梯状态机</h1><div><button onclick="step()">运行一步</button> <button onclick="toggleAuto()" id="autoButton">自动运行</button> <button onclick="eventCall('reset')">重置</button></div></header>
  <main>
    <section><h2>楼层外部按钮</h2><div id="hall"></div></section>
    <section><h2>状态信息</h2><div class="status-grid" id="status"></div><h2 style="margin-top:12px;">电梯井</h2><div id="shaft"></div><div id="log"></div></section>
    <section>
      <h2>电梯内部按钮</h2><div class="cabin-grid" id="cabin"></div>
      <div class="controls">
        <button onclick="eventCall('door_open_hold')">按住开门</button><button onclick="eventCall('door_open_release')">松开开门</button>
        <button onclick="eventCall('door_close')">关门</button><button onclick="eventCall('emergency')">紧急呼叫</button>
        <button onclick="eventCall('clear_emergency')" class="wide">清除紧急呼叫</button>
      </div>
      <h2 style="margin-top:16px;">安全与管理控制</h2>
      <div class="controls">
        <div class="inline wide"><input id="load" type="number" value="0"><button onclick="eventCall('load', Number(document.getElementById('load').value))">设置载重</button></div>
        <button onclick="eventCall('door_blocked', 1)">门被阻挡</button><button onclick="eventCall('door_blocked', 0)">解除阻挡</button>
        <button onclick="eventCall('admin_pause')">管理员暂停</button><button onclick="eventCall('admin_resume')">管理员恢复</button>
        <button onclick="eventCall('power_failure')">模拟停电</button><button onclick="eventCall('backup_rescue')">备用救援</button>
        <button onclick="eventCall('restore_power')">恢复供电</button><button onclick="eventCall('recovery')">安全恢复</button>
        <div class="inline wide"><select id="fault"><option value="1">门故障</option><option value="2">电机故障</option><option value="3">传感器故障</option><option value="4">未知故障</option></select><button onclick="eventCall('fault', Number(document.getElementById('fault').value))">设置故障</button></div>
        <button onclick="eventCall('clear_fault')" class="wide">清除故障</button>
      </div>
    </section>
  </main>
<script>
let latest = null;
let autoTimer = null;
function floorList(){ const floors=[]; for(let i=34;i>=1;i--) floors.push(i); floors.push(-1); return floors; }
function log(message){ const box=document.getElementById("log"); box.textContent += message + "\n"; box.scrollTop = box.scrollHeight; }
async function api(path, body){ const options = body === undefined ? {} : { method:"POST", headers:{"Content-Type":"application/json"}, body:JSON.stringify(body) }; const response = await fetch(path, options); return response.json(); }
async function refresh(){ latest = await api("/api/snapshot"); renderStatus(latest); renderHall(); renderShaft(latest); renderCabin(); }
async function eventCall(action, value){ let result = action === "reset" ? await api("/api/reset", {}) : await api("/api/event", {action, value}); log(`${action}${value === undefined ? "" : " " + value}: ${result.message}`); latest = result.snapshot; renderStatus(latest); renderHall(); renderShaft(latest); renderCabin(); }
async function step(){ const result = await api("/api/step", {}); log(result.message); latest = result.snapshot; renderStatus(latest); renderHall(); renderShaft(latest); renderCabin(); }
function toggleAuto(){ const button=document.getElementById("autoButton"); if(autoTimer){ clearInterval(autoTimer); autoTimer=null; button.textContent="自动运行"; log("自动运行已停止。"); return; } button.textContent="停止"; log("自动运行已开始。"); autoTimer=setInterval(async()=>{ await step(); if(latest && !latest.hasAnyRequest && latest.state === "空闲"){ toggleAuto(); log("自动运行已完成。"); } },450); }
function metric(label,value,cls=""){ return `<div class="metric"><span>${label}</span><strong class="${cls}">${value}</strong></div>`; }
function yn(value){ return value ? "是" : "否"; }
function renderStatus(data){ document.getElementById("status").innerHTML = [
metric("当前楼层",data.currentFloor), metric("目标楼层",data.targetFloor), metric("状态",data.state), metric("方向",data.direction),
metric("运行时间",`${data.totalTimeSeconds}秒`), metric("载重",`${data.currentLoadKg}kg`,data.isOverloaded?"bad":""), metric("轿厢门",data.carDoor), metric("当前层门",data.currentLandingDoor),
metric("层门锁定",yn(data.currentLandingDoorLocked),data.currentLandingDoorLocked?"good":"bad"), metric("主电源",yn(data.isMainPowerOn),data.isMainPowerOn?"good":"bad"), metric("允许移动",yn(data.canMove),data.canMove?"good":"bad"), metric("紧急呼叫",yn(data.isEmergencyCallActive),data.isEmergencyCallActive?"bad":""),
metric("已完成请求",data.completedRequestCount), metric("平均等待",`${data.averageWaitTimeSeconds}秒`), metric("最长等待",`${data.longestWaitTimeSeconds}秒`), metric("故障",data.fault,data.fault==="无"?"":"bad")].join(""); }
function renderHall(){ document.getElementById("hall").innerHTML = floorList().map(floor => `<div class="hall-row"><div class="floor">${floor}</div><button ${floor>=34?"disabled":""} onclick="eventCall('hall_up', ${floor})">上行</button><button ${floor<1?"disabled":""} onclick="eventCall('hall_down', ${floor})">下行</button></div>`).join(""); }
function renderCabin(){ const floors=[-1]; for(let i=1;i<=34;i++) floors.push(i); document.getElementById("cabin").innerHTML = floors.reverse().map(floor => `<button onclick="eventCall('car', ${floor})">${floor}</button>`).join(""); }
function renderShaft(data){ document.getElementById("shaft").innerHTML = data.floors.map(row => { const requests=[row.hallUp?"上":"",row.hallDown?"下":"",row.car?"内":""].filter(Boolean).join(" "); const door=row.isCurrent ? `${row.landingDoorOpen?"打开":"关闭"}, ${row.landingDoorLocked?"已锁":"未锁"}` : ""; return `<div class="shaft-row ${row.isCurrent?"current":""}"><div class="floor">${row.floor}</div><div class="car">${row.isCurrent?"[电梯]":"| |"}</div><div class="requests">${requests}</div><div class="door">${door}</div></div>`; }).join(""); }
refresh(); log("中文界面已就绪。");
</script>
</body>
</html>
"""


class ElevatorRequestHandler(BaseHTTPRequestHandler):
    bridge = None

    def send_json(self, data):
        payload = json.dumps(data).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def read_json(self):
        length = int(self.headers.get("Content-Length", "0"))
        if length <= 0:
            return {}
        return json.loads(self.rfile.read(length).decode("utf-8"))

    def do_GET(self):
        if self.path in ("/", "/index.html"):
            payload = HTML.encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
        elif self.path == "/api/snapshot":
            self.send_json(self.bridge.snapshot())
        else:
            self.send_error(404)

    def do_POST(self):
        body = self.read_json()
        if self.path == "/api/event":
            self.send_json(self.bridge.call_event(body.get("action"), body.get("value")))
        elif self.path == "/api/step":
            self.send_json(self.bridge.run_step())
        elif self.path == "/api/reset":
            self.send_json(self.bridge.reset())
        else:
            self.send_error(404)

    def log_message(self, format_text, *args):
        return


def run_server(dll_path, self_test=False):
    ElevatorRequestHandler.bridge = ElevatorBridge(dll_path)
    if self_test:
        snapshot = ElevatorRequestHandler.bridge.snapshot()
        print(f"自检通过：当前楼层 {snapshot['currentFloor']}，状态 {snapshot['state']}")
        return 0

    url = f"http://{HOST}:{PORT}"
    print(f"电梯 Web 界面：{url}")
    webbrowser.open(url)
    ThreadingHTTPServer((HOST, PORT), ElevatorRequestHandler).serve_forever()
    return 0


def main():
    if len(sys.argv) < 2:
        print("用法：python ui/elevator_web.py <path-to-elevator-dll> [--self-test]")
        return 1
    return run_server(sys.argv[1], "--self-test" in sys.argv[2:])


if __name__ == "__main__":
    raise SystemExit(main())
