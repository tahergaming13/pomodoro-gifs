"""E2E round 2: verify auto WORK->SHORT_BREAK flip, clouds GIF, settings panel."""
import ctypes
import subprocess
import time
import os

from PIL import ImageGrab

BIN = r"C:\Users\MSI RAIDER\PomodoroApp\build\bin"
EXE = os.path.join(BIN, "Pomodoro.exe")
INI = os.path.join(BIN, "settings.ini")

VK_CONTROL, VK_MENU, VK_P = 0x11, 0x12, 0x50
KEYEVENTF_KEYUP = 0x0002
MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP = 0x0002, 0x0004
user32 = ctypes.windll.user32


def hotkey():
    for vk in (VK_CONTROL, VK_MENU, VK_P):
        user32.keybd_event(vk, 0, 0, 0)
        time.sleep(0.05)
    for vk in (VK_P, VK_MENU, VK_CONTROL):
        user32.keybd_event(vk, 0, KEYEVENTF_KEYUP, 0)
        time.sleep(0.05)


def click(x, y):
    user32.SetCursorPos(x, y)
    time.sleep(0.15)
    user32.mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
    time.sleep(0.05)
    user32.mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
    time.sleep(0.4)


def shot(name):
    p = os.path.join(r"C:\pbuild", name)
    ImageGrab.grab().save(p)
    print("saved", p, flush=True)


def win_rect():
    hwnd = user32.FindWindowW(None, "Pomodoro")
    r = ctypes.wintypes.RECT() if hasattr(ctypes, "wintypes") else None
    import ctypes as C

    class RECT(C.Structure):
        _fields_ = [("l", C.c_long), ("t", C.c_long),
                    ("r", C.c_long), ("b", C.c_long)]

    rc = RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(rc))
    return rc.l, rc.t, rc.r, rc.b


if os.path.exists(INI):
    os.remove(INI)
with open(INI, "w") as f:
    f.write("[Pomodoro]\nwork_min=1\nshort_min=1\nlong_min=5\ncycles=2\n"
            "sound=1\nalways_on_top=1\n")

# Guard: exactly one instance.
n = len(subprocess.run(["tasklist", "/FI", "IMAGENAME eq Pomodoro.exe"],
                       capture_output=True, text=True).stdout.strip().splitlines())
print("pomodoro lines in tasklist:", n, flush=True)

proc = subprocess.Popen([EXE], cwd=BIN)
time.sleep(3)
assert proc.poll() is None
wx, wy, _, _ = win_rect()
print("window at:", wx, wy, flush=True)
shot("r2_1_initial.png")

hotkey()
time.sleep(61)  # work phase (60s) elapses mid-wait
assert proc.poll() is None
shot("r2_2_break.png")

wx, wy, _, _ = win_rect()
click(wx + 233, wy + 21)  # Settings button
shot("r2_3_settings.png")
click(wx + 233, wy + 21)  # toggle closed

hwnd = user32.FindWindowW(None, "Pomodoro")
user32.PostMessageW(hwnd, 0x0010, 0, 0)
try:
    proc.wait(timeout=10)
except subprocess.TimeoutExpired:
    proc.kill()
    print("WARN: killed", flush=True)
print("exit:", proc.returncode, flush=True)
print(open(INI).read(), flush=True)
