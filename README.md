# blinkkbd — Keyboard LED Blink Kernel Module

A Linux kernel module that blinks the Num Lock, Caps Lock, and Scroll Lock LEDs on a keyboard. Controlled entirely through `/proc/blinkkbd`. Written for CS430 Project 2.

---

## How It Works

When loaded, the module:
- Registers with the Linux input subsystem and attaches to the first keyboard that supports LED events
- Creates a writable `/proc/blinkkbd` entry
- Starts a kernel timer that toggles the selected LEDs on and off continuously
- Accepts two commands via `/proc` to change which LEDs blink and how fast

On unload, the timer is stopped, all LEDs are turned off, and the `/proc` entry is removed.

---

## Requirements

- Linux kernel with input subsystem support (standard on all Ubuntu installs)
- Kernel headers installed:
  ```
  sudo apt install linux-headers-$(uname -r)
  ```
- If Secure Boot is enabled, an enrolled MOK signing key (see Setup below)

---

## Setup

### 1. Build

```bash
make
```

### 2. Sign the module (Secure Boot only)

If Secure Boot is enabled on your system, the module must be signed before it can be loaded. Run this as a single line:

```bash
sudo /usr/src/linux-headers-$(uname -r)/scripts/sign-file sha256 signing_key.pem signing_cert.der blinkkbd.ko
```

No output means success.

> If you have not yet enrolled your signing certificate into the MOK database, run:
> ```bash
> sudo mokutil --import signing_cert.der
> ```
> Then reboot and follow the on-screen MOK enrollment prompt. After enrolling, sign and load as normal.

### 3. Load the module

```bash
sudo insmod blinkkbd.ko
```

### 4. Unload the module

```bash
sudo rmmod blinkkbd
```

Unloading stops all blinking and turns the LEDs off automatically.

---

## Commands

Write commands to `/proc/blinkkbd` using `sudo tee`:

```bash
echo <command> | sudo tee /proc/blinkkbd
```

### L — LED mask (which LEDs blink)

Bit 0 = Num Lock, Bit 1 = Caps Lock, Bit 2 = Scroll Lock

| Command | LEDs |
|---------|------|
| `echo L0 \| sudo tee /proc/blinkkbd` | Stop blinking, turn all LEDs off |
| `echo L1 \| sudo tee /proc/blinkkbd` | Num Lock only |
| `echo L2 \| sudo tee /proc/blinkkbd` | Caps Lock only |
| `echo L3 \| sudo tee /proc/blinkkbd` | Num + Caps |
| `echo L4 \| sudo tee /proc/blinkkbd` | Scroll Lock only |
| `echo L5 \| sudo tee /proc/blinkkbd` | Num + Scroll |
| `echo L6 \| sudo tee /proc/blinkkbd` | Caps + Scroll |
| `echo L7 \| sudo tee /proc/blinkkbd` | All three LEDs (default) |

### D — Blink speed (delay = HZ / divisor)

| Command | Speed |
|---------|-------|
| `echo D0 \| sudo tee /proc/blinkkbd` | Stop blinking, LEDs off |
| `echo D1 \| sudo tee /proc/blinkkbd` | Slowest (~1 toggle/sec) |
| `echo D2 \| sudo tee /proc/blinkkbd` | Slow |
| `echo D4 \| sudo tee /proc/blinkkbd` | Default speed |
| `echo D9 \| sudo tee /proc/blinkkbd` | Fastest |

Default on load: all three LEDs (L7) at D4 speed.

---

## Rebuild After Changes

```bash
make clean
make
sudo /usr/src/linux-headers-$(uname -r)/scripts/sign-file sha256 signing_key.pem signing_cert.der blinkkbd.ko
sudo rmmod blinkkbd 2>/dev/null; sudo insmod blinkkbd.ko
```

---

## Diagnostics

```bash
# View kernel log output
dmesg | tail -10

# Confirm module is loaded
cat /proc/modules | grep blink
```

---

## Files

| File | Purpose |
|------|---------|
| `blinkkbd.c` | Kernel module source code |
| `Makefile` | Build configuration |
| `signing_key.pem` | Private key for signing (keep private) |
| `signing_cert.pem` | Public certificate (PEM format) |
| `signing_cert.der` | Public certificate (DER format, used by sign-file) |
