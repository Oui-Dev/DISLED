# 0005 — GPL-3.0-or-later with a WCH SDK exception, CERN-OHL-S-2.0

- Status: Accepted
- Date: 2026-10-12

## Context

The project will be published. Modified versions that are distributed or
sold should stay open. A free USB PID from pid.codes requires a recognized
open source license for both software and hardware.

The firmware binary links the WCH SDK. Its files carry no standard license,
only a notice that they are meant for WCH microcontrollers. That notice is
not compatible with the GPL.

## Decision

- **Software:** GPL-3.0-or-later, with an additional permission under GPLv3
  section 7 that allows linking with the WCH SDK. The wording follows the
  Free Software Foundation template. See [LICENSING.md](../../LICENSING.md).
- **Hardware:** CERN-OHL-S-2.0 (strongly reciprocal).
- Every source file starts with an SPDX header that points to the
  additional permission.

## Consequences

- Anyone may build, modify and sell DISLED, but distributed modifications
  must publish their sources under the same licenses.
- Distributing firmware binaries is unambiguous despite the WCH SDK.
- The permission must cover every contribution, so contributors accept it
  together with the GPL.
