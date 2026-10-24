<!--
  SPDX-License-Identifier: GPL-3.0-or-later
  SPDX-FileCopyrightText: 2026 The DISLED contributors

  The Diagnostics window: identity of the lamp, its controls, diagnostics,
  self-test, live telemetry and recordings.
-->
<template>
  <main class="mx-auto flex max-w-6xl flex-col gap-4 p-5 md:p-6">
    <DiagnosticsHeader />
    <div class="grid items-start gap-4 min-[900px]:grid-cols-[minmax(19rem,2fr)_3fr]">
      <div class="flex flex-col gap-4">
        <DiagnosticsLamp />
        <DiagnosticsCounters />
        <DiagnosticsSelfTest />
      </div>
      <DiagnosticsTelemetry />
    </div>
  </main>
</template>
