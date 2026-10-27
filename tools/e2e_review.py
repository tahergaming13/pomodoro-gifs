"""End-to-end review: launch Pomodoro, hotkey-start it, capture screenshots,
wait for a WORK->SHORT_BREAK flip, then close gracefully and check settings."""
import ctypes
import subprocess
import sys
import time
import os

from PIL import ImageGrab

BIN = r"C:\Users\MSI RAIDER\PomodoroApp\build\bin"
EXE = os.path.join(BIN, "Pomodoro.exe")
INI = os.path.join(BIN, "settings.ini")

VK_CONTROL, VK_MENU, VK_P = 0x11, 0x12, 0x50
KEYEVENTF_KEYUP = 0x0002
user32 = ctypes.windll.user32


def hotkey():
    for vk in (VK_CONTROL, VK_MENU, VK_P):
        user32.keybd_event(vk, 0, 0, 0)
        time.sleep(0.05)
    for vk in (VK_P, VK_MENU, VK_CONTROL):
        user32.keybd_event(vk, 0, KEYEVENTF_KEYUP, 0)
        time.sleep(0.05)


def shot(name):
    p = os.path.join(r"C:\pbuild", name)
    ImageGrab.grab().save(p)
    print("saved", p, flush=True)


if os.path.exists(INI):
    os.remove(INI)
with open(INI, "w") as f:
    f.write("[Pomodoro]\nwork_min=1\nshort_min=1\nlong_min=1\ncycles=2\n"
            "sound=1\nalways_on_top=1\n")

proc = subprocess.Popen([EXE], cwd=BIN)
time.sleep(3)
assert proc.poll() is None, "app exited immediately!"
shot("shot1_initial.png")

hotkey()  # Ctrl+Alt+P -> start
time.sleep(4)
shot("shot2_running.png")

time.sleep(60)  # 1-min work phase elapses -> SHORT BREAK + beep
assert proc.poll() is None, "app died during run!"
shot("shot3_break.png")

# Graceful close (WM_CLOSE -> SDL_QUIT -> saves settings.ini).
hwnd = user32.FindWindowW(None, "Pomodoro")
print("hwnd:", hwnd, flush=True)
if hwnd:
    user32.PostMessageW(hwnd, 0x0010, 0, 0)  # WM_CLOSE
try:
    proc.wait(timeout=10)
except subprocess.TimeoutExpired:
    proc.kill()
    print("WARN: had to kill", flush=True)
print("exit code:", proc.returncode, flush=True)
print("--- settings.ini ---", flush=True)
print(open(INI).read(), flush=True)
