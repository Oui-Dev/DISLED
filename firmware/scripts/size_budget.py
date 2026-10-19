# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 The DISLED contributors
# Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
"""Check flash and RAM usage of the firmware against a budget.

Runs after linking. It prints a one-line summary, warns above
``custom_budget_warn_percent`` and fails the build above 100 %.
RAM usage includes the reserved stack, as placed by the linker script.
"""

import subprocess
import sys

Import("env")  # noqa: F821 - provided by PlatformIO

FLASH_SECTIONS = (".init", ".vector", ".text", ".fini", ".preinit_array",
                  ".init_array", ".fini_array", ".ctors", ".dtors", ".data",
                  ".highcode_init", ".highcode")
RAM_SECTIONS = (".data", ".bss", ".stack", ".highcode_init", ".highcode")


def section_sizes(size_tool, elf_path):
    output = subprocess.run([size_tool, "-A", "-d", elf_path], capture_output=True,
                            text=True, check=True).stdout
    sizes = {}
    for line in output.splitlines():
        fields = line.split()
        if len(fields) >= 2 and fields[0].startswith(".") and fields[1].isdigit():
            sizes[fields[0]] = int(fields[1])
    return sizes


def check_budget(target, source, env):  # SCons passes keyword arguments
    elf_path = str(target[0])
    sizes = section_sizes(env.subst("$SIZETOOL"), elf_path)
    flash_used = sum(sizes.get(name, 0) for name in FLASH_SECTIONS)
    ram_used = sum(sizes.get(name, 0) for name in RAM_SECTIONS)

    flash_budget = int(env.GetProjectOption("custom_flash_budget", "32768"))
    ram_budget = int(env.GetProjectOption("custom_ram_budget", "10240"))
    warn_percent = int(env.GetProjectOption("custom_budget_warn_percent", "90"))

    failed = False
    for label, used, budget in (("Flash", flash_used, flash_budget),
                                ("RAM", ram_used, ram_budget)):
        percent = 100.0 * used / budget
        status = "ok"
        if percent > 100.0:
            status = "OVER BUDGET"
            failed = True
        elif percent >= warn_percent:
            status = "warning"
        print("Budget %-5s %6d / %6d bytes (%5.1f %%) %s" % (label, used, budget, percent, status))

    if failed:
        sys.stderr.write("Error: firmware exceeds its memory budget\n")
        env.Exit(1)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", check_budget)  # noqa: F821
